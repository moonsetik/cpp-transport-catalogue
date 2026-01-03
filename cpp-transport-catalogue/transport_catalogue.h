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

        size_t GetUniqueStopsCount() const {
            std::unordered_set<std::string_view> unique_names;
            for (const auto& stop : stops) {
                unique_names.insert(stop->name);
            }
            return unique_names.size();
        }

        double CalculateRouteLength() const {
            double length = 0.0;
            for (size_t i = 1; i < stops.size(); ++i) {
                length += ComputeDistance(stops[i - 1]->coordinates, stops[i]->coordinates);
            }
            return length;
        }
    };

    struct BusInfo {
        std::optional<std::string> name;
        size_t stops_count = 0;
        size_t unique_stops_count = 0;
        double route_length = 0.0;
        bool found = false;
    };

    struct StopInfo {
        std::optional<std::string> name;
        std::set<std::string_view> buses;
        bool found = false;
    };

    class TransportCatalogue {
    public:
        void AddStop(const Stop& stop);
        void AddBus(const Bus& bus);

        const Stop* FindStop(std::string_view name) const;
        const Bus* FindBus(std::string_view name) const;

        BusInfo GetBusInfo(std::string_view name) const;
        StopInfo GetStopInfo(std::string_view name) const;

    private:
        std::deque<Stop> stops_;
        std::deque<Bus> buses_;

        std::unordered_map<std::string_view, const Stop*> stop_index_;
        std::unordered_map<std::string_view, const Bus*> bus_index_;
        std::unordered_map<const Stop*, std::set<std::string_view>> stop_to_buses_;
    };

} // namespace transport_catalogue