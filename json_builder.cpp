#include "json_builder.h"

namespace json {
using namespace std::literals;

// ---------- Builder ------------------------------------------------------------

DictValueContext Builder::Key(const std::string& key) {
    if (!IsKey()) {
        throw std::logic_error("Key error"s);
    }
    nodes_stack_.push_back(std::make_unique<Node>(key));

    return *this;
}

BaseContext Builder::Value(const Node::Value& value) {
    if (!IsValue()) {
        throw std::logic_error("Value error"s);
    }
    visit([this](auto& val) {
        nodes_stack_.push_back(std::make_unique<Node>(val));
        }, value);
    AddNode(std::move(*nodes_stack_.back()));

    return *this;
}

DictItemContext Builder::StartDict() {
    if (!IsStartDict()) {
        throw std::logic_error("Starting dict error"s);
    }
    nodes_stack_.push_back(std::make_unique<Node>(Dict{}));

    return *this;
}

BaseContext Builder::EndDict() {
    if (!IsEndDict()) {
        throw std::logic_error("Ending dict error"s);
    }
    AddNode(std::move(*nodes_stack_.back()));

    return *this;
}

ArrayItemContext Builder::StartArray() {
    if (!IsStartArray()) {
        throw std::logic_error("Starting array error"s);
    }
    nodes_stack_.push_back(std::make_unique<Node>(Array{}));

    return *this;
}

BaseContext Builder::EndArray() {
    if (!IsEndArray()) {
        throw std::logic_error("Ending array error"s);
    }
    AddNode(std::move(*nodes_stack_.back()));

    return *this;
}

Node Builder::Build() {
    if (!IsBuild()) {
        throw std::logic_error("Build error"s);
    }

    return root_.value();
}

void Builder::AddNode(Node node) {
    nodes_stack_.pop_back();
    if (nodes_stack_.empty()) {
        root_ = std::move(node);
    }
    else if (nodes_stack_.back()->IsArray()) {
        Array arr = nodes_stack_.back()->AsArray();
        arr.push_back(std::move(node));
        nodes_stack_.back() = std::make_unique<Node>(std::move(arr));

    }
    else {
        const Node key(nodes_stack_.back()->AsString());
        nodes_stack_.pop_back();
        Dict dict = nodes_stack_.back()->AsDict();
        dict[key.AsString()] = std::move(node);
        nodes_stack_.back() = std::make_unique<Node>(std::move(dict));
    }
}

bool Builder::IsAdd() const {
    return (nodes_stack_.empty()
        || nodes_stack_.back()->IsArray()
        || nodes_stack_.back()->IsString());
}

bool Builder::IsNullNode() const {
    return root_.has_value();
}

bool Builder::IsKey() const {
    return !IsNullNode()
        && !nodes_stack_.empty()
        && nodes_stack_.back()->IsDict();
}

bool Builder::IsValue() const {
    return !IsNullNode()
        && IsAdd();
}

bool Builder::IsStartDict() const {
    return IsValue();
}

bool Builder::IsEndDict() const {
    return !IsNullNode()
        && !nodes_stack_.empty()
        && nodes_stack_.back()->IsDict();
}

bool Builder::IsStartArray() const {
    return IsValue();
}

bool Builder::IsEndArray() const {
    return !IsNullNode()
        && !nodes_stack_.empty()
        && nodes_stack_.back()->IsArray();
}

bool Builder::IsBuild() const {
    return IsNullNode();
}

// ---------- BaseContext ------------------------------------------------------------

BaseContext::BaseContext(Builder& builder) 
    : builder_(builder) {
}

DictItemContext BaseContext::StartDict() {
    return builder_.StartDict();
}

BaseContext BaseContext::EndDict() {
    return builder_.EndDict();
}

ArrayItemContext BaseContext::StartArray() {
    return builder_.StartArray();
}

BaseContext BaseContext::EndArray() {
    return builder_.EndArray();
}

DictValueContext BaseContext::Key(const std::string& key) {
    return builder_.Key(key);
}

BaseContext BaseContext::Value(const Node::Value& val) {
    return builder_.Value(val);
}

Node BaseContext::Build() {
    return builder_.Build();
}

// ---------- DictItemContext ------------------------------------------------------------

DictItemContext::DictItemContext(Builder& builder)
    : BaseContext(builder) {
}

DictItemContext::DictItemContext(BaseContext context)
    : BaseContext(context) {
}

DictValueContext DictItemContext::Key(const std::string& key) {
    return BaseContext::Key(key);
}

BaseContext DictItemContext::EndDict() {
    return BaseContext::EndDict();
}

// ---------- DictValueContext ------------------------------------------------------------

DictValueContext::DictValueContext(Builder& builder)
    : BaseContext(builder) {
}

DictItemContext DictValueContext::Value(const Node::Value& val) {
    return BaseContext::Value(val);
}

DictItemContext DictValueContext::StartDict() {
    return BaseContext::StartDict();
}

ArrayItemContext DictValueContext::StartArray() {
    return BaseContext::StartArray();
}

// ---------- ArrayItemContext ------------------------------------------------------------

ArrayItemContext::ArrayItemContext(Builder& builder)
    : BaseContext(builder) {
}

ArrayItemContext::ArrayItemContext(BaseContext context)
    : BaseContext(context) {
}

ArrayItemContext ArrayItemContext::Value(const Node::Value& val) {
    return BaseContext::Value(val);
}

DictItemContext ArrayItemContext::StartDict() {
    return BaseContext::StartDict();
}

ArrayItemContext ArrayItemContext::StartArray() {
    return BaseContext::StartArray();
}

BaseContext ArrayItemContext::EndArray() {
    return BaseContext::EndArray();
}

} // json