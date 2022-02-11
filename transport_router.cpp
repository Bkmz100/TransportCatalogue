#include "transport_router.h"

namespace transport_router {

TransportRouter::TransportRouter(const tc::TransportCatalogue& catalog, domain::RouteSettings settings)
	: graph_(catalog.GetNamesToStops().size() * 2u)
	, settings_(settings) {

	BuildGraph(catalog);
	router_ = std::make_unique<graph::Router<double>>(graph_);
}

std::optional<domain::TRouteStat> TransportRouter::GetRoute(std::string_view from, std::string_view to) const {
	auto from_id = stop_to_stop_ids_.at(from).transfer_id;
	auto to_id = stop_to_stop_ids_.at(to).transfer_id;

	auto route = router_->BuildRoute(from_id, to_id);

	if (route) {
		domain::TRouteStat data;
		data.total_time = route->weight;

		for (graph::EdgeId id : route->edges) {
			data.items.push_back(TGraphDataToStat(edge_to_data_.at(id)));
		}

		return data;
	}

	return std::nullopt;
}

domain::TRouteItemStat TransportRouter::TGraphDataToStat(const TGraphData& data) const {
	if (data.bus.has_value()) {
		return { domain::TRouteType::kBus, data.bus.value(), data.span_count, data.time };
	}

	return { domain::TRouteType::kWait, data.from, data.span_count, data.time };
}

void TransportRouter::AddStops(const std::unordered_map<std::string_view, domain::StopPtr>& stops) {
	graph::VertexId i = 0u;

	for (const auto& [_, stop] : stops) {
		stop_to_stop_ids_.insert({ stop->name, {i, i + 1} });
		graph::EdgeId edge_id = graph_.AddEdge({ i + 1, i, settings_.bus_wait_time });
		edge_to_data_.insert({ edge_id, {stop->name, stop->name, std::nullopt, 0, settings_.bus_wait_time} });

		i += 2;
	}
}

void TransportRouter::BuildGraph(const tc::TransportCatalogue& catalog) {
	AddStops(catalog.GetNamesToStops());

	for (const auto& [_, bus_ptr] : catalog.GetNamesToBuses()) {
		AddEdgesFromBusRoute(bus_ptr->name, bus_ptr->stops.begin(), bus_ptr->stops.end(), catalog);

		if (!bus_ptr->is_roundtrip) {
			AddEdgesFromBusRoute(bus_ptr->name, bus_ptr->stops.rbegin(), bus_ptr->stops.rend(), catalog);
		}
	}
}

}