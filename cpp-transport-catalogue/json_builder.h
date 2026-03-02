#pragma once
#include "json.h"
#include <optional>
#include <vector>
#include <string>
#include <stdexcept>
#include <utility>

namespace json {

class Builder;

template<typename Parent> class DictContext;
template<typename Parent> class KeyContext;
template<typename Parent> class ValueAfterKeyContext;
template<typename Parent> class ArrayContext;
template<typename Parent> class AfterNestedContext;
class FinalContext;

class Builder {
public:
    Builder() = default;

    template<typename T>
    FinalContext Value(T&& value);

    DictContext<FinalContext> StartDict();
    ArrayContext<FinalContext> StartArray();
    Node Build() const;

private:
    struct Context {
        enum Type { ARRAY, DICT } type;
        Node* container;
        std::optional<std::string> pending_key;
    };
    std::optional<Node> root_;
    std::vector<Context> stack_;

    void DoKey(std::string key);
    void DoStartDict();
    void DoStartArray();
    void DoEndDict();
    void DoEndArray();
    Node* AddNode(Node node);
    Node DoBuild() const;

    template<typename T>
    void DoValue(T&& node) {
        AddNode(Node(std::forward<T>(node)));
    }

    template<typename> friend class DictContext;
    template<typename> friend class KeyContext;
    template<typename> friend class ValueAfterKeyContext;
    template<typename> friend class ArrayContext;
    template<typename> friend class AfterNestedContext;
    friend class FinalContext;
};

template<typename Parent>
class DictContext {
public:
    explicit DictContext(Builder& builder) : builder_(builder) {}

    KeyContext<Parent> Key(std::string key) {
        builder_.DoKey(std::move(key));
        return KeyContext<Parent>(builder_);
    }

    Parent EndDict() {
        builder_.DoEndDict();
        return Parent(builder_);
    }

    Parent EndArray() {
        builder_.DoEndArray();
        return Parent(builder_);
    }

private:
    Builder& builder_;
};

template<typename Parent>
class KeyContext {
public:
    explicit KeyContext(Builder& builder) : builder_(builder) {}

    template<typename T>
    ValueAfterKeyContext<Parent> Value(T&& value) {
        builder_.DoValue(std::forward<T>(value));
        return ValueAfterKeyContext<Parent>(builder_);
    }

    DictContext<AfterNestedContext<Parent>> StartDict() {
        builder_.DoStartDict();
        return DictContext<AfterNestedContext<Parent>>(builder_);
    }

    ArrayContext<AfterNestedContext<Parent>> StartArray() {
        builder_.DoStartArray();
        return ArrayContext<AfterNestedContext<Parent>>(builder_);
    }

private:
    Builder& builder_;
};

template<typename Parent>
class ValueAfterKeyContext {
public:
    explicit ValueAfterKeyContext(Builder& builder) : builder_(builder) {}

    KeyContext<Parent> Key(std::string key) {
        builder_.DoKey(std::move(key));
        return KeyContext<Parent>(builder_);
    }

    Parent EndDict() {
        builder_.DoEndDict();
        return Parent(builder_);
    }

    Parent EndArray() {
        builder_.DoEndArray();
        return Parent(builder_);
    }

private:
    Builder& builder_;
};

template<typename Parent>
class AfterNestedContext {
public:
    explicit AfterNestedContext(Builder& builder) : builder_(builder) {}

    KeyContext<Parent> Key(std::string key) {
        builder_.DoKey(std::move(key));
        return KeyContext<Parent>(builder_);
    }

    Parent EndDict() {
        builder_.DoEndDict();
        return Parent(builder_);
    }

    Parent EndArray() {
        builder_.DoEndArray();
        return Parent(builder_);
    }

    template<typename T>
    ValueAfterKeyContext<Parent> Value(T&& value) {
        builder_.DoValue(std::forward<T>(value));
        return ValueAfterKeyContext<Parent>(builder_);
    }

    DictContext<AfterNestedContext<Parent>> StartDict() {
        builder_.DoStartDict();
        return DictContext<AfterNestedContext<Parent>>(builder_);
    }

    ArrayContext<AfterNestedContext<Parent>> StartArray() {
        builder_.DoStartArray();
        return ArrayContext<AfterNestedContext<Parent>>(builder_);
    }

private:
    Builder& builder_;
};

template<typename Parent>
class ArrayContext {
public:
    explicit ArrayContext(Builder& builder) : builder_(builder) {}

    template<typename T>
    ArrayContext<Parent> Value(T&& value) {
        builder_.DoValue(std::forward<T>(value));
        return ArrayContext<Parent>(builder_);
    }

    DictContext<AfterNestedContext<Parent>> StartDict() {
        builder_.DoStartDict();
        return DictContext<AfterNestedContext<Parent>>(builder_);
    }

    ArrayContext<AfterNestedContext<Parent>> StartArray() {
        builder_.DoStartArray();
        return ArrayContext<AfterNestedContext<Parent>>(builder_);
    }

    Parent EndArray() {
        builder_.DoEndArray();
        return Parent(builder_);
    }

    Parent EndDict() {
        builder_.DoEndDict();
        return Parent(builder_);
    }

private:
    Builder& builder_;
};

class FinalContext {
public:
    explicit FinalContext(Builder& builder) : builder_(builder) {}

    Node Build() {
        return builder_.DoBuild();
    }

private:
    Builder& builder_;
};

template<typename T>
FinalContext Builder::Value(T&& value) {
    DoValue(std::forward<T>(value));
    return FinalContext(*this);
}

inline DictContext<FinalContext> Builder::StartDict() {
    DoStartDict();
    return DictContext<FinalContext>(*this);
}

inline ArrayContext<FinalContext> Builder::StartArray() {
    DoStartArray();
    return ArrayContext<FinalContext>(*this);
}

inline Node Builder::Build() const {
    return DoBuild();
}

} // namespace json