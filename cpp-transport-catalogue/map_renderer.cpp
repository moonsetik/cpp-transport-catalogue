#include "map_renderer.h"
#include "geo.h"
#include <set>
#include <vector>
#include <algorithm>
#include <optional>

namespace map_renderer {

namespace {

bool IsZero(double value) {
    const double EPSILON = 1e-6;
    return std::abs(value) < EPSILON;
}

class SphereProjector {
public:
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end,
                    double max_width, double max_height, double padding)
        : padding_(padding) {
        if (points_begin == points_end) {
            return;
        }

        const auto [left_it, right_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lng < rhs.lng; });
        min_lon_ = left_it->lng;
        const double max_lon = right_it->lng;

        const auto [bottom_it, top_it] = std::minmax_element(
            points_begin, points_end,
            [](auto lhs, auto rhs) { return lhs.lat < rhs.lat; });
        const double min_lat = bottom_it->lat;
        max_lat_ = top_it->lat;

        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (max_width - 2 * padding) / (max_lon - min_lon_);
        }

        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (max_height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        } else if (width_zoom) {
            zoom_coeff_ = *width_zoom;
        } else if (height_zoom) {
            zoom_coeff_ = *height_zoom;
        }
    }

    svg::Point operator()(Coordinates coords) const {
        return {
            (coords.lng - min_lon_) * zoom_coeff_ + padding_,
            (max_lat_ - coords.lat) * zoom_coeff_ + padding_
        };
    }

private:
    double padding_;
    double min_lon_ = 0;
    double max_lat_ = 0;
    double zoom_coeff_ = 0;
};

} // namespace

MapRenderer::MapRenderer(const transport_catalogue::TransportCatalogue& catalogue, const RenderSettings& settings)
    : catalogue_(catalogue), settings_(settings) {
}

svg::Document MapRenderer::Render() const {
    svg::Document doc;

    std::set<const transport_catalogue::Stop*> stops_in_routes;
    std::vector<const transport_catalogue::Bus*> buses_with_stops;
    for (const auto& bus : catalogue_.GetBuses()) {
        if (!bus.stops.empty()) {
            buses_with_stops.push_back(&bus);
            for (const auto* stop : bus.stops) {
                stops_in_routes.insert(stop);
            }
        }
    }

    if (stops_in_routes.empty()) {
        return doc; // пустая карта
    }

    std::vector<Coordinates> coords;
    coords.reserve(stops_in_routes.size());
    for (const auto* stop : stops_in_routes) {
        coords.push_back(stop->coordinates);
    }

    SphereProjector projector(coords.begin(), coords.end(),
                              settings_.width, settings_.height, settings_.padding);

    std::sort(buses_with_stops.begin(), buses_with_stops.end(),
              [](const auto* lhs, const auto* rhs) {
                  return lhs->name < rhs->name;
              });

    size_t color_index = 0;
    for (const auto* bus : buses_with_stops) {
        const auto& color = settings_.color_palette[color_index % settings_.color_palette.size()];
        ++color_index;

        std::vector<const transport_catalogue::Stop*> route_points = bus->stops;
        if (bus->is_roundtrip && !route_points.empty()) {
            if (route_points.back() != route_points.front()) {
                route_points.push_back(route_points.front());
            }
        }

        svg::Polyline polyline;
        for (const auto* stop : route_points) {
            polyline.AddPoint(projector(stop->coordinates));
        }
        polyline.SetFillColor(svg::NoneColor)
                .SetStrokeColor(color)
                .SetStrokeWidth(settings_.line_width)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        doc.Add(std::move(polyline));
    }

    color_index = 0; 
    for (const auto* bus : buses_with_stops) {
        const auto& color = settings_.color_palette[color_index % settings_.color_palette.size()];
        ++color_index;

        std::vector<const transport_catalogue::Stop*> endpoints;
        if (bus->is_roundtrip) {
            endpoints.push_back(bus->original_stops.front());
        } else {
            endpoints.push_back(bus->original_stops.front());
            if (bus->original_stops.size() > 1 && bus->original_stops.back() != bus->original_stops.front()) {
                endpoints.push_back(bus->original_stops.back());
            }
        }

        for (const auto* stop : endpoints) {
            svg::Point p = projector(stop->coordinates);

            svg::Text underlay;
            underlay.SetPosition(p)
                    .SetOffset({settings_.bus_label_offset.first, settings_.bus_label_offset.second})
                    .SetFontSize(settings_.bus_label_font_size)
                    .SetFontFamily("Verdana")
                    .SetFontWeight("bold")
                    .SetData(bus->name)
                    .SetFillColor(settings_.underlayer_color)
                    .SetStrokeColor(settings_.underlayer_color)
                    .SetStrokeWidth(settings_.underlayer_width)
                    .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                    .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
            doc.Add(std::move(underlay));

            svg::Text text;
            text.SetPosition(p)
                .SetOffset({settings_.bus_label_offset.first, settings_.bus_label_offset.second})
                .SetFontSize(settings_.bus_label_font_size)
                .SetFontFamily("Verdana")
                .SetFontWeight("bold")
                .SetData(bus->name)
                .SetFillColor(color);
            doc.Add(std::move(text));
        }
    }

    std::vector<const transport_catalogue::Stop*> sorted_stops(stops_in_routes.begin(), stops_in_routes.end());
    std::sort(sorted_stops.begin(), sorted_stops.end(),
              [](const auto* lhs, const auto* rhs) { return lhs->name < rhs->name; });

    for (const auto* stop : sorted_stops) {
        svg::Point p = projector(stop->coordinates);
        svg::Circle circle;
        circle.SetCenter(p)
              .SetRadius(settings_.stop_radius)
              .SetFillColor("white");
        doc.Add(std::move(circle));
    }

    for (const auto* stop : sorted_stops) {
        svg::Point p = projector(stop->coordinates);

        svg::Text underlay;
        underlay.SetPosition(p)
                .SetOffset({settings_.stop_label_offset.first, settings_.stop_label_offset.second})
                .SetFontSize(settings_.stop_label_font_size)
                .SetFontFamily("Verdana")
                .SetData(stop->name)
                .SetFillColor(settings_.underlayer_color)
                .SetStrokeColor(settings_.underlayer_color)
                .SetStrokeWidth(settings_.underlayer_width)
                .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
                .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);
        doc.Add(std::move(underlay));

        svg::Text text;
        text.SetPosition(p)
            .SetOffset({settings_.stop_label_offset.first, settings_.stop_label_offset.second})
            .SetFontSize(settings_.stop_label_font_size)
            .SetFontFamily("Verdana")
            .SetData(stop->name)
            .SetFillColor("black");
        doc.Add(std::move(text));
    }

    return doc;
}

} // namespace map_renderer