// Copyright (c) 2023, QuantStack and Mamba Contributors
//
// Distributed under the terms of the BSD 3-Clause License.
//
// The full license is in the file LICENSE, distributed with this software.

#ifndef MAMBA_UTIL_FLAT_EXPR_TREE_HPP
#define MAMBA_UTIL_FLAT_EXPR_TREE_HPP

#include <cassert>
#include <functional>
#include <utility>
#include <variant>
#include <vector>

#include "mamba/util/flat_binary_tree.hpp"

namespace mamba::util
{
    /**
     * A parser for postfix expressions.
     *
     * The parser creates an expression tree and validate that the expression being pushed
     * is a valid postfix expression.
     * For example, for the expression ``a + b * c`` on might push ``a b c * +``
     * or ``b c * a +``.
     */
    template <typename Variable, typename Operator>
    class PostfixParser
    {
    public:

        using operator_type = Operator;
        using variable_type = Variable;
        using tree_type = flat_binary_tree<operator_type, variable_type>;

        [[nodiscard]] auto push_variable(const variable_type& var) -> bool;
        [[nodiscard]] auto push_variable(variable_type&& var) -> bool;
        [[nodiscard]] auto push_operator(const operator_type& op) -> bool;
        [[nodiscard]] auto push_operator(operator_type&& op) -> bool;
        [[nodiscard]] auto finalize() -> bool;

        [[nodiscard]] auto tree() const& -> const tree_type&;
        [[nodiscard]] auto tree() && -> tree_type&&;

    private:

        using idx_type = typename tree_type::idx_type;
        using node_idx_stack = std::vector<idx_type>;

        /** The expression tree containing the expression being parsed. */
        tree_type m_tree = {};
        /** Orphan nodes are node without a parent. */
        node_idx_stack m_orphans = {};

        void orphans_push(idx_type idx);
        auto orphans_pop() -> idx_type;

        template <typename V>
        [[nodiscard]] auto push_variable_impl(V&& var) -> bool;
        template <typename O>
        [[nodiscard]] auto push_operator_impl(O&& op) -> bool;
    };

    /**
     * A parser for infix expressions.
     *
     * The parser creates an expression tree and validate that the expression being pushed
     * is a valid infix expression.
     * For example, the expression ``a + b * c`` can be pushed directly (thanks to the
     * operator precedence), or parenthesised as ``a + (b * c)``.
     */
    template <typename Variable, typename Operator, typename OperatorCmp = std::less<>>
    class InfixParser
    {
    public:

        using operator_type = Operator;
        using variable_type = Variable;
        using tree_type = flat_binary_tree<operator_type, variable_type>;
        using operator_precedence_type = OperatorCmp;

        InfixParser(const operator_precedence_type& cmp);
        InfixParser(operator_precedence_type&& cmp = {});

        [[nodiscard]] auto push_variable(const variable_type& var) -> bool;
        [[nodiscard]] auto push_variable(variable_type&& var) -> bool;
        [[nodiscard]] auto push_operator(const operator_type& op) -> bool;
        [[nodiscard]] auto push_operator(operator_type&& op) -> bool;
        [[nodiscard]] auto push_left_parenthesis() -> bool;
        [[nodiscard]] auto push_right_parenthesis() -> bool;
        [[nodiscard]] auto finalize() -> bool;

        [[nodiscard]] auto tree() const& -> const tree_type&;
        [[nodiscard]] auto tree() && -> tree_type&&;

    private:

        using postfix_parser_type = PostfixParser<variable_type, operator_type>;

        struct LeftParenthesis
        {
        };

        using operator_or_parenthesis_type = std::variant<operator_type, LeftParenthesis>;
        using operator_stack_type = std::vector<operator_or_parenthesis_type>;

        postfix_parser_type m_postfix_parser = {};
        operator_stack_type m_op_stack = {};
        std::size_t m_parenthesis_level = 0;
        bool m_expects_op = false;
        operator_precedence_type m_op_cmp = {};

        template <typename T>
        void stack_push(T&& elem);
        auto stack_pop() -> operator_or_parenthesis_type;
        [[nodiscard]] auto stack_empty() const -> bool;
        auto stack_top() const -> const operator_or_parenthesis_type&;
        [[nodiscard]] auto stack_top_is_parenthesis() const -> bool;
        auto stack_top_is_op_with_greater_precedence_than(const operator_type&) const -> bool;

        template <typename V>
        [[nodiscard]] auto push_variable_impl(V&& var) -> bool;
        template <typename O>
        [[nodiscard]] auto push_operator_impl(O&& op) -> bool;
    };

    enum struct BoolOperator
    {
        logical_and,
        logical_or
    };

    template <typename Variable>
    class flat_bool_expr_tree
    {
    public:

