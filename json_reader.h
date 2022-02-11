#pragma once

#include "request_handler.h"
#include "json_builder.h"
#include "map_renderer.h"

namespace json_reader {
using namespace std::literals;

struct DistanceSpec {
    int distance_meters;
    std::string dest;
};

class JSONReader {
public:
    JSONReader(json::Document doc);

public:
    void LoadDataToTC(tc::TransportCatalogue& catalog) const;
    map_render::RenderSettings GetRenderSettings() const;
    void PrintStats(std::ostream& output, const std::vector<reader::StatInfo>& stats_info);
    std::vector<reader::StatCommand> GetStatCommands() const;
    domain::RouteSettings GetRouteSettings() const;

private:
    void LoadStopsToTC(tc::TransportCatalogue& catalog,
        std::vector<json::Node>::const_iterator first, std::vector<json::Node>::const_iterator last) const;
    domain::Stop ParseStopCommand(const json::Node& node) const;
    std::vector<DistanceSpec> ParseDistances(const json::Node node) const;
    void LoadBusToTC(tc::TransportCatalogue& catalog, const json::Node& node) const;
    svg::Color ParseColor(const json::Node& node) const;
    json::Node ParseMapStat(const reader::StatInfo& stat_info) const;
    json::Node ParseBusStat(const reader::StatInfo& stat_info) const;
    json::Node ParseStopStat(const reader::StatInfo& stat_info) const;
    json::Node ParseRouteStat(const reader::StatInfo& stat_info) const;
    reader::QueryType DefineRequestType(std::string_view query) const;

private:
    json::Document requests_;
};

}
