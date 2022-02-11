#pragma once

#include <cstdint>
#include <iostream>
#include <memory>
#include <string>
#include <string_view>
#include <vector>
#include <optional>
#include <variant>
#include <map>

namespace svg {

enum class StrokeLineCap {
    BUTT,
    ROUND,
    SQUARE,
};

enum class StrokeLineJoin {
    ARCS,
    BEVEL,
    MITER,
    MITER_CLIP,
    ROUND,
};

struct Rgb {
    Rgb() = default;
    Rgb(uint8_t r, uint8_t g, uint8_t b)
        :red(r),
        green(g),
        blue(b) {
    }

    uint8_t red = 0u;
    uint8_t green = 0u;
    uint8_t blue = 0u;
};

struct Rgba : Rgb {
    Rgba() = default;
    Rgba(uint8_t r, uint8_t g, uint8_t b, double opa)
        :Rgb(r, g, b),
        opacity(opa) {
    }
    double opacity = 1.0;
};

using Color = std::variant<std::monostate, std::string, Rgb, Rgba>;

inline const std::string NoneColor = std::string("none");
inline const std::string EmptyStringColor = std::string("");

struct OstreamColorPrinter {
    std::ostream& out;
    void operator()(std::string color);
    void operator()(const Rgb& rgb);
    void operator()(const Rgba& rgba);
    void operator()(std::monostate) {}
};

std::ostream& operator<< (std::ostream& out, const Color& color);

struct Point {
    Point() = default;
    Point(double x, double y)
        : x(x)
        , y(y) {
    }
    double x = 0;
    double y = 0;
};

struct RenderContext {
    RenderContext(std::ostream& out);
    RenderContext(std::ostream& out, int indent_step, int indent = 0);
    
    RenderContext Indented() const;
    void RenderIndent() const;

    std::ostream& out;
    int indent_step = 0;
    int indent = 0;
};

std::ostream& operator<< (std::ostream& out, StrokeLineCap stroke_linecap);
std::ostream& operator<< (std::ostream& out, StrokeLineJoin stroke_linejoin);

class Object {
public:
    virtual void Render(const RenderContext& context) const;

    virtual ~Object() = default;

private:
    virtual void RenderObject(const RenderContext& context) const = 0;
};

class ObjectContainer {
public:
    template <typename Obj>
    void Add(Obj obj) {
        AddPtr(std::make_unique<Obj>(std::move(obj)));
    }

   virtual void AddPtr(std::unique_ptr<Object>&& obj) = 0;

protected:
    ~ObjectContainer() = default;

};

class Drawable {
public:
    virtual void Draw(ObjectContainer& objects) const = 0;
    virtual ~Drawable() = default;
};

template <typename Owner>
class PathProps {
public:
    Owner& SetFillColor(Color color) {
        fill_color_ = std::move(color);
        return AsOwner();
    }
    Owner& SetStrokeColor(Color color) {
        stroke_color_ = std::move(color);
        return AsOwner();
    }
    Owner& SetStrokeWidth(double width) {
        stroke_width_ = width;
        return AsOwner();
    }
    Owner& SetStrokeLineCap(StrokeLineCap line_cap) {
        stroke_linecap_ = line_cap;
        return AsOwner();
    }
    Owner& SetStrokeLineJoin(StrokeLineJoin line_join) {
        stroke_linejoin_ = line_join;
        return AsOwner();
    }
protected:
    ~PathProps() = default;

    void RenderAttrs(std::ostream& out) const {
        using namespace std::literals;

        if (fill_color_) {
            out << " fill=\""sv << *fill_color_ << "\""sv;
        }
        if (stroke_color_) {
            out << " stroke=\""sv << *stroke_color_ << "\""sv;
        }
        if (stroke_width_) {
            out << " stroke-width=\""sv << *stroke_width_ << "\""sv;
        }
        if (stroke_linecap_) {
            out << " stroke-linecap=\""sv << *stroke_linecap_ << "\""sv;
        }
        if (stroke_linejoin_) {
            out << " stroke-linejoin=\""sv << *stroke_linejoin_ << "\""sv;
        }
    }

private:
    Owner& AsOwner() {
        return static_cast<Owner&>(*this);
    }

private:
    std::optional<Color> fill_color_;
    std::optional<Color> stroke_color_;
    std::optional<double> stroke_width_;
    std::optional<StrokeLineCap> stroke_linecap_;
    std::optional<StrokeLineJoin> stroke_linejoin_;
};

class Circle final : public Object, public PathProps<Circle> {
public:
    Circle& SetCenter(const Point& center);
    Circle& SetRadius(double radius);

private:
    void RenderObject(const RenderContext& context) const override;

private:
    Point center_;
    double radius_ = 1.0;
};

class Polyline final : public Object, public PathProps<Polyline> {
public:
    Polyline& AddPoint(const Point& point);

private:
    void RenderObject(const RenderContext& context) const override;

private:
    std::vector<Point> points_;
};

void HtmlEncodeString(std::ostream& out, std::string_view sv);

class Text final : public Object, public PathProps<Text> {
public:
    Text& SetPosition(const Point& pos);
    Text& SetOffset(const Point& offset);
    Text& SetFontSize(uint32_t size);
    Text& SetFontFamily(const std::string& font_family);
    Text& SetFontWeight(const std::string& font_weight);
    Text& SetData(const std::string& data);

private:
    void RenderObject(const RenderContext& context) const override;

private:
    Point pos_;
    Point offset_;
    uint32_t size_ = 1u;
    std::string font_family_;
    std::string font_weight_;
    std::string data_;
};

class Document final : public ObjectContainer {
public:
    void AddPtr(std::unique_ptr<Object>&& obj) override;
    void Render(std::ostream& out) const;

private:
    std::vector<std::unique_ptr<Object>> objects_;
};

}  // namespace svg
