// SPDX-License-Identifier: BSD-3-Clause

#pragma once

#include <cctype>
#include <cstddef>
#include <cstdlib>
#include <exception>
#include <string>
#include <utility>
#include <vector>

namespace EvoFlow
{
    namespace App
    {
        struct JsonValue
        {
            enum class Type { nullValue, boolean, number, string, array, object };

            Type type = Type::nullValue;
            bool booleanValue = false;
            std::string numberValue;
            std::string stringValue;
            std::vector<JsonValue> arrayValue;
            std::vector<std::pair<std::string, JsonValue>> objectValue;

            bool isNull() const { return type == Type::nullValue; }
            bool isBoolean() const { return type == Type::boolean; }
            bool isNumber() const { return type == Type::number; }
            bool isString() const { return type == Type::string; }
            bool isArray() const { return type == Type::array; }
            bool isObject() const { return type == Type::object; }

            const JsonValue* find(const std::string& name) const
            {
                if (!isObject()) return nullptr;
                for (const auto& item : objectValue)
                    if (item.first == name) return &item.second;
                return nullptr;
            }
        };

        struct JsonParseResult
        {
            bool ok = false;
            JsonValue value;
            std::string error;
        };

        namespace JsonReaderDetail
        {
            class Parser
            {
            public:
                explicit Parser(const std::string& text) : _text(text) {}

                bool parse(JsonValue& value)
                {
                    skipWhitespace();
                    if (!parseValue(value, 0)) return false;
                    skipWhitespace();
                    if (_position != _text.size())
                        return fail("unexpected trailing data");
                    return true;
                }

                std::string error() const
                {
                    return "JSON parse error at byte " + std::to_string(_errorPosition)
                        + ": " + _error;
                }

            private:
                const std::string& _text;
                std::size_t _position = 0;
                std::size_t _errorPosition = 0;
                std::string _error;

                bool fail(const std::string& message)
                {
                    if (_error.empty())
                    {
                        _errorPosition = _position;
                        _error = message;
                    }
                    return false;
                }

                void skipWhitespace()
                {
                    while (_position < _text.size())
                    {
                        const char ch = _text[_position];
                        if (ch != ' ' && ch != '\t' && ch != '\r' && ch != '\n') break;
                        _position++;
                    }
                }

                bool parseValue(JsonValue& value, unsigned depth)
                {
                    if (depth > 128) return fail("nesting is too deep");
                    if (_position >= _text.size()) return fail("expected a value");
                    switch (_text[_position])
                    {
                    case 'n': return parseLiteral("null", JsonValue::Type::nullValue, value);
                    case 't':
                        value.booleanValue = true;
                        return parseLiteral("true", JsonValue::Type::boolean, value);
                    case 'f':
                        value.booleanValue = false;
                        return parseLiteral("false", JsonValue::Type::boolean, value);
                    case '"':
                        value.type = JsonValue::Type::string;
                        return parseString(value.stringValue);
                    case '[': return parseArray(value, depth);
                    case '{': return parseObject(value, depth);
                    default:
                        if (_text[_position] == '-' || isDigit(_text[_position]))
                            return parseNumber(value);
                        return fail("unexpected token");
                    }
                }

                bool parseLiteral(const char* literal, JsonValue::Type type, JsonValue& value)
                {
                    const std::size_t length = std::char_traits<char>::length(literal);
                    if (_text.size() - _position < length
                        || _text.compare(_position, length, literal) != 0)
                        return fail("invalid literal");
                    _position += length;
                    value.type = type;
                    return true;
                }

                static bool isDigit(char ch)
                {
                    return ch >= '0' && ch <= '9';
                }

                bool parseNumber(JsonValue& value)
                {
                    const std::size_t start = _position;
                    if (_text[_position] == '-') _position++;
                    if (_position >= _text.size()) return fail("incomplete number");
                    if (_text[_position] == '0')
                    {
                        _position++;
                        if (_position < _text.size() && isDigit(_text[_position]))
                            return fail("leading zero in number");
                    }
                    else
                    {
                        if (_text[_position] < '1' || _text[_position] > '9')
                            return fail("invalid number");
                        while (_position < _text.size() && isDigit(_text[_position])) _position++;
                    }
                    if (_position < _text.size() && _text[_position] == '.')
                    {
                        _position++;
                        const std::size_t digits = _position;
                        while (_position < _text.size() && isDigit(_text[_position])) _position++;
                        if (_position == digits) return fail("fraction has no digits");
                    }
                    if (_position < _text.size()
                        && (_text[_position] == 'e' || _text[_position] == 'E'))
                    {
                        _position++;
                        if (_position < _text.size()
                            && (_text[_position] == '+' || _text[_position] == '-'))
                            _position++;
                        const std::size_t digits = _position;
                        while (_position < _text.size() && isDigit(_text[_position])) _position++;
                        if (_position == digits) return fail("exponent has no digits");
                    }
                    value.type = JsonValue::Type::number;
                    value.numberValue.assign(_text, start, _position - start);
                    return true;
                }

