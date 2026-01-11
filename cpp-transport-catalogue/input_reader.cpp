#include "input_reader.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <map>
#include <regex>

namespace input_reader {

    struct StopWithDistances {
        Coordinates coords;
        std::vector<std::pair<std::string, double>> distances;
    };

    StopWithDistances ParseStopWithDistances(std::string_view str) {
        static const double nan = std::nan("");
        StopWithDistances result;

        std::string str_copy(str);
        std::regex distance_regex(R"((\d+)m to ([^,]+))");
        std::smatch matches;

        std::string remaining = str_copy;
        while (std::regex_search(remaining, matches, distance_regex)) {
            double distance = std::stod(matches[1].str());
            std::string stop_name = matches[2].str();

            size_t start = stop_name.find_first_not_of(' ');
            size_t end = stop_name.find_last_not_of(' ');
            if (start != std::string::npos && end != std::string::npos) {
                stop_name = stop_name.substr(start, end - start + 1);
            }

            result.distances.emplace_back(stop_name, distance);

            size_t pos = remaining.find(matches[0].str());
            remaining.erase(pos, matches[0].length());

            if (!remaining.empty() && remaining[0] == ',') {
                remaining.erase(0, 1);
            }
        }

        auto not_space = remaining.find_first_not_of(' ');
        auto comma = remaining.find(',');

        if (comma == std::string::npos) {
            result.coords = { nan, nan };
            return result;
        }

        auto not_space2 = remaining.find_first_not_of(' ', comma + 1);

        double lat = std::stod(remaining.substr(not_space, comma - not_space));
        double lng = std::stod(remaining.substr(not_space2));

        result.coords = { lat, lng };
        return result;
    }

    std::string_view Trim(std::string_view string) {
        const auto start = string.find_first_not_of(' ');
        if (start == std::string_view::npos) {
            return {};
        }
        return string.substr(start, string.find_last_not_of(' ') + 1 - start);
    }

    std::vector<std::string_view> Split(std::string_view string, char delim) {
        std::vector<std::string_view> result;

        size_t pos = 0;
        while ((pos = string.find_first_not_of(' ', pos)) < string.length()) {
            auto delim_pos = string.find(delim, pos);
            if (delim_pos == std::string_view::npos) {
                delim_pos = string.size();
            }
            if (auto substr = Trim(string.substr(pos, delim_pos - pos)); !substr.empty()) {
                result.push_back(substr);
            }
            pos = delim_pos + 1;
        }

        return result;
    }

    std::vector<std::string_view> ParseRoute(std::string_view route) {
        if (route.find('>') != std::string_view::npos) {
            return Split(route, '>');
        }

        auto stops = Split(route, '-');
        std::vector<std::string_view> results(stops.begin(), stops.end());
        results.insert(results.end(), std::next(stops.rbegin()), stops.rend());

        return results;
    }

    CommandDescription ParseCommandDescription(std::string_view line) {
        auto colon_pos = line.find(':');
        if (colon_pos == std::string_view::npos) {
            return {};
        }

        auto space_pos = line.find(' ');
        if (space_pos >= colon_pos) {
            return {};
        }

        auto not_space = line.find_first_not_of(' ', space_pos);
        if (not_space >= colon_pos) {
            return {};
        }

        return { std::string(line.substr(0, space_pos)),
                std::string(line.substr(not_space, colon_pos - not_space)),
                std::string(line.substr(colon_pos + 1)) };
    }

    void InputReader::ParseLine(std::string_view line) {
        auto command_description = ParseCommandDescription(line);
        if (command_description) {
            commands_.push_back(std::move(command_description));
        }
    }

    void InputReader::ApplyCommands(transport_catalogue::TransportCatalogue& catalogue) const {
        std::map<std::string, Coordinates> stops_to_add;
        std::map<std::string, std::vector<std::pair<std::string, double>>> distances_to_add;

        for (const auto& cmd : commands_) {
            if (cmd.command == "Stop") {
                auto stop_data = ParseStopWithDistances(cmd.description);
                stops_to_add[cmd.id] = stop_data.coords;
                if (!stop_data.distances.empty()) {
                    distances_to_add[cmd.id] = stop_data.distances;
                }
            }
        }

        for (const auto& [name, coords] : stops_to_add) {
            transport_catalogue::Stop stop{ name, coords };
            catalogue.AddStop(stop);
        }

        for (const auto& [from_name, distances] : distances_to_add) {
            const auto* from_stop = catalogue.FindStop(from_name);
            if (!from_stop) continue;

            for (const auto& [to_name, distance] : distances) {
                const auto* to_stop = catalogue.FindStop(to_name);
                if (to_stop) {
                    catalogue.AddDistance(from_stop, to_stop, distance);
                }
            }
        }

        for (const auto& cmd : commands_) {
            if (cmd.command == "Bus") {
                auto stop_names = ParseRoute(cmd.description);
                transport_catalogue::Bus bus;
                bus.name = cmd.id;
                bus.is_roundtrip = (cmd.description.find('>') != std::string_view::npos);

                for (const auto& stop_name : stop_names) {
                    const transport_catalogue::Stop* stop = catalogue.FindStop(stop_name);
                    if (stop) {
                        bus.stops.push_back(stop);
                    }
                }

                catalogue.AddBus(bus);
            }
        }
    }

    void ReadAndProcessBaseRequests(std::istream& input, transport_catalogue::TransportCatalogue& catalogue) {
        int base_request_count;
        input >> base_request_count >> std::ws;

        InputReader reader;
        for (int i = 0; i < base_request_count; ++i) {
            std::string line;
            std::getline(input, line);
            reader.ParseLine(line);
        }
        reader.ApplyCommands(catalogue);
    }

}