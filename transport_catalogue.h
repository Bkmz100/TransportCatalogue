#pragma once

#include <deque>
#include <set>
#include <string_view>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "domain.h"

namespace tc {
using namespace domain;

class TransportCatalogue {
public:
    StopPtr AddStop(Stop stop);
    BusPtr AddBus(Bus bus);
    void AddStopsDistance(StopPtr start, const std::pair<std::string_view, int>& end);
    int GetStopsDistance(StopPtr start, StopPtr end) const;
    StopPtr GetStopByName(std::string_view name) const;
    void AddStopsToBus(BusPtr bus,
        std::vector<StopPtr>::const_iterator first, std::vector<StopPtr>::const_iterator last);

    BusPtr GetBusByName(std::string_view name) const;
    StopStat GetBusesForStop(std::string_view stop_name) const;
    BusStat GetStat(BusPtr bus) const;
    std::vector<BusPtr> GetBuses() const;
    std::vector<StopPtr> GetStopsWithRoutes() const;

    const std::unordered_map<std::string_view, StopPtr>& GetNamesToStops() const;
    const std::unordered_map<std::string_view, BusPtr>& GetNamesToBuses() const;

private:
    std::deque<Stop> stops_;
    std::deque<Bus> buses_;
    std::unordered_map<StopPtr, std::unordered_set<BusPtr>> bus_by_stop_;
    std::unordered_map<std::string_view, StopPtr> names_to_stops_;
    std::unordered_map<std::string_view, BusPtr> names_to_buses_;
    std::unordered_map<std::pair<StopPtr, StopPtr>, int, detail::PairHasher> distances_;
};
}