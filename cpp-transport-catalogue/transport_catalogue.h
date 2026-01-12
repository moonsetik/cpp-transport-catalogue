#pragma once

#include <string>
#include <string_view>
#include <vector>
#include <deque>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <optional>
#include <iostream>
#include <iomanip>
#include <cmath>

#include "geo.h"

namespace transport_catalogue {

    struct Stop {
        std::string name;
        Coordinates coordinates;

        bool operator==(const Stop& other) const {
            return name == other.name && coordinates == other.coordinates;
        }
    };

    struct Bus {
        std::string name;
        std::vector<const Stop*> stops;
        bool is_roundtrip;
    };

    struct BusStats {
        size_t stops_count;
        size_t unique_stops_count;
        double route_length;
        double geographic_length;
        double curvature;
    };

    struct StopInfo {
        std::string_view name;
        const std::set<std::string_view>* buses = nullptr;
    };

    struct PairHash {
        size_t operator()(const std::pair<const Stop*, const Stop*>& p) const {
            auto hash1 = std::hash<const void*>{}(p.first);
            auto hash2 = std::hash<const void*>{}(p.second);
            return hash1 ^ (hash2 << 1);
        }
    };

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

    private:
        std::deque<Stop> stops_;
        std::deque<Bus> buses_;

        std::unordered_map<std::string_view, const Stop*> stop_index_;
        std::unordered_map<std::string_view, const Bus*> bus_index_;
        std::unordered_map<const Stop*, std::set<std::string_view>> stop_to_buses_;
        std::unordered_map<std::pair<const Stop*, const Stop*>, double, PairHash> distances_;
    };

}