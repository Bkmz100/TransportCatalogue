#include "json_reader.h"

namespace json_reader {
using namespace std::literals;

JSONReader::JSONReader(json::Document doc)
    : requests_(doc) {
}

void JSONReader::LoadDataToTC(tc::TransportCatalogue& catalog) const {
    json::Array base_requests = requests_.GetRoot().AsDict().at("base_requests"s).AsArray();

    auto it = std::partition(
        base_requests.begin(), base_requests.end(), [](const json::Node& node) {
            return node.AsDict().at("type"s).AsString() == "Stop"s;
        });

    LoadStopsToTC(catalog, base_requests.begin(), it);

    for (auto i = it; i != base_requests.end(); ++i) {
        LoadBusToTC(catalog, *i);
    }
}

map_render::RenderSettings JSONReader::GetRenderSettings() const {
    using namespace std::literals;

    map_render::RenderSettings settings;
    const json::Dict& map_render = requests_.GetRoot().AsDict().at("render_settings"s).AsDict();

    settings.width = map_render.at("width"s).AsDouble();
    settings.height = map_render.at("height"s).AsDouble();
    settings.padding = map_render.at("padding"s).AsDouble();
    settings.line_width = map_render.at("line_width"s).AsDouble();
    settings.stop_radius = map_render.at("stop_radius"s).AsDouble();
    settings.bus_label_font_size = map_render.at("bus_label_font_size"s).AsInt();
    settings.bus_label_offset.first = map_render.at("bus_label_offset"s).AsArray()[0].AsDouble();
    settings.bus_label_offset.second = map_render.at("bus_label_offset"s).AsArray()[1].AsDouble();
    settings.stop_label_font_size = map_render.at("stop_label_font_size"s).AsInt();
    settings.stop_label_offset.first = map_render.at("stop_label_offset"s).AsArray()[0].AsDouble();
    settings.stop_label_offset.second = map_render.at("stop_label_offset"s).AsArray()[1].AsDouble();
    settings.underlayer_color = ParseColor(map_render.at("underlayer_color"s));
    settings.underlayer_width = map_render.at("underlayer_width"s).AsDouble();

    for (const auto& node : map_render.at("color_palette"s).AsArray()) {
        settings.color_palette.push_back(ParseColor(node));
    }

    return settings;
}

void JSONReader::PrintStats(std::ostream& output, const std::vector<reader::StatInfo>& stats_info) {
    json::Array result;

    for (const auto& stat : stats_info) {
        switch (stat.query_type)
        {
        case reader::QueryType::kMap:
            result.push_back(std::move(ParseMapStat(stat)));
            break;
        case reader::QueryType::kStop:
            result.push_back(std::move(ParseStopStat(stat)));
            break;
        case reader::QueryType::kBus:
            result.push_back(std::move(ParseBusStat(stat)));
            break;
        case reader::QueryType::kRoute:
            result.push_back(std::move(ParseRouteStat(stat)));
            break;
        default:
            break;
        }
    }
    json::Print(json::Document(std::move(result)), output);
}

std::vector<reader::StatCommand> JSONReader::GetStatCommands() const {
    std::vector<reader::StatCommand> result;
    const json::Node& stat_requests = requests_.GetRoot().AsDict().at("stat_requests"s);

    for (const auto& node : stat_requests.AsArray()) {
        reader::StatCommand command;
        command.query_type = DefineRequestType(node.AsDict().at("type"s).AsString());
        command.id = node.AsDict().at("id"s).AsInt();
        if (command.query_type == reader::QueryType::kRoute) {
            command.data = reader::RouteCommand{
                node.AsDict().at("from"s).AsString(), node.AsDict().at("to"s).AsString()
            };
        } else
        if (command.query_type != reader::QueryType::kMap)
            command.data = node.AsDict().at("name"s).AsString();
        
        result.push_back(command);
    }

    return result;
}

domain::RouteSettings JSONReader::GetRouteSettings() const {
    domain::RouteSettings result;

    const json::Dict& stat_requests = requests_.GetRoot().AsDict().at("routing_settings"s).AsDict();
    result.bus_wait_time = stat_requests.at("bus_wait_time"s).AsDouble();
    result.bus_velocity = stat_requests.at("bus_velocity"s).AsDouble();

    return result;
}

void JSONReader::LoadStopsToTC(tc::TransportCatalogue& catalog,
    std::vector<json::Node>::const_iterator first, std::vector<json::Node>::const_iterator last) const {
    std::vector <std::pair<tc::StopPtr, std::vector<DistanceSpec>>> stops;
    stops.reserve(std::distance(first, last));

    for (auto it = first; it != last; ++it) {
        stops.push_back(
            { catalog.AddStop(std::move(ParseStopCommand(*it))), ParseDistances(*it) }
        );
    }

    for (const auto& stop : stops) {
        for (size_t i = 0u; i < stop.second.size(); ++i) {
            catalog.AddStopsDistance(stop.first, { stop.second[i].dest, stop.second[i].distance_meters });
        }
    }
}

domain::Stop JSONReader::ParseStopCommand(const json::Node& node) const {
    tc::Stop result;
    const json::Dict& dict = node.AsDict();

    result.name = dict.at("name"s).AsString();
    result.position.lat = dict.at("latitude"s).AsDouble();
    result.position.lng = dict.at("longitude"s).AsDouble();

    return result;
}

std::vector<DistanceSpec> JSONReader::ParseDistances(const json::Node node) const {
    std::vector <DistanceSpec> distances;
    const json::Dict& dict = node.AsDict();

    if (dict.count("road_distances"s)) {
        for (const auto& [key, value] : dict.at("road_distances"s).AsDict()) {
            distances.push_back({ value.AsInt(), key });
        }
    }

    return distances;
}

void JSONReader::LoadBusToTC(tc::TransportCatalogue& catalog, const json::Node& node) const {
    std::vector<tc::StopPtr> stops_in_tc;
    tc::Bus bus;
    const json::Dict& dict = node.AsDict();

    bus.name = dict.at("name"s).AsString();
    bus.is_roundtrip = dict.at("is_roundtrip"s).AsBool();

    stops_in_tc.reserve(dict.at("stops"s).AsArray().size());

    for (const auto& stop : dict.at("stops"s).AsArray()) {
        stops_in_tc.push_back(catalog.GetStopByName(stop.AsString()));
    }

    bus.stops = stops_in_tc;

    catalog.AddStopsToBus(
        catalog.AddBus(std::move(bus)),
        stops_in_tc.begin(),
        stops_in_tc.end()
    );
}

svg::Color JSONReader::ParseColor(const json::Node& node) const {
    if (node.IsArray()) {
        const auto& color = node.AsArray();
        if (color.size() == 3) {
            return svg::Rgb(
                static_cast<uint8_t>(color[0].AsInt()),
                static_cast<uint8_t>(color[1].AsInt()),
                static_cast<uint8_t>(color[2].AsInt())
            );
        }
        else {
            return svg::Rgba(
                static_cast<uint8_t>(color[0].AsInt()),
                static_cast<uint8_t>(color[1].AsInt()),
                static_cast<uint8_t>(color[2].AsInt()),
                color[3].AsDouble()
            );
        }
    }

    return node.AsString();
}

json::Node JSONReader::ParseMapStat(const reader::StatInfo& stat_info) const {
    return
        json::Builder{}
            .StartDict()
                .Key("request_id"s).Value(stat_info.id)
                .Key("map"s).Value(std::get<std::string>(stat_info.info))
            .EndDict()
        .Build();
}

json::Node JSONReader::ParseBusStat(const reader::StatInfo& stat_info) const {
    if (const domain::BusStat* value = std::get_if<domain::BusStat>(&stat_info.info)) {
    return json::Builder{}.StartDict()
            .Key("request_id"s).Value(stat_info.id)
            .Key("curvature"s).Value(value->curvature)
            .Key("unique_stop_count"s).Value(static_cast<int>(value->unique_stops))
            .Key("stop_count"s).Value(static_cast<int>(value->stops_count))
            .Key("route_length"s).Value(value->route_length)
        .EndDict().Build();
    }

    return json::Builder{}.StartDict()
            .Key("request_id"s).Value(stat_info.id)
            .Key("error_message"s).Value("not found"s)
        .EndDict().Build();
}

json::Node JSONReader::ParseStopStat(const reader::StatInfo& stat_info) const {
    auto& info = std::get<domain::StopStat>(stat_info.info);
    if (info.is_found) {
        json::Array buses;
        for (std::string_view bus : info.data) {
            buses.push_back(json::Node(static_cast<std::string>(bus)));
        }
        return json::Builder{}.StartDict()
                .Key("request_id"s).Value(stat_info.id)
                .Key("buses"s).Value(buses)
            .EndDict().Build();
    }

    return json::Builder{}.StartDict()
            .Key("request_id"s).Value(stat_info.id)
            .Key("error_message"s).Value("not found"s)
        .EndDict().Build();
}

json::Node JSONReader::ParseRouteStat(const reader::StatInfo& stat_info) const {
    if (const domain::TRouteStat* value = std::get_if<domain::TRouteStat>(&stat_info.info)) {
        json::Array items;

        for (const auto& item : value->items) {
            if (item.type == domain::TRouteType::kWait) {
                items.push_back(
                    json::Builder{}.StartDict()
                    .Key("type"s).Value("Wait"s)
                    .Key("stop_name"s).Value(static_cast<std::string>(item.name))
                    .Key("time"s).Value(item.time)
                    .EndDict().Build()
                );
            }
            else {
                items.push_back(
                    json::Builder{}.StartDict()
                    .Key("type"s).Value("Bus"s)
                    .Key("bus"s).Value(static_cast<std::string>(item.name))
                    .Key("time"s).Value(item.time)
                    .Key("span_count"s).Value(item.span_cout)
                    .EndDict().Build()
                );
            }
        }

        return json::Builder{}.StartDict()
            .Key("request_id"s).Value(stat_info.id)
            .Key("total_time"s).Value(value->total_time)
            .Key("items"s).Value(items)
            .EndDict().Build();
    }

    return json::Builder{}.StartDict()
        .Key("request_id"s).Value(stat_info.id)
        .Key("error_message"s).Value("not found"s)
        .EndDict().Build();
}

reader::QueryType JSONReader::DefineRequestType(std::string_view query) const {
    if (query == "Bus"s) return reader::QueryType::kBus;
    if (query == "Map"s) return reader::QueryType::kMap;
    if (query == "Route"s) return reader::QueryType::kRoute;
    return reader::QueryType::kStop;
}

}