#include "request_handler.h"

RequestHandler::RequestHandler(const transport_catalogue::TransportCatalogue& catalogue)
    : catalogue_(catalogue) {
}

std::optional<transport_catalogue::BusStats> RequestHandler::GetBusStat(std::string_view bus_name) const {
    return catalogue_.GetBusStats(bus_name);
}

std::optional<transport_catalogue::StopInfo> RequestHandler::GetStopInfo(std::string_view stop_name) const {
    return catalogue_.GetStopInfo(stop_name);
}