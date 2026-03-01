#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <optional>

#include "geo.h"
#include "domain.h"

namespace transport_catalogue {

    class TransportCatalogue {
    public:
        void AddStop(const Stop& stop);
        void AddBus(const Bus& bus);
        void AddDistance(const Stop* from, const Stop* to, double distance);
        double GetDistance(const Stop* from, const Stop* to) const;

        const Stop* FindStop(std::string_view name) const;
        const Bus* FindBus(std::string_view name) const;

        std::optional<BusStats> GetBusStats(std::string_view name) const;
        std::optional<StopInfo> GetStopInfo(std::string_view name) const;

        const std::deque<Bus>& GetBuses() const { return buses_; }

    private:
        std::deque<Stop> stops_;
        std::deque<Bus> buses_;

        std::unordered_map<std::string_view, const Stop*> stop_index_;
        std::unordered_map<std::string_view, const Bus*> bus_index_;
        std::unordered_map<const Stop*, std::set<std::string_view>> stop_to_buses_;
        std::unordered_map<std::pair<const Stop*, const Stop*>, double, PairHash> distances_;
    };

} // namespace transport_catalogue