        using self_type = flat_bool_expr_tree<Variable>;
        using operator_type = BoolOperator;
        using variable_type = Variable;
        using tree_type = flat_binary_tree<operator_type, variable_type>;
        using size_type = typename tree_type::size_type;

        struct LeftParenthesis
        {
        };

        struct RightParenthesis
        {
        };

        flat_bool_expr_tree() = default;
        flat_bool_expr_tree(const flat_bool_expr_tree&) = default;
        flat_bool_expr_tree(flat_bool_expr_tree&&) = default;
        flat_bool_expr_tree(const tree_type& tree);
        flat_bool_expr_tree(tree_type&& tree);

        auto operator=(const flat_bool_expr_tree&) -> flat_bool_expr_tree& = default;
        auto operator=(flat_bool_expr_tree&&) -> flat_bool_expr_tree& = default;

        [[nodiscard]] auto size() const -> size_type;
        [[nodiscard]] auto empty() const -> bool;

        void clear();
        void reserve(size_type size);

        template <typename UnaryFunc = std::identity>
        [[nodiscard]] auto evaluate(UnaryFunc&& var_evaluator = {}, bool empty_val = true) const
            -> bool;

        template <typename UnaryFunc>
        void infix_for_each(UnaryFunc&& func) const;

        // TODO(C++20): replace by the `= default` implementation of `operator==`
        [[nodiscard]] auto operator==(const self_type& other) const -> bool
        {
            return m_tree == other.m_tree;
        }

        [[nodiscard]] auto operator!=(const self_type& other) const -> bool
        {
            return !(*this == other);
        }

    private:

        using idx_type = typename tree_type::idx_type;

        template <typename UnaryFunc>
        auto evaluate_impl(UnaryFunc& var_evaluator, idx_type idx) const -> bool;

        tree_type m_tree = {};
    };

    template <typename V>
    constexpr auto operator==(
        typename flat_bool_expr_tree<V>::LeftParenthesis,
        typename flat_bool_expr_tree<V>::LeftParenthesis
    ) -> bool
    {
        return true;
    }

    template <typename V>
    constexpr auto operator!=(
        typename flat_bool_expr_tree<V>::LeftParenthesis,
        typename flat_bool_expr_tree<V>::LeftParenthesis
    ) -> bool
    {
        return false;
    }

    template <typename V>
    constexpr auto operator==(
        typename flat_bool_expr_tree<V>::RightParenthesis,
        typename flat_bool_expr_tree<V>::RightParenthesis
    ) -> bool
    {
        return true;
    }

    template <typename V>
    constexpr auto operator!=(
        typename flat_bool_expr_tree<V>::RightParenthesis,
        typename flat_bool_expr_tree<V>::RightParenthesis
    ) -> bool
    {
        return false;
    }

    /*************************************
     *  Implementation of PostfixParser  *
     *************************************/

    template <typename V, typename O>
    void PostfixParser<V, O>::orphans_push(idx_type idx)
    {
        return m_orphans.push_back(idx);
    }

    template <typename V, typename O>
    auto PostfixParser<V, O>::orphans_pop() -> idx_type
    {
        assert(!m_orphans.empty());
        auto out = m_orphans.back();
        m_orphans.pop_back();
        return out;
    }

    template <typename V, typename O>
    template <typename Var>
    auto PostfixParser<V, O>::push_variable_impl(Var&& var) -> bool
    {
        orphans_push(m_tree.add_leaf(std::forward<Var>(var)));
        return true;  //  Always valid
    }

    template <typename V, typename O>
    auto PostfixParser<V, O>::push_variable(const variable_type& var) -> bool
    {
        return push_variable_impl(var);
    }

    template <typename V, typename O>
    auto PostfixParser<V, O>::push_variable(variable_type&& var) -> bool
    {
        return push_variable_impl(std::move(var));
    }

    template <typename V, typename O>
    template <typename Op>
    auto PostfixParser<V, O>::push_operator_impl(Op&& op) -> bool
    {
        if (m_orphans.size() < 2)
        {
            return false;
        }
        const auto right = orphans_pop();
        const auto left = orphans_pop();
        orphans_push(m_tree.add_branch(std::forward<Op>(op), left, right));
        return true;
    }

    template <typename V, typename O>
    auto PostfixParser<V, O>::push_operator(const operator_type& op) -> bool
    {
        return push_operator_impl(op);
    }

    template <typename V, typename O>
    auto PostfixParser<V, O>::push_operator(operator_type&& op) -> bool
    {
        return push_operator_impl(std::move(op));
    }

    template <typename V, typename O>
    auto PostfixParser<V, O>::finalize() -> bool
    {
        if (((m_orphans.size() == 1) && !m_tree.empty()) || (m_orphans.empty() && m_tree.empty()))
        {
            return true;
        }
        return false;  // Incomplete expression
    }

