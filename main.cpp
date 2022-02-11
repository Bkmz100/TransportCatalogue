#include "transport_catalogue.h"
#include "json_reader.h"
#include "map_renderer.h"
#include "transport_router.h"

int main() {

    tc::TransportCatalogue catalog;
    json::Document doc(json::Load(std::cin));
    json_reader::JSONReader json(doc);
    map_render::MapRender map_render(json.GetRenderSettings());
    json.LoadDataToTC(catalog);

    transport_router::TransportRouter router(catalog, json.GetRouteSettings());
    handler::RequestHandler handler(catalog, map_render, router);

    json.PrintStats(std::cout, handler.GetStats(json.GetStatCommands()));

}