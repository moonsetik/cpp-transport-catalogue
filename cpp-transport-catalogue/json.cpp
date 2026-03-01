#include "json.h"

#include <cctype>
#include <charconv>
#include <iterator>
#include <string_view>

namespace json {

    using namespace std::literals;

    Node::Node(std::nullptr_t) : value_(nullptr) {}
    Node::Node(Array array) : value_(std::move(array)) {}
    Node::Node(Dict map) : value_(std::move(map)) {}
    Node::Node(bool value) : value_(value) {}
    Node::Node(int value) : value_(value) {}
    Node::Node(double value) : value_(value) {}
    Node::Node(std::string value) : value_(std::move(value)) {}

    bool Node::IsInt() const {
        return std::holds_alternative<int>(value_);
    }

    bool Node::IsDouble() const {
        return std::holds_alternative<double>(value_) || std::holds_alternative<int>(value_);
    }

    bool Node::IsPureDouble() const {
        return std::holds_alternative<double>(value_);
    }

    bool Node::IsBool() const {
        return std::holds_alternative<bool>(value_);
    }

    bool Node::IsString() const {
        return std::holds_alternative<std::string>(value_);
    }

    bool Node::IsNull() const {
        return std::holds_alternative<std::nullptr_t>(value_);
    }

    bool Node::IsArray() const {
        return std::holds_alternative<Array>(value_);
    }

    bool Node::IsMap() const {
        return std::holds_alternative<Dict>(value_);
    }

    int Node::AsInt() const {
        if (!IsInt()) {
            throw std::logic_error("Not an int");
        }
        return std::get<int>(value_);
    }

    bool Node::AsBool() const {
        if (!IsBool()) {
            throw std::logic_error("Not a bool");
        }
        return std::get<bool>(value_);
    }

    double Node::AsDouble() const {
        if (IsPureDouble()) {
            return std::get<double>(value_);
        }
        if (IsInt()) {
            return static_cast<double>(std::get<int>(value_));
        }
        throw std::logic_error("Not a double");
    }

    const std::string& Node::AsString() const {
        if (!IsString()) {
            throw std::logic_error("Not a string");
        }
        return std::get<std::string>(value_);
    }

    const Array& Node::AsArray() const {
        if (!IsArray()) {
            throw std::logic_error("Not an array");
        }
        return std::get<Array>(value_);
    }

    const Dict& Node::AsMap() const {
        if (!IsMap()) {
            throw std::logic_error("Not a map");
        }
        return std::get<Dict>(value_);
    }

    Array& Node::AsArray() {
        if (!IsArray()) {
            throw std::logic_error("Not an array");
        }
        return std::get<Array>(value_);
    }

    Dict& Node::AsMap() {
        if (!IsMap()) {
            throw std::logic_error("Not a map");
        }
        return std::get<Dict>(value_);
    }

    bool Node::operator==(const Node& other) const {
        return value_ == other.value_;
    }

    bool Node::operator!=(const Node& other) const {
        return !(*this == other);
    }

    Document::Document(Node root) : root_(std::move(root)) {}

    const Node& Document::GetRoot() const {
        return root_;
    }

    bool Document::operator==(const Document& other) const {
        return root_ == other.root_;
    }

    bool Document::operator!=(const Document& other) const {
        return !(*this == other);
    }

    namespace {

        Node LoadNode(std::istream& input);

        Node LoadNumber(std::istream& input) {
            using namespace std::literals;

            std::string parsed_num;

            auto read_char = [&parsed_num, &input] {
                parsed_num += static_cast<char>(input.get());
                if (!input) {
                    throw ParsingError("Failed to read number from stream"s);
                }
                };

            auto read_digits = [&input, read_char] {
                if (!std::isdigit(input.peek())) {
                    throw ParsingError("A digit is expected"s);
                }
                while (std::isdigit(input.peek())) {
                    read_char();
                }
                };

            if (input.peek() == '-') {
                read_char();
            }

            if (input.peek() == '0') {
                read_char();
            }
            else {
                read_digits();
            }

            bool is_int = true;

            if (input.peek() == '.') {
                read_char();
                read_digits();
                is_int = false;
            }

            if (int ch = input.peek(); ch == 'e' || ch == 'E') {
                read_char();
                if (ch = input.peek(); ch == '+' || ch == '-') {
                    read_char();
                }
                read_digits();
                is_int = false;
            }

            try {
                if (is_int) {
                    try {
                        return std::stoi(parsed_num);
                    }
                    catch (...) {
                    }
                }
                return std::stod(parsed_num);
            }
            catch (...) {
                throw ParsingError("Failed to convert "s + parsed_num + " to number"s);
            }
        }

        std::string LoadString(std::istream& input) {
            using namespace std::literals;

            auto it = std::istreambuf_iterator<char>(input);
            auto end = std::istreambuf_iterator<char>();
            std::string s;
            while (true) {
                if (it == end) {
                    throw ParsingError("String parsing error");
                }
                const char ch = *it;
                if (ch == '"') {
                    ++it;
                    break;
                }
                else if (ch == '\\') {
                    ++it;
                    if (it == end) {
                        throw ParsingError("String parsing error");
                    }
                    const char escaped_char = *(it);
                    switch (escaped_char) {
                    case 'n':
                        s.push_back('\n');
                        break;
                    case 't':
                        s.push_back('\t');
                        break;
                    case 'r':
                        s.push_back('\r');
                        break;
                    case '"':
                        s.push_back('"');
                        break;
                    case '\\':
                        s.push_back('\\');
                        break;
                    default:
                        throw ParsingError("Unrecognized escape sequence \\"s + escaped_char);
                    }
                }
                else if (ch == '\n' || ch == '\r') {
                    throw ParsingError("Unexpected end of line"s);
                }
                else {
                    s.push_back(ch);
                }
                ++it;
            }

            return s;
        }

