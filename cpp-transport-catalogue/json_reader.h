#pragma once

#include <iostream>
#include <optional>
#include "transport_catalogue.h"
#include "map_renderer.h"
#include "json.h"

namespace json_reader {

    class JSONReader {
    public:
        explicit JSONReader(transport_catalogue::TransportCatalogue& catalogue);
        void ProcessRequests(std::istream& input, std::ostream& output);

    private:
        transport_catalogue::TransportCatalogue& catalogue_;

        void ProcessBaseRequests(const json::Array& base_requests);
        void ProcessStops(const json::Array& base_requests);
        void ProcessDistances(const json::Array& base_requests);
        void ProcessBuses(const json::Array& base_requests);

        std::optional<map_renderer::RenderSettings> ReadRenderSettings(const json::Dict& render_dict);

        json::Array ProcessStatRequests(const json::Array& stat_requests,
            const std::optional<map_renderer::RenderSettings>& render_settings);

        json::Node ProcessBusRequest(int id, const std::string& name);
        json::Node ProcessStopRequest(int id, const std::string& name);
        json::Node ProcessMapRequest(int id, const map_renderer::RenderSettings& settings);

        svg::Color ConvertColor(const json::Node& node) const;
    };

} // namespace json_reader