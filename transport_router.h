#pragma once

#include <memory>

#include "transport_catalogue.h"
#include "router.h"

namespace transport_router {

struct StopIds {
	graph::VertexId id = 0u;
	graph::VertexId transfer_id = 0u;
};

struct TGraphData {
	std::string_view from;
	std::string_view to;
	std::optional<std::string_view> bus;
	int span_count = 0;
	double time = 0.0;
};

class TransportRouter {
public:
	TransportRouter(const tc::TransportCatalogue& catalog, domain::RouteSettings settings);

public:
	std::optional<domain::TRouteStat> GetRoute(std::string_view from, std::string_view to) const;

private:
	domain::TRouteItemStat TGraphDataToStat(const TGraphData& data) const;
	void AddStops(const std::unordered_map<std::string_view, domain::StopPtr>& stops);
	void BuildGraph(const tc::TransportCatalogue& catalog);

	template <typename Iter>
	void AddEdgesFromBusRoute(std::string_view bus, Iter first, Iter last, const tc::TransportCatalogue& catalog) {
		if (first == last) return;
		for (auto it_from = first; it_from != last; ++it_from) {
			domain::StopPtr last_stop = *it_from;
			double distance = 0.0;
			int span_count = 0;

			for (auto it_to = it_from + 1; it_to != last; ++it_to) {
				if (*it_from != *it_to) {
					graph::VertexId from = stop_to_stop_ids_.at((*it_from)->name).id;
					graph::VertexId to = stop_to_stop_ids_.at((*it_to)->name).transfer_id;

					distance += static_cast<double>(catalog.GetStopsDistance(last_stop, *it_to));
					++span_count;

					graph::EdgeId id = graph_.AddEdge({ from, to, distance / settings_.bus_velocity * 3.6 / 60.0 });
					edge_to_data_.emplace(
						id, 
						std::move(
							TGraphData{ 
								(*it_from)->name, 
								(*it_to)->name, 
								bus, 
								span_count, 
								distance / settings_.bus_velocity * 3.6 / 60.0 
							}
					));
				}

				last_stop = *it_to;
			}
		}
	}

private:
	graph::DirectedWeightedGraph<double> graph_;
	domain::RouteSettings settings_;
	std::unique_ptr<graph::Router<double>> router_;
	std::unordered_map<graph::EdgeId, TGraphData> edge_to_data_;
	std::unordered_map<std::string_view, StopIds> stop_to_stop_ids_;
};

}