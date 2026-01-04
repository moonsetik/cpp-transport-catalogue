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
            auto bus_stats = transport_catalogue.GetBusStats(argument);

            if (!bus_stats) {
                output << "Bus " << argument << ": not found\n";
            }
            else {
                output << "Bus " << argument << ": "
                    << bus_stats->stops_count << " stops on route, "
                    << bus_stats->unique_stops_count << " unique stops, "
                    << std::fixed << std::setprecision(6)
                    << bus_stats->route_length << " route length\n";
            }
        }
        else if (command == "Stop") {
            auto stop_info = transport_catalogue.GetStopInfo(argument);

            if (!stop_info) {
                output << "Stop " << argument << ": not found\n";
            }
            else if (!stop_info->buses || stop_info->buses->empty()) {
                output << "Stop " << stop_info->name << ": no buses\n";
            }
            else {
                output << "Stop " << stop_info->name << ": buses";
                for (const auto& bus_name : *(stop_info->buses)) {
                    output << " " << bus_name;
                }
                output << "\n";
            }
        }
    }

    void ProcessAndPrintStatRequests(std::istream& input, std::ostream& output,
        const transport_catalogue::TransportCatalogue& catalogue) {
        int stat_request_count;
        input >> stat_request_count >> std::ws;

        for (int i = 0; i < stat_request_count; ++i) {
            std::string line;
            std::getline(input, line);
            ParseAndPrintStat(catalogue, line, output);
        }
    }

} // namespace stat_reader