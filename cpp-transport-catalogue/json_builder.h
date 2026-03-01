#pragma once

#include "json.h"
#include <optional>
#include <vector>
#include <string>

namespace json {

class Builder {
public:
    Builder& Key(std::string key);
    Builder& Value(Node value);
    Builder& StartDict();
    Builder& EndDict();
    Builder& StartArray();
    Builder& EndArray();
    Node Build();

private:
    void DoKey(std::string key);
    void DoStartDict();
    void DoStartArray();
    void DoEndDict();
    void DoEndArray();
    Node* AddNode(Node node);
    Node DoBuild() const;  

    struct Context {
        enum Type { DICT, ARRAY };
        Type type;
        Node* container;
        std::optional<std::string> pending_key;
    };

    std::optional<Node> root_;
    std::vector<Context> stack_;
};

} // namespace json