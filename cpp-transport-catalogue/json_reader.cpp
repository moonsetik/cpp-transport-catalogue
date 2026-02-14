#include "json_reader.h"

#include "json.h"
#include "transport_catalogue.h"
#include "request_handler.h"
#include "map_renderer.h"

#include <algorithm>
#include <vector>
#include <string>
#include <stdexcept>
#include <sstream>

using namespace std::literals;

namespace json_reader {

    namespace {

        svg::Color ConvertColor(const json::Node& node) {
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

        std::vector<std::string> ExpandStops(const std::vector<std::string>& stops, bool is_roundtrip) {
            if (is_roundtrip) {
                return stops;
            }
            std::vector<std::string> result = stops;
            for (auto it = stops.rbegin() + 1; it != stops.rend(); ++it) {
                result.push_back(*it);
            }
            return result;
        }

        void ProcessBaseRequests(const json::Array& base_requests,
                                 transport_catalogue::TransportCatalogue& catalogue) {
            for (const auto& item : base_requests) {
                const json::Dict& request = item.AsMap();
                const std::string& type = request.at("type").AsString();
                if (type == "Stop") {
                    const std::string& name = request.at("name").AsString();
                    double lat = request.at("latitude").AsDouble();
                    double lng = request.at("longitude").AsDouble();
                    catalogue.AddStop({ name, {lat, lng} });
                }
            }

            for (const auto& item : base_requests) {
                const json::Dict& request = item.AsMap();
                const std::string& type = request.at("type").AsString();
                if (type == "Stop") {
                    const std::string& name = request.at("name").AsString();
                    const auto* from = catalogue.FindStop(name);
                    if (!from) continue;

                    auto it = request.find("road_distances");
                    if (it == request.end()) continue;

                    const json::Dict& distances = it->second.AsMap();
                    for (const auto& [neighbor_name, dist_node] : distances) {
                        int distance = dist_node.AsInt();
                        const auto* to = catalogue.FindStop(neighbor_name);
                        if (to) {
                            catalogue.AddDistance(from, to, static_cast<double>(distance));
                        }
                    }
                }
            }

            for (const auto& item : base_requests) {
                const json::Dict& request = item.AsMap();
                const std::string& type = request.at("type").AsString();
                if (type == "Bus") {
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
                        const auto* stop = catalogue.FindStop(stop_name);
                        if (stop) {
                            bus.original_stops.push_back(stop);
                        }
                    }

                    for (const auto& stop_name : expanded) {
                        const auto* stop = catalogue.FindStop(stop_name);
                        if (stop) {
                            bus.stops.push_back(stop);
                        }
                    }

                    catalogue.AddBus(bus);
                }
            }
        }

        json::Node ProcessBusRequest(const RequestHandler& handler, int id, const std::string& name) {
            auto stats = handler.GetBusStat(name);
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

        json::Node ProcessStopRequest(const RequestHandler& handler, int id, const std::string& name) {
            auto info = handler.GetStopInfo(name);
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

        json::Node ProcessMapRequest(const transport_catalogue::TransportCatalogue& catalogue,
                                      const map_renderer::RenderSettings& settings,
                                      int id) {
            map_renderer::MapRenderer renderer(catalogue, settings);
            svg::Document doc = renderer.Render();
            std::ostringstream ss;
            doc.Render(ss);
            json::Dict response;
            response["request_id"] = id;
            response["map"] = ss.str();
            return json::Node(std::move(response));
        }

    } // namespace

    void ProcessRequests(std::istream& input, std::ostream& output,
                         transport_catalogue::TransportCatalogue& catalogue) {
        json::Document doc = json::Load(input);
        const json::Dict& root = doc.GetRoot().AsMap();

        auto base_it = root.find("base_requests");
        if (base_it != root.end() && base_it->second.IsArray()) {
            ProcessBaseRequests(base_it->second.AsArray(), catalogue);
        }

        std::optional<map_renderer::RenderSettings> render_settings;
        auto render_it = root.find("render_settings");
        if (render_it != root.end() && render_it->second.IsMap()) {
            const auto& render_dict = render_it->second.AsMap();

            map_renderer::RenderSettings settings;
            settings.width = render_dict.at("width").AsDouble();
            settings.height = render_dict.at("height").AsDouble();
            settings.padding = render_dict.at("padding").AsDouble();
            settings.line_width = render_dict.at("line_width").AsDouble();
            settings.stop_radius = render_dict.at("stop_radius").AsDouble();
            settings.bus_label_font_size = render_dict.at("bus_label_font_size").AsInt();

            const auto& bus_offset = render_dict.at("bus_label_offset").AsArray();
            settings.bus_label_offset = {bus_offset[0].AsDouble(), bus_offset[1].AsDouble()};

            settings.stop_label_font_size = render_dict.at("stop_label_font_size").AsInt();
            const auto& stop_offset = render_dict.at("stop_label_offset").AsArray();
            settings.stop_label_offset = {stop_offset[0].AsDouble(), stop_offset[1].AsDouble()};

            settings.underlayer_color = ConvertColor(render_dict.at("underlayer_color"));
            settings.underlayer_width = render_dict.at("underlayer_width").AsDouble();

            const auto& palette = render_dict.at("color_palette").AsArray();
            for (const auto& color_node : palette) {
                settings.color_palette.push_back(ConvertColor(color_node));
            }

            render_settings = std::move(settings);
        }

        RequestHandler handler(catalogue);
        json::Array responses;
        auto stat_it = root.find("stat_requests");
        if (stat_it != root.end() && stat_it->second.IsArray()) {
            const json::Array& stat_requests = stat_it->second.AsArray();
            for (const auto& req_node : stat_requests) {
                const auto& request = req_node.AsMap();
                int id = request.at("id").AsInt();
                const std::string& type = request.at("type").AsString();

                if (type == "Bus") {
                    const std::string& name = request.at("name").AsString();
                    responses.push_back(ProcessBusRequest(handler, id, name));
                } else if (type == "Stop") {
                    const std::string& name = request.at("name").AsString();
                    responses.push_back(ProcessStopRequest(handler, id, name));
                } else if (type == "Map") {
                    if (render_settings) {
                        responses.push_back(ProcessMapRequest(catalogue, *render_settings, id));
                    } else {
                        responses.push_back(json::Node(json::Dict{
                            {"request_id", id},
                            {"error_message", "not found"s}
                        }));
                    }
                } else {
                    responses.push_back(json::Node(json::Dict{
                        {"request_id", id},
                        {"error_message", "not found"s}
                    }));
                }
            }
        }

        json::Document output_doc(std::move(responses));
        json::Print(output_doc, output);
    }

} // namespace json_reader