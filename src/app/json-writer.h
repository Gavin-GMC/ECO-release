//------------------------Description------------------------
// CLI 与 UI 共用的最小 JSON 写出器：流式生成对象/数组并统一处理字符串转义。

//-------------------------Reference-------------------------
// JSON 字符串与数字语法：RFC 8259。

// SPDX-License-Identifier: BSD-3-Clause

#pragma once
#include <cmath>
#include <iomanip>
#include <limits>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <vector>

namespace EvoFlow
{
    namespace App
    {
        class JsonWriter
        {
        private:
            enum class Container { object, array };
            struct Frame
            {
                Container kind;
                bool first = true;
                bool expectsValue = false;
            };

            std::ostream&      _out;
            std::vector<Frame> _stack;
            bool               _rootWritten = false;

            static void writeEscaped(std::ostream& out, const std::string& value)
            {
                static const char* hex = "0123456789abcdef";
                out.put('"');
                for (unsigned char ch : value)
                {
                    switch (ch)
                    {
                    case '"':  out << "\\\""; break;
                    case '\\': out << "\\\\"; break;
                    case '\b': out << "\\b";  break;
                    case '\f': out << "\\f";  break;
                    case '\n': out << "\\n";  break;
                    case '\r': out << "\\r";  break;
                    case '\t': out << "\\t";  break;
                    default:
                        if (ch < 0x20)
                        {
                            out << "\\u00" << hex[(ch >> 4) & 0x0f] << hex[ch & 0x0f];
                        }
                        else out.put(static_cast<char>(ch));
                    }
                }
                out.put('"');
            }

            void beforeValue()
            {
                if (_stack.empty())
                {
                    if (_rootWritten) throw std::logic_error("JsonWriter: multiple root values");
                    _rootWritten = true;
                    return;
                }

                Frame& frame = _stack.back();
                if (frame.kind == Container::array)
                {
                    if (!frame.first) _out.put(',');
                    frame.first = false;
                    return;
                }
                if (!frame.expectsValue)
                    throw std::logic_error("JsonWriter: object value without key");
                frame.expectsValue = false;
            }

        public:
            explicit JsonWriter(std::ostream& out) : _out(out) {}

            JsonWriter& beginObject()
            {
                beforeValue();
                _out.put('{');
                _stack.push_back({ Container::object, true, false });
                return *this;
            }

            JsonWriter& endObject()
            {
                if (_stack.empty() || _stack.back().kind != Container::object
                    || _stack.back().expectsValue)
                    throw std::logic_error("JsonWriter: mismatched object end");
                _stack.pop_back();
                _out.put('}');
                return *this;
            }

            JsonWriter& beginArray()
            {
                beforeValue();
                _out.put('[');
                _stack.push_back({ Container::array, true, false });
                return *this;
            }

            JsonWriter& endArray()
            {
                if (_stack.empty() || _stack.back().kind != Container::array)
                    throw std::logic_error("JsonWriter: mismatched array end");
                _stack.pop_back();
                _out.put(']');
                return *this;
            }

            JsonWriter& key(const std::string& name)
            {
                if (_stack.empty() || _stack.back().kind != Container::object
                    || _stack.back().expectsValue)
                    throw std::logic_error("JsonWriter: key outside object");
                Frame& frame = _stack.back();
                if (!frame.first) _out.put(',');
                frame.first = false;
                writeEscaped(_out, name);
                _out.put(':');
                frame.expectsValue = true;
                return *this;
            }

            JsonWriter& value(const std::string& value)
            {
                beforeValue();
                writeEscaped(_out, value);
                return *this;
            }

            JsonWriter& value(const char* value)
            {
                return value ? this->value(std::string(value)) : nullValue();
            }

            JsonWriter& value(bool value)
            {
                beforeValue();
                _out << (value ? "true" : "false");
                return *this;
            }

            template<typename T,
                     typename std::enable_if<std::is_integral<T>::value
                                             && !std::is_same<T, bool>::value, int>::type = 0>
            JsonWriter& value(T value)
            {
                beforeValue();
                _out << value;
                return *this;
            }

            JsonWriter& value(double value)
            {
                if (!std::isfinite(value)) return nullValue();
                beforeValue();
                std::ostringstream number;
                number << std::setprecision(std::numeric_limits<double>::max_digits10) << value;
                _out << number.str();
                return *this;
            }

            JsonWriter& nullValue()
            {
                beforeValue();
                _out << "null";
                return *this;
            }
        };
    }
}