                static int hexValue(char ch)
                {
                    if (ch >= '0' && ch <= '9') return ch - '0';
                    if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
                    if (ch >= 'A' && ch <= 'F') return ch - 'A' + 10;
                    return -1;
                }

                bool parseHex(unsigned& value)
                {
                    if (_text.size() - _position < 4) return fail("incomplete Unicode escape");
                    value = 0;
                    for (int i = 0; i < 4; ++i)
                    {
                        const int digit = hexValue(_text[_position++]);
                        if (digit < 0) return fail("invalid Unicode escape");
                        value = value * 16u + static_cast<unsigned>(digit);
                    }
                    return true;
                }

                static void appendUtf8(std::string& output, unsigned codePoint)
                {
                    if (codePoint <= 0x7f)
                        output.push_back(static_cast<char>(codePoint));
                    else if (codePoint <= 0x7ff)
                    {
                        output.push_back(static_cast<char>(0xc0u | (codePoint >> 6)));
                        output.push_back(static_cast<char>(0x80u | (codePoint & 0x3fu)));
                    }
                    else if (codePoint <= 0xffff)
                    {
                        output.push_back(static_cast<char>(0xe0u | (codePoint >> 12)));
                        output.push_back(static_cast<char>(0x80u | ((codePoint >> 6) & 0x3fu)));
                        output.push_back(static_cast<char>(0x80u | (codePoint & 0x3fu)));
                    }
                    else
                    {
                        output.push_back(static_cast<char>(0xf0u | (codePoint >> 18)));
                        output.push_back(static_cast<char>(0x80u | ((codePoint >> 12) & 0x3fu)));
                        output.push_back(static_cast<char>(0x80u | ((codePoint >> 6) & 0x3fu)));
                        output.push_back(static_cast<char>(0x80u | (codePoint & 0x3fu)));
                    }
                }

                bool appendRawUtf8(std::string& output)
                {
                    const std::size_t start = _position;
                    const unsigned char first = static_cast<unsigned char>(_text[_position]);
                    unsigned length = 0;
                    unsigned codePoint = 0;
                    unsigned minimum = 0;
                    if (first >= 0xc2 && first <= 0xdf)
                    {
                        length = 2; codePoint = first & 0x1fu; minimum = 0x80;
                    }
                    else if (first >= 0xe0 && first <= 0xef)
                    {
                        length = 3; codePoint = first & 0x0fu; minimum = 0x800;
                    }
                    else if (first >= 0xf0 && first <= 0xf4)
                    {
                        length = 4; codePoint = first & 0x07u; minimum = 0x10000;
                    }
                    else return fail("invalid UTF-8 in string");

                    if (_text.size() - _position < length)
                        return fail("incomplete UTF-8 in string");
                    for (unsigned i = 1; i < length; ++i)
                    {
                        const unsigned char next =
                            static_cast<unsigned char>(_text[_position + i]);
                        if ((next & 0xc0u) != 0x80u)
                            return fail("invalid UTF-8 continuation byte");
                        codePoint = (codePoint << 6) | (next & 0x3fu);
                    }
                    if (codePoint < minimum || codePoint > 0x10ffff
                        || (codePoint >= 0xd800 && codePoint <= 0xdfff))
                        return fail("invalid UTF-8 code point");
                    output.append(_text, start, length);
                    _position += length;
                    return true;
                }

