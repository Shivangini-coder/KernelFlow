// ================================================================================
// mini_json.hpp
// ================================================================================
// WHY THIS FILE EXISTS:
// Every "bridge" program in this folder needs to read a JSON request from
// stdin (sent by FastAPI) and turn it into C++ data (vector<ProcessInput>,
// an int time quantum, etc). Rather than pull in a third-party JSON library
// (which would mean asking you to install/vendor something extra just to
// build this student project), this file implements a SMALL, GENERIC JSON
// reader by hand. It only needs to handle what our own backend ever sends:
// numbers, strings, booleans, arrays, and objects. It is intentionally NOT
// a full/robust JSON parser — it trusts that FastAPI is the one producing
// the input, so it does not need to defend against malformed JSON from a
// random source.
//
// HOW IT'S USED:
// Each bridge does:
//   JsonValue root = JsonValue::parse(entireStdinString);
//   int quantum = (int)root["timeQuantum"].asNumber();
//   for (auto& p : root["processes"].asArray()) { ... }
//
// WRITING JSON BACK OUT (the response to FastAPI) is done directly with
// std::ostringstream in each bridge's main(), NOT through this file — the
// output shape is fixed and small, so hand-writing it with << is simpler
// to read than building a generic JSON writer.
// ================================================================================

#pragma once
#include <string>
#include <vector>
#include <map>
#include <stdexcept>
#include <cctype>

// A single parsed JSON value. `type` tells you which of the fields below
// is actually meaningful — this mimics a "tagged union" using a plain
// struct, which is easier for a beginner to read than std::variant.
struct JsonValue {
    enum class Type { Null, Bool, Number, String, Array, Object };

    Type type = Type::Null;
    bool boolValue = false;
    double numberValue = 0.0;
    std::string stringValue;
    std::vector<JsonValue> arrayValue;
    std::map<std::string, JsonValue> objectValue;

    double asNumber() const { return numberValue; }
    int asInt() const { return (int)numberValue; }
    const std::string& asString() const { return stringValue; }
    const std::vector<JsonValue>& asArray() const { return arrayValue; }
    bool has(const std::string& key) const { return objectValue.count(key) > 0; }

    // operator[] lets bridge code read fields with obj["key"] syntax,
    // just like real JSON libraries. Returns a Null JsonValue if the key
    // is missing (rather than throwing), so optional fields are easy to
    // check with .has(...) first.
    const JsonValue& operator[](const std::string& key) const {
        static JsonValue nullValue;
        auto it = objectValue.find(key);
        if (it == objectValue.end()) return nullValue;
        return it->second;
    }

    // Entry point: parses a full JSON document from a string.
    static JsonValue parse(const std::string& text) {
        size_t pos = 0;
        JsonValue value = parseValue(text, pos);
        return value;
    }

private:
    static void skipWhitespace(const std::string& text, size_t& pos) {
        while (pos < text.size() && std::isspace((unsigned char)text[pos])) pos++;
    }

    static JsonValue parseValue(const std::string& text, size_t& pos) {
        skipWhitespace(text, pos);
        if (pos >= text.size()) throw std::runtime_error("Unexpected end of JSON input");

        char c = text[pos];
        if (c == '{') return parseObject(text, pos);
        if (c == '[') return parseArray(text, pos);
        if (c == '"') return parseString(text, pos);
        if (c == 't' || c == 'f') return parseBool(text, pos);
        if (c == 'n') { pos += 4; return JsonValue(); } // "null"
        return parseNumber(text, pos);
    }

    static JsonValue parseObject(const std::string& text, size_t& pos) {
        JsonValue value;
        value.type = JsonValue::Type::Object;
        pos++; // consume '{'
        skipWhitespace(text, pos);
        if (pos < text.size() && text[pos] == '}') { pos++; return value; }

        while (true) {
            skipWhitespace(text, pos);
            JsonValue key = parseString(text, pos);
            skipWhitespace(text, pos);
            pos++; // consume ':'
            JsonValue val = parseValue(text, pos);
            value.objectValue[key.stringValue] = val;
            skipWhitespace(text, pos);
            if (pos < text.size() && text[pos] == ',') { pos++; continue; }
            if (pos < text.size() && text[pos] == '}') { pos++; break; }
            throw std::runtime_error("Malformed JSON object");
        }
        return value;
    }

    static JsonValue parseArray(const std::string& text, size_t& pos) {
        JsonValue value;
        value.type = JsonValue::Type::Array;
        pos++; // consume '['
        skipWhitespace(text, pos);
        if (pos < text.size() && text[pos] == ']') { pos++; return value; }

        while (true) {
            JsonValue element = parseValue(text, pos);
            value.arrayValue.push_back(element);
            skipWhitespace(text, pos);
            if (pos < text.size() && text[pos] == ',') { pos++; continue; }
            if (pos < text.size() && text[pos] == ']') { pos++; break; }
            throw std::runtime_error("Malformed JSON array");
        }
        return value;
    }

    static JsonValue parseString(const std::string& text, size_t& pos) {
        JsonValue value;
        value.type = JsonValue::Type::String;
        pos++; // consume opening quote
        std::string result;
        while (pos < text.size() && text[pos] != '"') {
            if (text[pos] == '\\' && pos + 1 < text.size()) {
                pos++;
                char escaped = text[pos];
                if (escaped == 'n') result += '\n';
                else if (escaped == 't') result += '\t';
                else result += escaped;
            } else {
                result += text[pos];
            }
            pos++;
        }
        pos++; // consume closing quote
        value.stringValue = result;
        return value;
    }

    static JsonValue parseBool(const std::string& text, size_t& pos) {
        JsonValue value;
        value.type = JsonValue::Type::Bool;
        if (text.compare(pos, 4, "true") == 0) { value.boolValue = true; pos += 4; }
        else { value.boolValue = false; pos += 5; } // "false"
        return value;
    }

    static JsonValue parseNumber(const std::string& text, size_t& pos) {
        JsonValue value;
        value.type = JsonValue::Type::Number;
        size_t start = pos;
        while (pos < text.size() &&
               (std::isdigit((unsigned char)text[pos]) || text[pos] == '-' ||
                text[pos] == '+' || text[pos] == '.' || text[pos] == 'e' || text[pos] == 'E')) {
            pos++;
        }
        value.numberValue = std::stod(text.substr(start, pos - start));
        return value;
    }
};

// Reads ALL of stdin into a single string. Every bridge's main() calls this
// first, since FastAPI sends the whole JSON request body in one shot (no
// streaming needed — these simulations are tiny).
inline std::string readAllStdin() {
    std::string content, line;
    std::string all;
    char buffer[4096];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), stdin)) > 0) {
        all.append(buffer, bytesRead);
    }
    return all;
}
