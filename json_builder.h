#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>

#include "json.h"

namespace json {

class BaseContext;
class DictItemContext;
class DictValueContext;
class ArrayItemContext;

class Builder {
public:
    Builder() = default;

public:
    DictItemContext StartDict();
    BaseContext EndDict();
    ArrayItemContext StartArray();
    BaseContext EndArray();
    DictValueContext Key(const std::string& key);
    BaseContext Value(const Node::Value& val);
    Node Build();

private:
    bool IsAdd() const;
    bool IsNullNode() const;
    bool IsKey() const;
    bool IsValue() const;
    bool IsStartDict() const;
    bool IsEndDict() const;
    bool IsStartArray() const;
    bool IsEndArray() const;
    bool IsBuild() const;
    void AddNode(Node node);

private:
    std::optional<Node> root_;
    std::vector<std::unique_ptr<Node>> nodes_stack_;
};

class BaseContext {
public:
    BaseContext(Builder& builder);
    DictItemContext StartDict();
    BaseContext EndDict();
    ArrayItemContext StartArray();
    BaseContext EndArray();
    DictValueContext Key(const std::string& key);
    BaseContext Value(const Node::Value& val);
    Node Build();

private:
    Builder& builder_;
};

class DictItemContext : public BaseContext {
public:
    DictItemContext(Builder& builder);
    DictItemContext(BaseContext context);

public:
    DictValueContext Key(const std::string& key);
    BaseContext EndDict();

    DictItemContext StartDict() = delete;
    ArrayItemContext StartArray() = delete;
    BaseContext EndArray() = delete;
    BaseContext Value(const Node::Value& val) = delete;
    Node Build() = delete;
};

class DictValueContext : public BaseContext {
public:
    DictValueContext(Builder& builder);

public:
    DictItemContext Value(const Node::Value& val);
    DictItemContext StartDict();
    ArrayItemContext StartArray();

    BaseContext EndDict() = delete;
    BaseContext EndArray() = delete;
    DictValueContext Key(const std::string& key) = delete;
    Node Build() = delete;
};

class ArrayItemContext : public BaseContext {
public:
    ArrayItemContext(Builder& builder);
    ArrayItemContext(BaseContext context);

public:
    ArrayItemContext Value(const Node::Value& val);
    DictItemContext StartDict();
    ArrayItemContext StartArray();
    BaseContext EndArray();

    BaseContext EndDict() = delete;
    DictValueContext Key(const std::string& key) = delete;
    Node Build() = delete;
};

} // namespace json