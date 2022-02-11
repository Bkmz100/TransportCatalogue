#include "transport_catalogue.h"

namespace tc {

StopPtr TransportCatalogue::AddStop(Stop stop) {
    const auto& ref = stops_.emplace_back(std::move(stop));
    names_to_stops_[ref.name] = &ref;
    bus_by_stop_[&ref];
    return &ref;
}

BusPtr TransportCatalogue::AddBus(Bus bus) {
    const auto& ref = buses_.emplace_back(std::move(bus));
    names_to_buses_[ref.name] = &ref;
    return &ref;
}

void TransportCatalogue::AddStopsDistance(StopPtr start, const std::pair<std::string_view, int>& end) {
    StopPtr end_stop = GetStopByName(end.first);
    distances_[{ start, end_stop }] = end.second;
}

int TransportCatalogue::GetStopsDistance(StopPtr start, StopPtr end) const {
    auto it = distances_.find({ start, end });

    if (it != distances_.end()) {
        return it->second;
    }

    it = distances_.find({ end, start });

    if (it != distances_.end()) {
        return it->second;
    }

    return 0;
}

StopPtr TransportCatalogue::GetStopByName(std::string_view name) const {
    auto it = names_to_stops_.find(name);

    if (it != names_to_stops_.end()) {
        return it->second;
    }
    return nullptr;
}

void TransportCatalogue::AddStopsToBus(BusPtr bus,
    std::vector<StopPtr>::const_iterator first, std::vector<StopPtr>::const_iterator last) {
    for (auto it = first; it != last; ++it) {
        bus_by_stop_[*it].insert(bus);
    }
}

BusPtr TransportCatalogue::GetBusByName(std::string_view name) const {
    auto it = names_to_buses_.find(name);

    if (it != names_to_buses_.end()) {
        return it->second;
    }

    return nullptr;
}

StopStat TransportCatalogue::GetBusesForStop(std::string_view stop_name) const {
    StopPtr stop = GetStopByName(stop_name);

    auto it = bus_by_stop_.find(stop);
    std::set<std::string_view> result;
    bool is_found = false;
    if (it != bus_by_stop_.end()) {
        for (const auto& stop : it->second) {
            result.insert(stop->name);
        }
        is_found = true;
    }

    return { is_found, result };
}

BusStat TransportCatalogue::GetStat(BusPtr bus) const {
    BusStat result;

    StopPtr last_stop = bus->stops[0];
    for (size_t i = 1; i < bus->stops.size(); ++i) {
        StopPtr stop = bus->stops[i];
        result.curvature += geo::ComputeDistance(last_stop->position, stop->position);
        result.route_length += GetStopsDistance(last_stop, stop);
        last_stop = stop;
    }

    if (!bus->is_roundtrip) {
        for (size_t i = bus->stops.size() - 1; i-- > 0u;) {
            StopPtr stop = bus->stops[i];
            result.curvature += geo::ComputeDistance(last_stop->position, stop->position);
            result.route_length += GetStopsDistance(last_stop, stop);
            last_stop = stop;
        }
        result.stops_count = bus->stops.size() * 2 - 1;
    }
    else {
        result.stops_count = bus->stops.size();
    }

    result.unique_stops = std::unordered_set<StopPtr>(bus->stops.cbegin(), bus->stops.cend()).size();
    result.curvature = static_cast<double>(result.route_length) / result.curvature;

    return result;
}

std::vector<BusPtr> TransportCatalogue::GetBuses() const {
    std::vector<BusPtr> result;
    for (const auto& [_, value] : names_to_buses_) {
        result.push_back(value);
    }
    return result;
}

std::vector<StopPtr> TransportCatalogue::GetStopsWithRoutes() const {
    std::vector<StopPtr> result;
    for (const auto& [key, value] : bus_by_stop_) {
        if (!value.empty())
            result.push_back(key);
    }

    return result;
}

const std::unordered_map<std::string_view, StopPtr>& TransportCatalogue::GetNamesToStops() const {
    return names_to_stops_;
}

const std::unordered_map<std::string_view, BusPtr>& TransportCatalogue::GetNamesToBuses() const {
    return names_to_buses_;
}

}