                bool parseString(std::string& output)
                {
                    if (_text[_position] != '"') return fail("expected a string");
                    _position++;
                    output.clear();
                    while (_position < _text.size())
                    {
                        const unsigned char ch =
                            static_cast<unsigned char>(_text[_position++]);
                        if (ch == '"') return true;
                        if (ch < 0x20) return fail("unescaped control character in string");
                        if (ch >= 0x80)
                        {
                            _position--;
                            if (!appendRawUtf8(output)) return false;
                            continue;
                        }
                        if (ch != '\\')
                        {
                            output.push_back(static_cast<char>(ch));
                            continue;
                        }
                        if (_position >= _text.size()) return fail("incomplete escape");
                        const char escaped = _text[_position++];
                        switch (escaped)
                        {
                        case '"': output.push_back('"'); break;
                        case '\\': output.push_back('\\'); break;
                        case '/': output.push_back('/'); break;
                        case 'b': output.push_back('\b'); break;
                        case 'f': output.push_back('\f'); break;
                        case 'n': output.push_back('\n'); break;
                        case 'r': output.push_back('\r'); break;
                        case 't': output.push_back('\t'); break;
                        case 'u':
                        {
                            unsigned codePoint = 0;
                            if (!parseHex(codePoint)) return false;
                            if (codePoint >= 0xd800 && codePoint <= 0xdbff)
                            {
                                if (_text.size() - _position < 2
                                    || _text[_position] != '\\'
                                    || _text[_position + 1] != 'u')
                                    return fail("high surrogate has no low surrogate");
                                _position += 2;
                                unsigned low = 0;
                                if (!parseHex(low)) return false;
                                if (low < 0xdc00 || low > 0xdfff)
                                    return fail("invalid low surrogate");
                                codePoint = 0x10000u
                                    + ((codePoint - 0xd800u) << 10)
                                    + (low - 0xdc00u);
                            }
                            else if (codePoint >= 0xdc00 && codePoint <= 0xdfff)
                                return fail("unexpected low surrogate");
                            appendUtf8(output, codePoint);
                            break;
                        }
                        default: return fail("unknown escape");
                        }
                    }
                    return fail("unterminated string");
                }

                bool parseArray(JsonValue& value, unsigned depth)
                {
                    _position++;
                    value.type = JsonValue::Type::array;
                    value.arrayValue.clear();
                    skipWhitespace();
                    if (_position < _text.size() && _text[_position] == ']')
                    {
                        _position++;
                        return true;
                    }
                    for (;;)
                    {
                        JsonValue item;
                        if (!parseValue(item, depth + 1)) return false;
                        value.arrayValue.push_back(std::move(item));
                        skipWhitespace();
                        if (_position >= _text.size()) return fail("unterminated array");
                        const char delimiter = _text[_position++];
                        if (delimiter == ']') return true;
                        if (delimiter != ',') return fail("expected ',' or ']'");
                        skipWhitespace();
                    }
                }

                bool parseObject(JsonValue& value, unsigned depth)
                {
                    _position++;
                    value.type = JsonValue::Type::object;
                    value.objectValue.clear();
                    skipWhitespace();
                    if (_position < _text.size() && _text[_position] == '}')
                    {
                        _position++;
                        return true;
                    }
                    for (;;)
                    {
                        if (_position >= _text.size() || _text[_position] != '"')
                            return fail("expected an object key");
                        std::string key;
                        if (!parseString(key)) return false;
                        for (const auto& item : value.objectValue)
                            if (item.first == key) return fail("duplicate object key");
                        skipWhitespace();
                        if (_position >= _text.size() || _text[_position] != ':')
                            return fail("expected ':' after object key");
                        _position++;
                        skipWhitespace();
                        JsonValue item;
                        if (!parseValue(item, depth + 1)) return false;
                        value.objectValue.emplace_back(std::move(key), std::move(item));
                        skipWhitespace();
                        if (_position >= _text.size()) return fail("unterminated object");
                        const char delimiter = _text[_position++];
                        if (delimiter == '}') return true;
                        if (delimiter != ',') return fail("expected ',' or '}'");
                        skipWhitespace();
                    }
                }
            };
        }

        inline JsonParseResult parseJson(const std::string& text)
        {
            JsonParseResult result;
            try
            {
                JsonReaderDetail::Parser parser(text);
                result.ok = parser.parse(result.value);
                if (!result.ok) result.error = parser.error();
            }
            catch (const std::exception&)
            {
                result.error = "JSON parse error: input could not be represented";
            }
            catch (...)
            {
                result.error = "JSON parse error: unexpected parser failure";
            }
            return result;
        }
    }
}
