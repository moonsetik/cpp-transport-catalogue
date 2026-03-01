#pragma once

#include "svg.h"
#include "domain.h"
#include "transport_catalogue.h"

#include <vector>
#include <string>
#include <utility>
#include <set>

namespace map_renderer {

    class SphereProjector; 

    struct RenderSettings {
        double width;
        double height;
        double padding;
        double line_width;
        double stop_radius;
        int bus_label_font_size;
        std::pair<double, double> bus_label_offset;
        int stop_label_font_size;
        std::pair<double, double> stop_label_offset;
        svg::Color underlayer_color;
        double underlayer_width;
        std::vector<svg::Color> color_palette;
    };

    class MapRenderer {
    public:
        MapRenderer(const transport_catalogue::TransportCatalogue& catalogue, const RenderSettings& settings);
        svg::Document Render() const;

    private:
        const transport_catalogue::TransportCatalogue& catalogue_;
        RenderSettings settings_;

        //auto GatherData() const
        //    -> std::pair<std::set<const transport_catalogue::Stop*>,
        //    std::vector<const transport_catalogue::Bus*>>;

        void AddRouteLines(svg::Document& doc,
            const std::vector<const transport_catalogue::Bus*>& buses,
            const SphereProjector& projector) const;

        void AddBusLabels(svg::Document& doc,
            const std::vector<const transport_catalogue::Bus*>& buses,
            const SphereProjector& projector) const;

        void AddStopCircles(svg::Document& doc,
            const std::set<const transport_catalogue::Stop*>& stops,
            const SphereProjector& projector) const;

        void AddStopLabels(svg::Document& doc,
            const std::set<const transport_catalogue::Stop*>& stops,
            const SphereProjector& projector) const;
    };

} // namespace map_renderer