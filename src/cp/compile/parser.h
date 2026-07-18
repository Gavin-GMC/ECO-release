#pragma once
//
// cp/compile/parser.h
//
// 递归下降解析器：直接把表达式发射成 Program 的后序扁平节点（无独立 AST）。
// 文法（优先级由低到高）：
//   relational := addsub ( (< <= > >= == !=) addsub )?
//   addsub     := term ( (+|-) term )*
//   term       := factor ( (*|/) factor )*
//   factor     := ('-'|'+') factor | number | identifier | '(' relational ')'
//
// 解析错误抛 parse_error（建立期，符合「运行期无异常」的分工）。
//
#include <string>
#include <cctype>
#include <utility>
#include <vector>
#include "cp/compile/program.h"

namespace ECFlow {

class Parser {
public:
    Parser(Emitter& em, const std::string& src) : em_(em), src_(src) {}

    // 解析完整输入，返回根节点；要求消费到末尾。
    NodeId parse_full() {
        NodeId r = parse_relational();
        skip_ws();
        if (pos_ != src_.size())
            throw parse_error("unexpected trailing characters at pos " + std::to_string(pos_));
        return r;
    }

private:
    Emitter&           em_;
    const std::string& src_;
    std::size_t        pos_ = 0;

    void skip_ws() { while (pos_ < src_.size() && std::isspace((unsigned char)src_[pos_])) ++pos_; }
    char peek()    { skip_ws(); return pos_ < src_.size() ? src_[pos_] : '\0'; }
    char peek2()   { return (pos_ + 1 < src_.size()) ? src_[pos_ + 1] : '\0'; }

    NodeId parse_relational() {
        NodeId lhs = parse_addsub();
        skip_ws();
        char c = peek();
        Op op = Op::Lt; bool has = true;
        if      (c == '<' && peek2() == '=') { op = Op::Le; pos_ += 2; }
        else if (c == '>' && peek2() == '=') { op = Op::Ge; pos_ += 2; }
        else if (c == '=' && peek2() == '=') { op = Op::Eq; pos_ += 2; }
        else if (c == '!' && peek2() == '=') { op = Op::Ne; pos_ += 2; }
        else if (c == '<')                   { op = Op::Lt; pos_ += 1; }
        else if (c == '>')                   { op = Op::Gt; pos_ += 1; }
        else has = false;
        if (!has) return lhs;
        NodeId rhs = parse_addsub();
        return em_.binary(op, lhs, rhs);
    }

    NodeId parse_addsub() {
        NodeId lhs = parse_term();
        for (;;) {
            char c = peek();
            if (c == '+')      { ++pos_; lhs = em_.binary(Op::Add, lhs, parse_term()); }
            else if (c == '-') { ++pos_; lhs = em_.binary(Op::Sub, lhs, parse_term()); }
            else break;
        }
        return lhs;
    }

    NodeId parse_term() {
        NodeId lhs = parse_factor();
        for (;;) {
            char c = peek();
            if (c == '*')      { ++pos_; lhs = em_.binary(Op::Mul, lhs, parse_factor()); }
            else if (c == '/') { ++pos_; lhs = em_.binary(Op::Div, lhs, parse_factor()); }
            else break;
        }
        return lhs;
    }

    NodeId parse_factor() {
        char c = peek();
        if (c == '-') { ++pos_; return em_.unary(Op::Neg, parse_factor()); }
        if (c == '+') { ++pos_; return parse_factor(); }
        if (c == '(') {
            ++pos_;
            NodeId e = parse_relational();
            if (peek() != ')') throw parse_error("missing ')'");
            ++pos_;
            return e;
        }
        if (std::isdigit((unsigned char)c) || c == '.') return parse_number();
        if (std::isalpha((unsigned char)c) || c == '_') return parse_identifier();
        throw parse_error(std::string("unexpected character '") + (c ? c : '?') + "'");
    }

    NodeId parse_number() {
        skip_ws();
        std::size_t start = pos_;
        while (pos_ < src_.size()) {
            char c = src_[pos_];
            if (std::isdigit((unsigned char)c) || c == '.') ++pos_;
            else if ((c == 'e' || c == 'E') &&
                     (std::isdigit((unsigned char)peek2()) || peek2() == '+' || peek2() == '-')) {
                pos_ += 2; // 吃掉 e 和符号/数字首位
            } else break;
        }
        try {
            return em_.konst(std::stod(src_.substr(start, pos_ - start)));
        } catch (...) {
            throw parse_error("invalid number near pos " + std::to_string(start));
        }
    }

    NodeId parse_identifier() {
        skip_ws();
        std::size_t start = pos_;
        while (pos_ < src_.size() &&
               (std::isalnum((unsigned char)src_[pos_]) || src_[pos_] == '_')) ++pos_;
        std::string base = src_.substr(start, pos_ - start);

        // 可选向量下标 name[const_int]
        if (peek() == '[') {
            ++pos_;                       // 吃 '['
            skip_ws();
            std::size_t ds = pos_;
            while (pos_ < src_.size() && std::isdigit((unsigned char)src_[pos_])) ++pos_;
            if (pos_ == ds) throw parse_error("expected integer index after '['");
            int index = std::stoi(src_.substr(ds, pos_ - ds));
            if (peek() != ']') throw parse_error("missing ']' in index");
            ++pos_;                       // 吃 ']'
            return em_.var(base, index);
        }
        return em_.var(base, 0);   // 裸变量 a 默认即 a[0]
    }
};

// —— 对外门面：搭建约束系统并编译 ——
class ConstraintSystemBuilder {
public:
    // 添加一条约束（根须为关系表达式）。weight 为违反度权重。可链式调用。
    ConstraintSystemBuilder& add(const std::string& expr, double weight = 1.0) {
        entries_.emplace_back(expr, weight);
        return *this;
    }

    Program compile() const {
        Emitter em;
        for (const auto& e : entries_) {
            Parser p(em, e.first);
            NodeId root = p.parse_full();
            if (!is_relational(em.nodes[static_cast<std::size_t>(root)].op))
                throw parse_error("constraint must be a relational expression: " + e.first);
            em.add_constraint(root, e.second);
        }
        return Program::from_emitter(std::move(em));
    }

private:
    std::vector<std::pair<std::string, double>> entries_;
};

} // namespace ECFlow
