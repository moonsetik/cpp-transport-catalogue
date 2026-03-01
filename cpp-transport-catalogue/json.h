#pragma once

#include <cstddef>
#include <iostream>
#include <map>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>
#include <cstring>

namespace json {

    class Node;
    class Document;

    using Array = std::vector<Node>;
    using Dict = std::map<std::string, Node>;

    class ParsingError : public std::runtime_error {
    public:
        using runtime_error::runtime_error;
    };

    class Node {
    public:
        using Value = std::variant<std::nullptr_t, Array, Dict, bool, int, double, std::string>;

        Node() = default;
        Node(std::nullptr_t);
        Node(Array array);
        Node(Dict map);
        Node(bool value);
        Node(int value);
        Node(double value);
        Node(std::string value);

        bool IsInt() const;
        bool IsDouble() const;
        bool IsPureDouble() const;
        bool IsBool() const;
        bool IsString() const;
        bool IsNull() const;
        bool IsArray() const;
        bool IsMap() const;

        int AsInt() const;
        bool AsBool() const;
        double AsDouble() const;
        const std::string& AsString() const;
        const Array& AsArray() const;
        const Dict& AsMap() const;

        // Неконстантные версии для модификации
        Array& AsArray();
        Dict& AsMap();

        const Value& GetValue() const { return value_; }
        Value& GetValue() { return value_; }

        bool operator==(const Node& other) const;
        bool operator!=(const Node& other) const;

    private:
        Value value_;
    };

    class Document {
    public:
        explicit Document(Node root);

        const Node& GetRoot() const;

        bool operator==(const Document& other) const;
        bool operator!=(const Document& other) const;

    private:
        Node root_;
    };

    Document Load(std::istream& input);
    void Print(const Document& doc, std::ostream& output);

}  // namespace json