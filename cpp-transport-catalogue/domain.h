#pragma once

#include <string>
#include <vector>
#include <set>
#include <unordered_set>
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
        std::vector<const Stop*> original_stops; 
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

} // namespace transport_catalogue