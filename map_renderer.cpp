#include "map_renderer.h"

namespace map_render {

inline bool IsZero(double value) {
    return std::abs(value) < EPSILON;
}

class SphereProjector {
public:
    template <typename PointInputIt>
    SphereProjector(PointInputIt points_begin, PointInputIt points_end, double width, double height, double padding)
        : padding_(padding) {

        if (points_begin == points_end) return;

        const auto [left_it, right_it]
            = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) {
            return lhs->position.lng < rhs->position.lng;
                });
        min_lon_ = (*left_it)->position.lng;
        const double max_lon = (*right_it)->position.lng;

        const auto [bottom_it, top_it]
            = std::minmax_element(points_begin, points_end, [](auto lhs, auto rhs) {
            return lhs->position.lat < rhs->position.lat;
                });
        const double min_lat = (*bottom_it)->position.lat;
        max_lat_ = (*top_it)->position.lat;

        std::optional<double> width_zoom;
        if (!IsZero(max_lon - min_lon_)) {
            width_zoom = (width - 2 * padding) / (max_lon - min_lon_);
        }
        std::optional<double> height_zoom;
        if (!IsZero(max_lat_ - min_lat)) {
            height_zoom = (height - 2 * padding) / (max_lat_ - min_lat);
        }

        if (width_zoom && height_zoom) {
            zoom_coeff_ = std::min(*width_zoom, *height_zoom);
        }
        else if (width_zoom) {
            zoom_coeff_ = *width_zoom;
        }
        else if (height_zoom) {
            zoom_coeff_ = *height_zoom;
        }
    }

public:
    svg::Point operator()(geo::Coordinates coords) const {
        return { (coords.lng - min_lon_) * zoom_coeff_ + padding_,
                (max_lat_ - coords.lat) * zoom_coeff_ + padding_ };
    }

private:
    double padding_ = 0.0;
    double min_lon_ = 0.0;
    double max_lat_ = 0.0;
    double zoom_coeff_ = 0.0;

};

MapRender::MapRender(RenderSettings settings)
    : settings_(std::move(settings)) {
}

svg::Document MapRender::GetMapForTC(std::vector<domain::StopPtr> stops, std::vector<domain::BusPtr> buses) const {
    buses.erase(
        std::remove_if(buses.begin(), buses.end(), [](auto ptr) {return ptr->stops.empty(); }), buses.end()
    );

    std::sort(stops.begin(), stops.end(), [](auto lhs, auto rhs) {return lhs->name < rhs->name; });
    std::sort(buses.begin(), buses.end(), [](auto lhs, auto rhs) {return lhs->name < rhs->name; });

    SphereProjector projector(stops.begin(), stops.end(), settings_.width, settings_.height, settings_.padding);

    std::vector<svg::Polyline> polylines;
    std::vector<svg::Text> bus_names;
    std::vector<svg::Circle> circles;
    std::vector<svg::Text> stop_names;

    bus_names.reserve(buses.size() * 2);
    polylines.reserve(buses.size());
    circles.reserve(stops.size());
    stop_names.reserve(stops.size() * 2);

    size_t color_num = 0u;

    for (const auto& bus : buses) {
        if (color_num == settings_.color_palette.size()) color_num = 0u;

        svg::Polyline polyline = svg::Polyline()
            .SetStrokeColor(settings_.color_palette[color_num])
            .SetFillColor(svg::NoneColor)
            .SetStrokeWidth(settings_.line_width)
            .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
            .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

        for (auto it = bus->stops.begin(); it != bus->stops.end(); ++it) {
            polyline.AddPoint(projector((*it)->position));
        }

        if (!bus->is_roundtrip) {
            for (auto it = bus->stops.rbegin()+1; it != bus->stops.rend(); ++it) {
                polyline.AddPoint(projector((*it)->position));
            }
        }

        AddBusNameToContainer(bus_names, bus, color_num, projector);

        polylines.push_back(std::move(polyline));
        ++color_num;
    }

    for (auto it = stops.begin(); it != stops.end(); ++it) {
        AddCircleToContainer(circles, *it, projector);
        AddStopNameToContainer(stop_names, *it, projector);
    }

    svg::Document result;
    AddToDocument(result, std::move(polylines));
    AddToDocument(result, std::move(bus_names));
    AddToDocument(result, std::move(circles));
    AddToDocument(result, std::move(stop_names));

    return result;
}

void MapRender::AddBusNameToContainer(
    std::vector<svg::Text>& bus_names, domain::BusPtr bus_ptr, size_t color_num, SphereProjector& projector) const {
    using namespace std::literals;

    svg::Text default_bus_name = svg::Text()
        .SetOffset(svg::Point(settings_.bus_label_offset.first, settings_.bus_label_offset.second))
        .SetFontSize(settings_.bus_label_font_size)
        .SetFontFamily("Verdana"s)
        .SetFontWeight("bold"s)
        .SetData(bus_ptr->name)
        .SetFillColor(settings_.color_palette[color_num]);

    svg::Text default_bus_name_plate = svg::Text(default_bus_name)
        .SetFillColor(settings_.underlayer_color)
        .SetStrokeColor(settings_.underlayer_color)
        .SetStrokeWidth(settings_.underlayer_width)
        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND);

    bus_names.push_back(std::move(
        svg::Text(default_bus_name_plate).SetPosition(projector(bus_ptr->stops.front()->position))
    ));

    bus_names.push_back(std::move(
        svg::Text(default_bus_name).SetPosition(projector(bus_ptr->stops.front()->position))
    ));

    if (!bus_ptr->is_roundtrip && bus_ptr->stops.back() != bus_ptr->stops.front()) {
        bus_names.push_back(std::move(
            svg::Text(default_bus_name_plate)
            .SetPosition(projector(bus_ptr->stops.back()->position))
        ));

        bus_names.push_back(std::move(
            svg::Text(default_bus_name)
            .SetPosition(projector(bus_ptr->stops.back()->position))
        ));
    }
}

void MapRender::AddStopNameToContainer(
    std::vector<svg::Text>& stop_names, domain::StopPtr stop_ptr, SphereProjector& projector) const {
    using namespace std::literals;

    svg::Text stop_name = svg::Text()
        .SetPosition(projector(stop_ptr->position))
        .SetOffset({ settings_.stop_label_offset.first,settings_.stop_label_offset.second })
        .SetFontSize(settings_.stop_label_font_size)
        .SetFontFamily("Verdana"s)
        .SetData(stop_ptr->name)
        .SetFillColor("black"s);

    stop_names.push_back(std::move(
        svg::Text(stop_name)
        .SetFillColor(settings_.underlayer_color)
        .SetStrokeColor(settings_.underlayer_color)
        .SetStrokeWidth(settings_.underlayer_width)
        .SetStrokeLineCap(svg::StrokeLineCap::ROUND)
        .SetStrokeLineJoin(svg::StrokeLineJoin::ROUND)
    ));

    stop_names.push_back(std::move(stop_name));
}

void MapRender::AddCircleToContainer(
    std::vector<svg::Circle>& circles, domain::StopPtr stop_ptr, SphereProjector& projector) const {
    using namespace std::literals;

    circles.push_back(std::move(
        svg::Circle()
        .SetCenter(projector(stop_ptr->position))
        .SetRadius(settings_.stop_radius)
        .SetFillColor("white"s)
    ));
}

}
