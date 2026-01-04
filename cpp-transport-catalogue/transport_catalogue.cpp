#include "transport_catalogue.h"

namespace transport_catalogue {

    void TransportCatalogue::AddStop(const Stop& stop) {
        if (stop_index_.count(stop.name)) {
            return;
        }

        stops_.push_back(stop);
        const Stop* added_stop = &stops_.back();
        stop_index_[added_stop->name] = added_stop;
    }

    void TransportCatalogue::AddBus(const Bus& bus) {
        if (bus_index_.count(bus.name)) {
            return;
        }

        buses_.push_back(bus);
        const Bus* added_bus = &buses_.back();
        bus_index_[added_bus->name] = added_bus;

        for (const Stop* stop : added_bus->stops) {
            stop_to_buses_[stop].insert(added_bus->name);
        }
    }

    const Stop* TransportCatalogue::FindStop(std::string_view name) const {
        auto it = stop_index_.find(name);
        return it != stop_index_.end() ? it->second : nullptr;
    }

    const Bus* TransportCatalogue::FindBus(std::string_view name) const {
        auto it = bus_index_.find(name);
        return it != bus_index_.end() ? it->second : nullptr;
    }

    std::optional<BusStats> TransportCatalogue::GetBusStats(std::string_view name) const {
        const Bus* bus = FindBus(name);
        if (!bus) {
            return std::nullopt;
        }

        BusStats stats;
        stats.stops_count = bus->stops.size();

        std::unordered_set<std::string_view> unique_names;
        for (const auto& stop : bus->stops) {
            unique_names.insert(stop->name);
        }
        stats.unique_stops_count = unique_names.size();

        stats.route_length = 0.0;
        for (size_t i = 1; i < bus->stops.size(); ++i) {
            stats.route_length += ComputeDistance(bus->stops[i - 1]->coordinates, bus->stops[i]->coordinates);
        }

        return stats;
    }

    std::optional<StopInfo> TransportCatalogue::GetStopInfo(std::string_view name) const {
        const Stop* stop = FindStop(name);
        if (!stop) {
            return std::nullopt;
        }

        StopInfo info;
        info.name = stop->name;

        auto it = stop_to_buses_.find(stop);
        if (it != stop_to_buses_.end()) {
            info.buses = &(it->second);
        }

        return info;
    }

} // namespace transport_catalogue