#include "transport_router.h"
#include <cmath>

namespace transport_router {

TransportRouter::TransportRouter(const transport_catalogue::TransportCatalogue& catalogue,
                                 const RoutingSettings& settings)
    : catalogue_(catalogue), settings_(settings) {
    BuildGraph();
    router_ = std::make_unique<graph::Router<double>>(graph_);
}

double TransportRouter::ComputeTravelTime(double distance_meters) const {
    double distance_km = distance_meters / 1000.0;
    double hours = distance_km / settings_.bus_velocity;
    return hours * 60.0;
}

void TransportRouter::AddEdgeWait(const transport_catalogue::Stop* stop) {
    auto it = stop_to_vertex_.find(stop);
    if (it == stop_to_vertex_.end()) return;
    const auto& v = it->second;
    graph::Edge<double> edge{v.wait_id, v.board_id, static_cast<double>(settings_.bus_wait_time)};
    graph_.AddEdge(edge);
}

void TransportRouter::AddEdgesForBus(const transport_catalogue::Bus* bus) {
    const auto& stops = bus->stops;
    if (stops.size() < 2) return;

    for (size_t i = 0; i < stops.size(); ++i) {
        const auto* from_stop = stops[i];
        double accumulated_distance = 0.0;
        for (size_t j = i + 1; j < stops.size(); ++j) {
            const auto* to_stop = stops[j];
            accumulated_distance += catalogue_.GetDistance(stops[j-1], stops[j]);
            double travel_time = ComputeTravelTime(accumulated_distance);
            int span_count = static_cast<int>(j - i);

            auto from_it = stop_to_vertex_.find(from_stop);
            auto to_it = stop_to_vertex_.find(to_stop);
            if (from_it != stop_to_vertex_.end() && to_it != stop_to_vertex_.end()) {
                graph::Edge<double> edge{from_it->second.board_id, to_it->second.wait_id, travel_time};
                graph::EdgeId edge_id = graph_.AddEdge(edge);
                if (edge_data_.size() <= edge_id) edge_data_.resize(edge_id + 1);
                edge_data_[edge_id] = {bus->name, from_stop, to_stop, span_count};
            }
        }
    }
}

void TransportRouter::BuildGraph() {
    const auto all_stops = catalogue_.GetAllStops();
    const size_t stop_count = all_stops.size();
    graph_ = graph::DirectedWeightedGraph<double>(stop_count * 2);
    vertex_to_stop_.resize(stop_count * 2, nullptr);

    for (size_t i = 0; i < stop_count; ++i) {
        const auto* stop = all_stops[i];
        StopVertex sv{2*i, 2*i+1};
        stop_to_vertex_[stop] = sv;
        vertex_to_stop_[sv.wait_id] = stop;
        vertex_to_stop_[sv.board_id] = stop;
        AddEdgeWait(stop);
    }

    for (const auto& bus : catalogue_.GetBuses()) {
        AddEdgesForBus(&bus);
    }
}

std::optional<RouteInfo> TransportRouter::BuildRoute(const std::string& from, const std::string& to) const {
    const auto* stop_from = catalogue_.FindStop(from);
    const auto* stop_to = catalogue_.FindStop(to);
    if (!stop_from || !stop_to) return std::nullopt;

    auto from_it = stop_to_vertex_.find(stop_from);
    auto to_it = stop_to_vertex_.find(stop_to);
    if (from_it == stop_to_vertex_.end() || to_it == stop_to_vertex_.end()) return std::nullopt;

    auto route = router_->BuildRoute(from_it->second.wait_id, to_it->second.wait_id);
    if (!route) return std::nullopt;

    RouteInfo result;
    result.total_time = route->weight;

    for (graph::EdgeId edge_id : route->edges) {
        const auto& edge = graph_.GetEdge(edge_id);
        if (edge.from % 2 == 0 && edge.to == edge.from + 1) {
            RouteItem item;
            item.type = RouteItem::Type::Wait;
            item.stop_name = vertex_to_stop_[edge.from]->name;
            item.time = edge.weight;
            result.items.push_back(std::move(item));
        } else {
            const EdgeData& data = edge_data_.at(edge_id);
            RouteItem item;
            item.type = RouteItem::Type::Bus;
            item.bus_name = data.bus_name;
            item.span_count = data.span_count;
            item.time = edge.weight;
            result.items.push_back(std::move(item));
        }
    }

    return result;
}

} // namespace transport_router