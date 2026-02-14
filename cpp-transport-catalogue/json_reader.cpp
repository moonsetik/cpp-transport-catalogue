#include "json_reader.h"
#include "json.h"
#include "geo.h"
#include <algorithm>
#include <sstream>

using namespace std::literals;

namespace json_reader {

    JSONReader::JSONReader(transport_catalogue::TransportCatalogue& catalogue)
        : catalogue_(catalogue) {
    }

    void JSONReader::ProcessRequests(std::istream& input, std::ostream& output) {
        json::Document doc = json::Load(input);
        const json::Dict& root = doc.GetRoot().AsMap();

        auto base_it = root.find("base_requests");
        if (base_it != root.end() && base_it->second.IsArray()) {
            ProcessBaseRequests(base_it->second.AsArray());
        }

        std::optional<map_renderer::RenderSettings> render_settings;
        auto render_it = root.find("render_settings");
        if (render_it != root.end() && render_it->second.IsMap()) {
            render_settings = ReadRenderSettings(render_it->second.AsMap());
        }

        json::Array responses;
        auto stat_it = root.find("stat_requests");
        if (stat_it != root.end() && stat_it->second.IsArray()) {
            responses = ProcessStatRequests(stat_it->second.AsArray(), render_settings);
        }

        json::Document output_doc(std::move(responses));
        json::Print(output_doc, output);
    }

    void JSONReader::ProcessBaseRequests(const json::Array& base_requests) {
        ProcessStops(base_requests);
        ProcessDistances(base_requests);
        ProcessBuses(base_requests);
    }

    void JSONReader::ProcessStops(const json::Array& base_requests) {
        for (const auto& item : base_requests) {
            const json::Dict& request = item.AsMap();
            const std::string& type = request.at("type").AsString();
            if (type != "Stop") continue;
            const std::string& name = request.at("name").AsString();
            double lat = request.at("latitude").AsDouble();
            double lng = request.at("longitude").AsDouble();
            catalogue_.AddStop({ name, {lat, lng} });
        }
    }

    void JSONReader::ProcessDistances(const json::Array& base_requests) {
        for (const auto& item : base_requests) {
            const json::Dict& request = item.AsMap();
            const std::string& type = request.at("type").AsString();
            if (type != "Stop") continue;
            const std::string& name = request.at("name").AsString();
            const auto* from = catalogue_.FindStop(name);
            if (!from) continue;
            auto it = request.find("road_distances");
            if (it == request.end()) continue;
            const json::Dict& distances = it->second.AsMap();
            for (const auto& [neighbor_name, dist_node] : distances) {
                int distance = dist_node.AsInt();
                const auto* to = catalogue_.FindStop(neighbor_name);
                if (to) {
                    catalogue_.AddDistance(from, to, static_cast<double>(distance));
                }
            }
        }
    }

    void JSONReader::ProcessBuses(const json::Array& base_requests) {
        auto ExpandStops = [](const std::vector<std::string>& stops, bool is_roundtrip) {
            if (is_roundtrip) return stops;
            std::vector<std::string> result = stops;
            for (auto it = stops.rbegin() + 1; it != stops.rend(); ++it) {
                result.push_back(*it);
            }
            return result;
            };

        for (const auto& item : base_requests) {
            const json::Dict& request = item.AsMap();
            const std::string& type = request.at("type").AsString();
            if (type != "Bus") continue;
            const std::string& name = request.at("name").AsString();
            bool is_roundtrip = request.at("is_roundtrip").AsBool();
            const json::Array& stops_array = request.at("stops").AsArray();

            std::vector<std::string> stop_names;
            stop_names.reserve(stops_array.size());
            for (const auto& stop_node : stops_array) {
                stop_names.push_back(stop_node.AsString());
            }

            std::vector<std::string> expanded = ExpandStops(stop_names, is_roundtrip);

            transport_catalogue::Bus bus;
            bus.name = name;
            bus.is_roundtrip = is_roundtrip;

            for (const auto& stop_name : stop_names) {
                const auto* stop = catalogue_.FindStop(stop_name);
                if (stop) bus.original_stops.push_back(stop);
            }
            for (const auto& stop_name : expanded) {
                const auto* stop = catalogue_.FindStop(stop_name);
                if (stop) bus.stops.push_back(stop);
            }

            catalogue_.AddBus(bus);
        }
    }

