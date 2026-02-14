#include "svg.h"

#include <sstream>

namespace svg {

    using namespace std::literals;

    std::ostream& operator<<(std::ostream& out, StrokeLineCap line_cap) {
        switch (line_cap) {
        case StrokeLineCap::BUTT:
            out << "butt";
            break;
        case StrokeLineCap::ROUND:
            out << "round";
            break;
        case StrokeLineCap::SQUARE:
            out << "square";
            break;
        }
        return out;
    }

    std::ostream& operator<<(std::ostream& out, StrokeLineJoin line_join) {
        switch (line_join) {
        case StrokeLineJoin::ARCS:
            out << "arcs";
            break;
        case StrokeLineJoin::BEVEL:
            out << "bevel";
            break;
        case StrokeLineJoin::MITER:
            out << "miter";
            break;
        case StrokeLineJoin::MITER_CLIP:
            out << "miter-clip";
            break;
        case StrokeLineJoin::ROUND:
            out << "round";
            break;
        }
        return out;
    }

    std::ostream& operator<<(std::ostream& out, const Rgb& rgb) {
        out << "rgb("sv
            << static_cast<int>(rgb.red) << ","sv
            << static_cast<int>(rgb.green) << ","sv
            << static_cast<int>(rgb.blue) << ")"sv;
        return out;
    }

    std::ostream& operator<<(std::ostream& out, const Rgba& rgba) {
        out << "rgba("sv
            << static_cast<int>(rgba.red) << ","sv
            << static_cast<int>(rgba.green) << ","sv
            << static_cast<int>(rgba.blue) << ","sv
            << rgba.opacity << ")"sv;
        return out;
    }

    std::ostream& operator<<(std::ostream& out, const Color& color) {
        std::visit([&out](const auto& value) {
            using T = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<T, std::monostate>) {
                out << "none"sv;
            }
            else if constexpr (std::is_same_v<T, std::string>) {
                out << value;
            }
            else {
                out << value;
            }
            }, color);
        return out;
    }

    void Object::Render(const RenderContext& context) const {
        context.RenderIndent();
        RenderObject(context);
        context.out << std::endl;
    }

    Circle& Circle::SetCenter(Point center) {
        center_ = center;
        return *this;
    }

    Circle& Circle::SetRadius(double radius) {
        radius_ = radius;
        return *this;
    }

    void Circle::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<circle cx=\""sv << center_.x << "\" cy=\""sv << center_.y << "\" "sv;
        out << "r=\""sv << radius_ << "\""sv;
        RenderAttrs(out);
        out << "/>"sv;
    }

    Polyline& Polyline::AddPoint(Point point) {
        points_.push_back(point);
        return *this;
    }

    void Polyline::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<polyline points=\""sv;

        bool first = true;
        for (const auto& point : points_) {
            if (!first) {
                out << " "sv;
            }
            first = false;
            out << point.x << ","sv << point.y;
        }

        out << "\""sv;
        RenderAttrs(out);
        out << "/>"sv;
    }

    Text& Text::SetPosition(Point pos) {
        position_ = pos;
        return *this;
    }

    Text& Text::SetOffset(Point offset) {
        offset_ = offset;
        return *this;
    }

    Text& Text::SetFontSize(uint32_t size) {
        font_size_ = size;
        return *this;
    }

    Text& Text::SetFontFamily(std::string font_family) {
        font_family_ = std::move(font_family);
        return *this;
    }

    Text& Text::SetFontWeight(std::string font_weight) {
        font_weight_ = std::move(font_weight);
        return *this;
    }

    Text& Text::SetData(std::string data) {
        data_ = std::move(data);
        return *this;
    }

    std::string Text::EscapeText(const std::string& text) const {
        std::string result;
        for (char c : text) {
            switch (c) {
            case '"': result += "&quot;"sv; break;
            case '\'': result += "&apos;"sv; break;
            case '<': result += "&lt;"sv; break;
            case '>': result += "&gt;"sv; break;
            case '&': result += "&amp;"sv; break;
            default: result += c; break;
            }
        }
        return result;
    }

    void Text::RenderObject(const RenderContext& context) const {
        auto& out = context.out;
        out << "<text"sv;

        RenderAttrs(out);

        out << " x=\""sv << position_.x << "\""sv;
        out << " y=\""sv << position_.y << "\""sv;
        out << " dx=\""sv << offset_.x << "\""sv;
        out << " dy=\""sv << offset_.y << "\""sv;
        out << " font-size=\""sv << font_size_ << "\""sv;

        if (!font_family_.empty()) {
            out << " font-family=\""sv << font_family_ << "\""sv;
        }

        if (!font_weight_.empty()) {
            out << " font-weight=\""sv << font_weight_ << "\""sv;
        }

        out << ">"sv;
        out << EscapeText(data_);
        out << "</text>"sv;
    }

    void Document::AddPtr(std::unique_ptr<Object>&& obj) {
        objects_.push_back(std::move(obj));
    }

    void Document::Render(std::ostream& out) const {
        out << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>"sv << std::endl;
        out << "<svg xmlns=\"http://www.w3.org/2000/svg\" version=\"1.1\">"sv << std::endl;

        RenderContext ctx(out, 2, 2);
        for (const auto& obj : objects_) {
            obj->Render(ctx);
        }

        out << "</svg>"sv;
    }

}  // namespace svg