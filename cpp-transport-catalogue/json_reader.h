#pragma once

#include <iostream>
#include <optional>
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"
#include "json.h"

namespace json_reader {

    class JSONReader {
    public:
        explicit JSONReader(transport_catalogue::TransportCatalogue& catalogue);
        void ProcessRequests(std::istream& input, std::ostream& output);

    private:
        transport_catalogue::TransportCatalogue& catalogue_;
        std::optional<map_renderer::RenderSettings> render_settings_;
        std::optional<transport_router::RoutingSettings> routing_settings_;

        void ProcessBaseRequests(const json::Array& base_requests);
        void ProcessStops(const json::Array& base_requests);
        void ProcessDistances(const json::Array& base_requests);
        void ProcessBuses(const json::Array& base_requests);

        void ProcessRoutingSettings(const json::Dict& routing_dict);
        std::optional<map_renderer::RenderSettings> ReadRenderSettings(const json::Dict& render_dict);

        json::Array ProcessStatRequests(const json::Array& stat_requests,
            const std::optional<map_renderer::RenderSettings>& render_settings,
            const std::optional<transport_router::TransportRouter>& router);

        json::Node ProcessBusRequest(int id, const std::string& name);
        json::Node ProcessStopRequest(int id, const std::string& name);
        json::Node ProcessMapRequest(int id, const map_renderer::RenderSettings& settings);
        json::Node ProcessRouteRequest(int id, const transport_router::TransportRouter& router,
                                       const std::string& from, const std::string& to);

        svg::Color ConvertColor(const json::Node& node) const;
    };

} // namespace json_reader