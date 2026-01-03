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

    BusInfo TransportCatalogue::GetBusInfo(std::string_view name) const {
        BusInfo info;

        const Bus* bus = FindBus(name);
        if (!bus) {
            info.found = false;
            return info;
        }

        info.name = std::string(name);
        info.stops_count = bus->stops.size();
        info.unique_stops_count = bus->GetUniqueStopsCount();
        info.route_length = bus->CalculateRouteLength();
        info.found = true;

        return info;
    }

    StopInfo TransportCatalogue::GetStopInfo(std::string_view name) const {
        StopInfo info;
        const Stop* stop = FindStop(name);

        if (!stop) {
            info.found = false;
            return info;
        }

        info.name = std::string(name);
        info.found = true;

        auto it = stop_to_buses_.find(stop);
        if (it != stop_to_buses_.end()) {
            info.buses = it->second;
        }

        return info;
    }

} // namespace transport_catalogue