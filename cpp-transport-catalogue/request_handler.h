#pragma once

#include <optional>
#include <string_view>

#include "domain.h"
#include "transport_catalogue.h"

class RequestHandler {
public:
    explicit RequestHandler(const transport_catalogue::TransportCatalogue& catalogue);

    std::optional<transport_catalogue::BusStats> GetBusStat(std::string_view bus_name) const;
    std::optional<transport_catalogue::StopInfo> GetStopInfo(std::string_view stop_name) const;

private:
    const transport_catalogue::TransportCatalogue& catalogue_;
};