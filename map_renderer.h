#pragma once

#include <algorithm>
#include "svg.h"
#include "domain.h"

namespace map_render {

inline const double EPSILON = 1e-6;
inline bool IsZero(double value);

struct RenderSettings {
    double width = 0.0;
    double height = 0.0;
    double padding = 0.0;
    double line_width = 0.0;
    double stop_radius = 0.0;
    int bus_label_font_size = 0;
    std::pair<double, double> bus_label_offset;
    int stop_label_font_size = 0;
    std::pair<double, double> stop_label_offset;
    svg::Color underlayer_color;
    double underlayer_width = 0.0;
    std::vector<svg::Color> color_palette;
};

class SphereProjector;

class MapRender {
public:
    MapRender(RenderSettings settings);
    svg::Document GetMapForTC(std::vector<domain::StopPtr> stops, std::vector<domain::BusPtr> buses) const;

private:
    template <typename Obj>
    void AddToDocument(svg::Document& doc, std::vector<Obj>&& objects) const {
        for (auto&& obj : objects) {
            doc.Add(std::move(obj));
        }
    }

    void AddBusNameToContainer(
        std::vector<svg::Text>& bus_names, domain::BusPtr bus_ptr, size_t color_num, SphereProjector& projector) const;

    void AddStopNameToContainer(
        std::vector<svg::Text>& stop_names, domain::StopPtr stop_ptr, SphereProjector& projector) const;

    void AddCircleToContainer(
        std::vector<svg::Circle>& circles, domain::StopPtr stop_ptr, SphereProjector& projector) const;

private:
    RenderSettings settings_;

};

}
