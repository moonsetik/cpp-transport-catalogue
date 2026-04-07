#pragma once

#include "graph.h"
#include "router.h"
#include "transport_catalogue.h"

#include <memory>
#include <unordered_map>
#include <vector>
#include <optional>
#include <string>

namespace transport_router {

struct RoutingSettings {
    int bus_wait_time = 0;  
    double bus_velocity = 0.0;
};

struct RouteItem {
    enum class Type { Wait, Bus };
    Type type;
    std::string stop_name;
    std::string bus_name;
    int span_count = 0;
    double time = 0.0;
};

struct RouteInfo {
    double total_time = 0.0;
    std::vector<RouteItem> items;
};

class TransportRouter {
public:
    TransportRouter(const transport_catalogue::TransportCatalogue& catalogue,
                    const RoutingSettings& settings);

    std::optional<RouteInfo> BuildRoute(const std::string& from, const std::string& to) const;

private:
    struct EdgeData {
        std::string bus_name;
        const transport_catalogue::Stop* from_stop;
        const transport_catalogue::Stop* to_stop;
        int span_count;
    };

    const transport_catalogue::TransportCatalogue& catalogue_;
    RoutingSettings settings_;

    struct StopVertex {
        size_t wait_id;   
        size_t board_id;
    };
    std::unordered_map<const transport_catalogue::Stop*, StopVertex> stop_to_vertex_;
    std::vector<const transport_catalogue::Stop*> vertex_to_stop_;

    graph::DirectedWeightedGraph<double> graph_;
    std::unique_ptr<graph::Router<double>> router_;
    std::vector<EdgeData> edge_data_;

    void BuildGraph();
    double ComputeTravelTime(double distance_meters) const;
    void AddEdgeWait(const transport_catalogue::Stop* stop);
    void AddEdgesForBus(const transport_catalogue::Bus* bus);
};

} // namespace transport_router