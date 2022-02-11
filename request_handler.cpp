#include "request_handler.h"

namespace handler {

RequestHandler::RequestHandler(const tc::TransportCatalogue& db, const map_render::MapRender& renderer, const transport_router::TransportRouter& router)
    : db_(db)
    , renderer_(renderer)
    , router_(router) {
}

std::vector<StatInfo> RequestHandler::GetStats(const std::vector<StatCommand>& commands) const {
    std::vector<StatInfo> result;
    result.reserve(commands.size());
    for (const auto& command : commands) {
        result.push_back(std::move(GetStat(command)));
    }

    return result;
}

StatInfo RequestHandler::GetStat(const StatCommand& command) const {
    StatInfo result;
    result.id = command.id;
    result.query_type = command.query_type;
    switch (command.query_type) {
    case reader::QueryType::kStop:
        result.info = db_.GetBusesForStop(std::get<std::string>(command.data));
        break;
    case reader::QueryType::kBus: {
        const tc::BusPtr bus = db_.GetBusByName(std::get<std::string>(command.data));
        if (bus) {
            result.info = db_.GetStat(bus);
        }
    } break;
    case reader::QueryType::kMap: {
        std::ostringstream out;
        RenderMap().Render(out);
        result.info = std::move(out.str());

    } break;
    case reader::QueryType::kRoute: {
        const auto& route_command=std::get<RouteCommand>(command.data);
        std::optional<domain::TRouteStat> route_stat = router_.GetRoute(route_command.from, route_command.to);
        if (route_stat) {
            result.info = std::move(route_stat.value());
        }
    } break;

    }

    return result;
}

svg::Document RequestHandler::RenderMap() const {
    return renderer_.GetMapForTC(db_.GetStopsWithRoutes(), db_.GetBuses());
}

}