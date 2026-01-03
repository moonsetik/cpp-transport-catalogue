#include "stat_reader.h"

#include <iomanip>
#include <sstream>

namespace stat_reader {

    void ParseAndPrintStat(const transport_catalogue::TransportCatalogue& transport_catalogue,
        std::string_view request, std::ostream& output) {
        auto trimmed_request = request;
        while (!trimmed_request.empty() && trimmed_request.front() == ' ') {
            trimmed_request.remove_prefix(1);
        }
        while (!trimmed_request.empty() && trimmed_request.back() == ' ') {
            trimmed_request.remove_suffix(1);
        }

        auto space_pos = trimmed_request.find(' ');
        if (space_pos == std::string_view::npos) {
            return;
        }

        std::string_view command = trimmed_request.substr(0, space_pos);
        std::string_view argument = trimmed_request.substr(space_pos + 1);

        if (command == "Bus") {
            transport_catalogue::BusInfo info = transport_catalogue.GetBusInfo(argument);

            if (info.found) {
                output << "Bus " << *info.name << ": "
                    << info.stops_count << " stops on route, "
                    << info.unique_stops_count << " unique stops, "
                    << std::fixed << std::setprecision(6)
                    << info.route_length << " route length\n";
            }
            else {
                output << "Bus " << argument << ": not found\n";
            }
        }
        else if (command == "Stop") {
            transport_catalogue::StopInfo info = transport_catalogue.GetStopInfo(argument);

            if (!info.found) {
                output << "Stop " << argument << ": not found\n";
            }
            else if (info.buses.empty()) {
                output << "Stop " << *info.name << ": no buses\n";
            }
            else {
                output << "Stop " << *info.name << ": buses";
                for (const auto& bus_name : info.buses) {
                    output << " " << bus_name;
                }
                output << "\n";
            }
        }
    }

} // namespace stat_reader