    std::optional<map_renderer::RenderSettings> JSONReader::ReadRenderSettings(const json::Dict& render_dict) {
        map_renderer::RenderSettings settings;
        settings.width = render_dict.at("width").AsDouble();
        settings.height = render_dict.at("height").AsDouble();
        settings.padding = render_dict.at("padding").AsDouble();
        settings.line_width = render_dict.at("line_width").AsDouble();
        settings.stop_radius = render_dict.at("stop_radius").AsDouble();
        settings.bus_label_font_size = render_dict.at("bus_label_font_size").AsInt();

        const auto& bus_offset = render_dict.at("bus_label_offset").AsArray();
        settings.bus_label_offset = { bus_offset[0].AsDouble(), bus_offset[1].AsDouble() };

        settings.stop_label_font_size = render_dict.at("stop_label_font_size").AsInt();
        const auto& stop_offset = render_dict.at("stop_label_offset").AsArray();
        settings.stop_label_offset = { stop_offset[0].AsDouble(), stop_offset[1].AsDouble() };

        settings.underlayer_color = ConvertColor(render_dict.at("underlayer_color"));
        settings.underlayer_width = render_dict.at("underlayer_width").AsDouble();

        const auto& palette = render_dict.at("color_palette").AsArray();
        for (const auto& color_node : palette) {
            settings.color_palette.push_back(ConvertColor(color_node));
        }

        return settings;
    }

    json::Array JSONReader::ProcessStatRequests(const json::Array& stat_requests,
        const std::optional<map_renderer::RenderSettings>& render_settings) {
        json::Array responses;
        for (const auto& req_node : stat_requests) {
            const auto& request = req_node.AsMap();
            int id = request.at("id").AsInt();
            const std::string& type = request.at("type").AsString();

            if (type == "Bus") {
                const std::string& name = request.at("name").AsString();
                responses.push_back(ProcessBusRequest(id, name));
            }
            else if (type == "Stop") {
                const std::string& name = request.at("name").AsString();
                responses.push_back(ProcessStopRequest(id, name));
            }
            else if (type == "Map") {
                if (render_settings) {
                    responses.push_back(ProcessMapRequest(id, *render_settings));
                }
                else {
                    responses.push_back(json::Node(json::Dict{
                        {"request_id", id},
                        {"error_message", "not found"s}
                        }));
                }
            }
            else {
                responses.push_back(json::Node(json::Dict{
                    {"request_id", id},
                    {"error_message", "not found"s}
                    }));
            }
        }
        return responses;
    }

    json::Node JSONReader::ProcessBusRequest(int id, const std::string& name) {
        auto stats = catalogue_.GetBusStats(name);
        if (!stats) {
            return json::Node(json::Dict{
                {"request_id", id},
                {"error_message", "not found"s}
                });
        }
        json::Dict response;
        response["request_id"] = id;
        response["stop_count"] = static_cast<int>(stats->stops_count);
        response["unique_stop_count"] = static_cast<int>(stats->unique_stops_count);
        response["route_length"] = static_cast<int>(stats->route_length);
        response["curvature"] = stats->curvature;
        return json::Node(std::move(response));
    }

    json::Node JSONReader::ProcessStopRequest(int id, const std::string& name) {
        auto info = catalogue_.GetStopInfo(name);
        if (!info) {
            return json::Node(json::Dict{
                {"request_id", id},
                {"error_message", "not found"s}
                });
        }
        json::Dict response;
        response["request_id"] = id;

        json::Array buses_array;
        if (info->buses) {
            std::vector<std::string> bus_names;
            for (std::string_view bus_name : *info->buses) {
                bus_names.emplace_back(bus_name);
            }
            std::sort(bus_names.begin(), bus_names.end());
            for (const std::string& name : bus_names) {
                buses_array.emplace_back(name);
            }
        }
        response["buses"] = std::move(buses_array);
        return json::Node(std::move(response));
    }

    json::Node JSONReader::ProcessMapRequest(int id, const map_renderer::RenderSettings& settings) {
        map_renderer::MapRenderer renderer(catalogue_, settings);
        svg::Document doc = renderer.Render();
        std::ostringstream ss;
        doc.Render(ss);
        json::Dict response;
        response["request_id"] = id;
        response["map"] = ss.str();
        return json::Node(std::move(response));
    }

    svg::Color JSONReader::ConvertColor(const json::Node& node) const {
        if (node.IsString()) {
            return node.AsString();
        }
        if (node.IsArray()) {
            const auto& arr = node.AsArray();
            if (arr.size() == 3) {
                int r = arr[0].AsInt();
                int g = arr[1].AsInt();
                int b = arr[2].AsInt();
                return svg::Rgb(static_cast<uint8_t>(r),
                    static_cast<uint8_t>(g),
                    static_cast<uint8_t>(b));
            }
            if (arr.size() == 4) {
                int r = arr[0].AsInt();
                int g = arr[1].AsInt();
                int b = arr[2].AsInt();
                double o = arr[3].AsDouble();
                return svg::Rgba(static_cast<uint8_t>(r),
                    static_cast<uint8_t>(g),
                    static_cast<uint8_t>(b),
                    o);
            }
        }
        throw std::runtime_error("Invalid color format");
    }

} // namespace json_reader