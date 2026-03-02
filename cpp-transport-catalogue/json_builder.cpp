#include "json_builder.h"

namespace json {

using namespace std::literals;

void Builder::DoKey(std::string key) {
    if (stack_.empty() || stack_.back().type != Context::DICT || stack_.back().pending_key.has_value()) {
        throw std::logic_error("Key called in wrong context");
    }
    stack_.back().pending_key = std::move(key);
}

void Builder::DoStartDict() {
    Node* new_node = AddNode(Node(Dict{}));
    stack_.push_back(Context{Context::DICT, new_node, std::nullopt});
}

void Builder::DoStartArray() {
    Node* new_node = AddNode(Node(Array{}));
    stack_.push_back(Context{Context::ARRAY, new_node, std::nullopt});
}

void Builder::DoEndDict() {
    if (stack_.empty() || stack_.back().type != Context::DICT || stack_.back().pending_key.has_value()) {
        throw std::logic_error("EndDict called in wrong context");
    }
    stack_.pop_back();
}

void Builder::DoEndArray() {
    if (stack_.empty() || stack_.back().type != Context::ARRAY) {
        throw std::logic_error("EndArray called in wrong context");
    }
    stack_.pop_back();
}

Node* Builder::AddNode(Node node) {
    if (!root_ && stack_.empty()) {
        root_ = std::move(node);
        return &root_.value();
    }
    if (stack_.empty()) {
        throw std::logic_error("Cannot add value: object is already built");
    }
    Context& current = stack_.back();
    if (current.type == Context::ARRAY) {
        Array& arr = current.container->AsArray();
        arr.push_back(std::move(node));
        return &arr.back();
    }
    
    if (!current.pending_key.has_value()) {
        throw std::logic_error("No pending key for value in dict");
    }
    Dict& dict = current.container->AsMap();  
    auto [it, inserted] = dict.emplace(std::move(*current.pending_key), std::move(node));
    if (!inserted) {
        throw std::logic_error("Duplicate key in dict");
    }
    current.pending_key.reset();
    return &it->second;
}

Node Builder::DoBuild() const {
    if (!root_ || !stack_.empty()) {
        throw std::logic_error("Invalid Build: object is not complete");
    }
    return *root_;
}

} // namespace json