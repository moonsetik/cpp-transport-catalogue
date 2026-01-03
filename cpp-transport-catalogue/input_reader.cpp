#include "input_reader.h"

#include <algorithm>
#include <cassert>
#include <iterator>
#include <map>

namespace input_reader {

    Coordinates ParseCoordinates(std::string_view str) {
        static const double nan = std::nan("");

        auto not_space = str.find_first_not_of(' ');
        auto comma = str.find(',');

        if (comma == str.npos) {
            return { nan, nan };
        }

        auto not_space2 = str.find_first_not_of(' ', comma + 1);

        double lat = std::stod(std::string(str.substr(not_space, comma - not_space)));
        double lng = std::stod(std::string(str.substr(not_space2)));

        return { lat, lng };
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

        for (const auto& cmd : commands_) {
            if (cmd.command == "Stop") {
                Coordinates coords = ParseCoordinates(cmd.description);
                stops_to_add[cmd.id] = coords;
            }
        }

        for (const auto& [name, coords] : stops_to_add) {
            transport_catalogue::Stop stop{ name, coords };
            catalogue.AddStop(stop);
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

} // namespace input_reader