#pragma once

#include <algorithm>
#include <optional>
#include <sstream>

#include "transport_catalogue.h"
#include "map_renderer.h"
#include "transport_router.h"

namespace handler {


using namespace reader;



class RequestHandler {
public:
    RequestHandler(const tc::TransportCatalogue& db, const map_render::MapRender& renderer, const transport_router::TransportRouter& router);

public:
    std::vector<StatInfo> GetStats(const std::vector<StatCommand>& commands) const;
    StatInfo GetStat(const StatCommand& command) const;

private:
    svg::Document RenderMap() const;

private:
    const tc::TransportCatalogue& db_;
    const map_render::MapRender& renderer_;
    const transport_router::TransportRouter& router_;
};

}