    template <typename V, typename O>
    auto PostfixParser<V, O>::tree() const& -> const tree_type&
    {
        return m_tree;
    }

    template <typename V, typename O>
    auto PostfixParser<V, O>::tree() && -> tree_type&&
    {
        return std::move(m_tree);
    }

    /***********************************
     *  Implementation of InfixParser  *
     ***********************************/

    template <typename V, typename O, typename C>
    InfixParser<V, O, C>::InfixParser(const operator_precedence_type& cmp)
        : m_op_cmp(cmp)
    {
    }

    template <typename V, typename O, typename C>
    InfixParser<V, O, C>::InfixParser(operator_precedence_type&& cmp)
        : m_op_cmp(std::move(cmp))
    {
    }

    template <typename V, typename O, typename C>
    template <typename T>
    void InfixParser<V, O, C>::stack_push(T&& elem)
    {
        m_op_stack.push_back(std::forward<T>(elem));
    }

    template <typename V, typename O, typename C>
    auto InfixParser<V, O, C>::stack_pop() -> operator_or_parenthesis_type
    {
        assert(!stack_empty());
        auto top = stack_top();
        m_op_stack.pop_back();
        return top;
    }

    template <typename V, typename O, typename C>
    auto InfixParser<V, O, C>::stack_empty() const -> bool
    {
        return m_op_stack.empty();
    }

    template <typename V, typename O, typename C>
    auto InfixParser<V, O, C>::stack_top() const -> const operator_or_parenthesis_type&
    {
        assert(!stack_empty());
        return m_op_stack.back();
    }

    template <typename V, typename O, typename C>
    auto InfixParser<V, O, C>::stack_top_is_parenthesis() const -> bool
    {
        return (!stack_empty()) && std::holds_alternative<LeftParenthesis>(stack_top());
    }

    template <typename V, typename O, typename C>
    auto InfixParser<V, O, C>::stack_top_is_op_with_greater_precedence_than(const operator_type& op
    ) const -> bool
    {
        if (stack_empty())
        {
            return false;
        }
        if (const auto* const op_ptr = std::get_if<operator_type>(&stack_top()))
        {
            return m_op_cmp(op, *op_ptr);
        }
        return false;
    }

    template <typename V, typename O, typename C>
    template <typename Var>
    auto InfixParser<V, O, C>::push_variable_impl(Var&& var) -> bool
    {
        // Input check
        if (m_expects_op)
        {
            return false;  // Unexpected variable
        }
        m_expects_op = true;
        // Parsing
        return m_postfix_parser.push_variable(std::forward<Var>(var));
    }

    template <typename V, typename O, typename C>
    auto InfixParser<V, O, C>::push_variable(const variable_type& var) -> bool
    {
        return push_variable_impl(var);
    }

    template <typename V, typename O, typename C>
    auto InfixParser<V, O, C>::push_variable(variable_type&& var) -> bool
    {
        return push_variable_impl(std::move(var));
    }

    template <typename V, typename O, typename C>
    template <typename Op>
    auto InfixParser<V, O, C>::push_operator_impl(Op&& op) -> bool
    {
        // Input check
        if (!m_expects_op)
        {
            return false;
        }
        m_expects_op = false;
        // Parsing
        while (stack_top_is_op_with_greater_precedence_than(op))
        {
            bool pushed = m_postfix_parser.push_operator(std::get<operator_type>(stack_pop()));
            if (!pushed)
            {
                return false;
            }
        }
        stack_push(std::forward<Op>(op));
        return true;
    }

    template <typename V, typename O, typename C>
    auto InfixParser<V, O, C>::push_operator(const operator_type& op) -> bool
    {
        return push_operator_impl(op);
    }

    template <typename V, typename O, typename C>
    auto InfixParser<V, O, C>::push_operator(operator_type&& op) -> bool
    {
        return push_operator_impl(std::move(op));
    }

    template <typename V, typename O, typename C>
    auto InfixParser<V, O, C>::push_left_parenthesis() -> bool
    {
        // Input check
        if (m_expects_op)
        {
            return false;  // Unexpected left parenthesis
        }
        ++m_parenthesis_level;
        // Parsing
        stack_push(LeftParenthesis{});
        return true;
    }

    template <typename V, typename O, typename C>
    auto InfixParser<V, O, C>::push_right_parenthesis() -> bool
    {
        // Input check
        if (!m_expects_op || (m_parenthesis_level == 0))
        {
            return false;  // Unexpected right parenthesis
        }
        --m_parenthesis_level;
        // Parsing
        while (!stack_top_is_parenthesis())
        {
            assert(!stack_empty());
            bool pushed = m_postfix_parser.push_operator(std::get<operator_type>(stack_pop()));
            if (!pushed)
            {
                return false;
            }
        }
        assert(stack_top_is_parenthesis());
        stack_pop();
        return true;
    }

