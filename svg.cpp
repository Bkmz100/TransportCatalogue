#include "svg.h"

namespace svg {

using namespace std::literals;

// ---------- Color ------------------

void OstreamColorPrinter::operator()(std::string color) {
    out << color;
}
void OstreamColorPrinter::operator()(const Rgb& rgb) {
    out << "rgb("s << static_cast<int>(rgb.red)
        << ","s << static_cast<int>(rgb.green)
        << ","s << static_cast<int>(rgb.blue) << ")"s;
}
void OstreamColorPrinter::operator()(const Rgba& rgba) {
    out << "rgba("s << static_cast<int>(rgba.red)
        << ","s << static_cast<int>(rgba.green)
        << ","s << static_cast<int>(rgba.blue)
        << ","s << rgba.opacity << ")"s;
}

std::ostream& operator<< (std::ostream& out, const Color& color) {
    std::visit(OstreamColorPrinter{ out }, color);
    return out;
}

// ---------- RenderContext ------------------

RenderContext::RenderContext(std::ostream& out)
    : out(out) {
}

RenderContext::RenderContext(std::ostream& out, int indent_step, int indent)
    : out(out)
    , indent_step(indent_step)
    , indent(indent) {
}

RenderContext RenderContext::Indented() const {
    return { out, indent_step, indent + indent_step };
}

void RenderContext::RenderIndent() const {
    for (int i = 0; i < indent; ++i) {
        out.put(' ');
    }
}

void Object::Render(const RenderContext& context) const {
    context.RenderIndent();
    RenderObject(context);
    context.out << std::endl;
}

// ---------- StrokeLineCap ------------------

std::ostream& operator<< (std::ostream& out, StrokeLineCap stroke_linecap) {
    using namespace std::literals;
    switch (stroke_linecap) {
    case svg::StrokeLineCap::BUTT: return out << "butt"sv;
    case svg::StrokeLineCap::ROUND: return out << "round"sv;
    case svg::StrokeLineCap::SQUARE: return out << "square"sv;
    }

    return out;
}

// ---------- StrokeLineJoin ------------------

std::ostream& operator<< (std::ostream& out, StrokeLineJoin stroke_linejoin) {
    using namespace std::literals;
    switch (stroke_linejoin) {
    case svg::StrokeLineJoin::ARCS: return out << "arcs"s;
    case svg::StrokeLineJoin::BEVEL: return out << "bevel"s;
    case svg::StrokeLineJoin::MITER: return out << "miter"s;
    case svg::StrokeLineJoin::MITER_CLIP: return out << "miter-clip"s;
    case svg::StrokeLineJoin::ROUND: return out << "round"s;
    }

    return out;
}

// ---------- Circle ------------------

Circle& Circle::SetCenter(const Point& center) {
    center_ = center;
    return *this;
}

Circle& Circle::SetRadius(double radius) {
    radius_ = radius;
    return *this;
}

void Circle::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<circle cx=\""s << center_.x << "\" cy=\""s << center_.y << "\" "s;
    out << "r=\""s << radius_ << "\""s;
    RenderAttrs(out);
    out << "/>"s;
}

// ---------- Polyline ------------------

Polyline& Polyline::AddPoint(const Point& point) {
    points_.push_back(point);
    return *this;
}

void Polyline::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<polyline points=\""s;
    bool first = true;
    for (const Point& point : points_) {
        if (first) {
            first = false;
        }
        else {
            out << " "s;
        }
        out << point.x << ","s << point.y;
    }
    out << "\"";
    RenderAttrs(out);
    out << "/>"s;
}

// ---------- Text ------------------

Text& Text::SetPosition(const Point& pos) {
    pos_ = pos;
    return *this;
}

Text& Text::SetOffset(const Point& offset) {
    offset_ = offset;
    return *this;
}

Text& Text::SetFontSize(uint32_t size) {
    size_ = size;
    return *this;
}

Text& Text::SetFontFamily(const std::string& font_family) {
    font_family_ = font_family;
    return *this;
}

Text& Text::SetFontWeight(const std::string& font_weight) {
    font_weight_ = font_weight;
    return *this;
}

Text& Text::SetData(const std::string& data) {
    data_ = data;
    return *this;
}

void HtmlEncodeString(std::ostream& out, std::string_view sv) {
    const std::map<char, std::string> escapes = {
        {'&', "&amp;"s},
        {'\"', "&quot;"s},
        {'\'', "&apos;"s},
        {'<', "&lt;"s},
        {'>', "&gt;"s}
    };

    for (char c : sv) {
        if (escapes.count(c)) {
            out << escapes.at(c);
        }
        else {
            out << c;
        }
    }
}

void Text::RenderObject(const RenderContext& context) const {
    auto& out = context.out;
    out << "<text"s;
    RenderAttrs(out);
    out << " x=\""s << pos_.x << "\" y=\""s << pos_.y << "\" dx=\""s << offset_.x << "\" "s <<
        "dy=\""s << offset_.y << "\" font-size=\""s << size_ << "\""s;
    if (!font_family_.empty()) {
        out << " font-family=\"" << font_family_ << "\""s;
    }
    if (!font_weight_.empty()) {
        out << " font-weight=\""s << font_weight_ << "\""s;
    }

    out << ">"s;
    HtmlEncodeString(out, data_);
    out << "</text>"s;


}

// ---------- Document ------------------

void Document::AddPtr(std::unique_ptr<Object>&& obj) {
    objects_.push_back(std::move(obj));
}

void Document::Render(std::ostream& out) const {
    using namespace std::literals;
    out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"s << std::endl;
    out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"s << std::endl;
    for (const auto& object : objects_) {
        out << "  "s;
        object->Render(out);
    }
    out << "</svg>"s;
}

}  // namespace svg