        Node LoadArray(std::istream& input) {
            Array result;

            for (char c; input >> c && c != ']';) {
                if (c != ',') {
                    input.putback(c);
                }
                result.push_back(LoadNode(input));
            }

            if (!input) {
                throw ParsingError("Array parsing error");
            }

            return Node(std::move(result));
        }

        Node LoadDict(std::istream& input) {
            Dict result;

            for (char c; input >> c && c != '}';) {
                if (c == ',') {
                    input >> c;
                }

                if (c != '"') {
                    throw ParsingError("Expected '\"' in dictionary key"s);
                }

                std::string key = LoadString(input);

                input >> c;
                if (c != ':') {
                    throw ParsingError("Expected ':' after dictionary key"s);
                }

                result.emplace(std::move(key), LoadNode(input));
            }

            if (!input) {
                throw ParsingError("Dictionary parsing error");
            }

            return Node(std::move(result));
        }

        Node LoadNode(std::istream& input) {
            char c;
            input >> c;

            if (c == '"') {
                return Node(LoadString(input));
            }
            else if (c == '[') {
                return LoadArray(input);
            }
            else if (c == '{') {
                return LoadDict(input);
            }
            else if (c == 'n' || c == 't' || c == 'f') {
                std::string word;
                word += c;
                for (int i = 0; i < 3; ++i) {
                    if (!input) break;
                    word += static_cast<char>(input.get());
                }

                if (word == "null") {
                    if (input.peek() != EOF && !strchr(" \t\n\r,:]}", input.peek())) {
                        throw ParsingError("Invalid null value"s);
                    }
                    return Node(nullptr);
                }
                else if (word == "true") {
                    if (input.peek() != EOF && !strchr(" \t\n\r,:]}", input.peek())) {
                        throw ParsingError("Invalid boolean value"s);
                    }
                    return Node(true);
                }
                else if (word.size() >= 4 && word.substr(0, 4) == "fals") {
                    if (input) {
                        word += static_cast<char>(input.get());
                    }
                    if (word == "false") {
                        if (input.peek() != EOF && !strchr(" \t\n\r,:]}", input.peek())) {
                            throw ParsingError("Invalid boolean value"s);
                        }
                        return Node(false);
                    }
                }
                throw ParsingError("Invalid value"s);
            }
            else {
                input.putback(c);
                return LoadNumber(input);
            }
        }

        struct PrintContext {
            std::ostream& out;
            int indent_step = 4;
            int indent = 0;

            void PrintIndent() const {
                for (int i = 0; i < indent; ++i) {
                    out.put(' ');
                }
            }

            PrintContext Indented() const {
                return { out, indent_step, indent + indent_step };
            }
        };

        void PrintNode(const Node& node, const PrintContext& ctx);

        template <typename Value>
        void PrintValue(const Value& value, const PrintContext& ctx) {
            ctx.out << value;
        }

        void PrintValue(std::nullptr_t, const PrintContext& ctx) {
            ctx.out << "null";
        }

        void PrintValue(bool value, const PrintContext& ctx) {
            ctx.out << (value ? "true" : "false");
        }

        void PrintValue(const std::string& value, const PrintContext& ctx) {
            ctx.out << '"';
            for (char c : value) {
                switch (c) {
                case '\n': ctx.out << "\\n"; break;
                case '\r': ctx.out << "\\r"; break;
                case '"': ctx.out << "\\\""; break;
                case '\t': ctx.out << "\\t"; break;
                case '\\': ctx.out << "\\\\"; break;
                default: ctx.out << c; break;
                }
            }
            ctx.out << '"';
        }

        void PrintValue(const Array& array, const PrintContext& ctx) {
            ctx.out << '[';
            bool first = true;
            auto inner_ctx = ctx.Indented();
            for (const auto& item : array) {
                if (!first) {
                    ctx.out << ',';
                }
                first = false;
                if (ctx.indent_step > 0) {
                    ctx.out << '\n';
                    inner_ctx.PrintIndent();
                }
                PrintNode(item, inner_ctx);
            }
            if (ctx.indent_step > 0 && !array.empty()) {
                ctx.out << '\n';
                ctx.PrintIndent();
            }
            ctx.out << ']';
        }

        void PrintValue(const Dict& dict, const PrintContext& ctx) {
            ctx.out << '{';
            bool first = true;
            auto inner_ctx = ctx.Indented();
            for (const auto& [key, value] : dict) {
                if (!first) {
                    ctx.out << ',';
                }
                first = false;
                if (ctx.indent_step > 0) {
                    ctx.out << '\n';
                    inner_ctx.PrintIndent();
                }
                PrintValue(key, inner_ctx);
                ctx.out << ':';
                if (ctx.indent_step > 0) {
                    ctx.out << ' ';
                }
                PrintNode(value, inner_ctx);
            }
            if (ctx.indent_step > 0 && !dict.empty()) {
                ctx.out << '\n';
                ctx.PrintIndent();
            }
            ctx.out << '}';
        }

        void PrintNode(const Node& node, const PrintContext& ctx) {
            std::visit(
                [&ctx](const auto& value) {
                    PrintValue(value, ctx);
                },
                node.GetValue());
        }

    }  // namespace

    Document Load(std::istream& input) {
        return Document{ LoadNode(input) };
    }

    void Print(const Document& doc, std::ostream& output) {
        PrintContext ctx{ output };
        PrintNode(doc.GetRoot(), ctx);
    }

}  // namespace json