    template <typename V, typename O, typename C>
    auto InfixParser<V, O, C>::finalize() -> bool
    {
        // Empty expression case
        if (m_postfix_parser.tree().empty() && stack_empty())
        {
            return true;
        }
        // Input check
        if (!m_expects_op || (m_parenthesis_level != 0))
        {
            return false;  // Invalid expression
        }
        // Parsing
        while (!stack_empty())
        {
            assert(!stack_top_is_parenthesis());
            bool pushed = m_postfix_parser.push_operator(std::get<operator_type>(stack_pop()));
            if (!pushed)
            {
                return false;
            }
        }
        return m_postfix_parser.finalize();
    }

    template <typename V, typename O, typename C>
    auto InfixParser<V, O, C>::tree() const& -> const tree_type&
    {
        return m_postfix_parser.tree();
    }

    template <typename V, typename O, typename C>
    auto InfixParser<V, O, C>::tree() && -> tree_type&&
    {
        return std::move(m_postfix_parser).tree();
    }

    /*******************************************
     *  Implementation of flat_bool_expr_tree  *
     *******************************************/

    template <typename V>
    flat_bool_expr_tree<V>::flat_bool_expr_tree(const tree_type& tree)
        : m_tree(tree)
    {
    }

    template <typename V>
    flat_bool_expr_tree<V>::flat_bool_expr_tree(tree_type&& tree)
        : m_tree(std::move(tree))
    {
    }

    template <typename V>
    auto flat_bool_expr_tree<V>::size() const -> size_type
    {
        return m_tree.size();
    }

    template <typename V>
    auto flat_bool_expr_tree<V>::empty() const -> bool
    {
        return m_tree.empty();
    }

    template <typename V>
    void flat_bool_expr_tree<V>::clear()
    {
        return m_tree.clear();
    }

    template <typename V>
    void flat_bool_expr_tree<V>::reserve(size_type size)
    {
        return m_tree.reserve(size);
    }

    template <typename V>
    template <typename UnaryFunc>
    auto flat_bool_expr_tree<V>::evaluate(UnaryFunc&& var_evaluator, bool empty_val) const -> bool
    {
        if (m_tree.empty())
        {
            return empty_val;
        }
        return evaluate_impl(var_evaluator, m_tree.root());
    }

    template <typename V>
    template <typename UnaryFunc>
    auto flat_bool_expr_tree<V>::evaluate_impl(UnaryFunc& var_eval, idx_type idx) const -> bool
    {
        // We do a tree evaluation rather than a stack-based postfix evaluation to
        // avoid evaluation sub trees thanks to operator && and || short circuiting.
        assert(idx < m_tree.size());
        if (m_tree.is_leaf(idx))
        {
            return var_eval(m_tree.leaf(idx));
        }
        if ((m_tree.branch(idx) == BoolOperator::logical_and))
        {
            return evaluate_impl(var_eval, m_tree.left(idx))
                   && evaluate_impl(var_eval, m_tree.right(idx));
        }
        else  // BoolOperator::logical_or
        {
            return evaluate_impl(var_eval, m_tree.left(idx))
                   || evaluate_impl(var_eval, m_tree.right(idx));
        }
    }

    template <typename V>
    template <typename UnaryFunc>
    void flat_bool_expr_tree<V>::infix_for_each(UnaryFunc&& func) const
    {
        struct TreeVisitor
        {
            using idx_type = typename tree_type::idx_type;

            void on_leaf(const tree_type& tree, idx_type idx)
            {
                m_func(tree.leaf(idx));
            }

            void on_branch_left_before(const tree_type& tree, idx_type, idx_type left_idx)
            {
                if (!tree.is_leaf(left_idx))
                {
                    m_func(LeftParenthesis{});
                }
            }

            void
            on_branch_infix(const tree_type& tree, idx_type branch_idx, idx_type left_idx, idx_type right_idx)
            {
                if (!tree.is_leaf(left_idx))
                {
                    m_func(RightParenthesis{});
                }
                m_func(tree.branch(branch_idx));
                if (!tree.is_leaf(right_idx))
                {
                    m_func(LeftParenthesis{});
                }
            }

            void on_branch_right_after(const tree_type& tree, idx_type, idx_type right_idx)
            {
                if (!tree.is_leaf(right_idx))
                {
                    m_func(RightParenthesis{});
                }
            }

            UnaryFunc m_func;
        } tree_visitor{ std::forward<UnaryFunc>(func) };

        m_tree.dfs_raw(tree_visitor, m_tree.root());
    }
}
#endif
