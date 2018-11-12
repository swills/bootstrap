// ----------------------------------------------------------------------------
//
// Basecode Bootstrap Compiler
// Copyright (C) 2018 Jeff Panici
// All rights reserved.
//
// This software source file is licensed under the terms of MIT license.
// For details, please read the LICENSE file.
//
// ----------------------------------------------------------------------------

#include <regex>
#include <string>
#include <iomanip>
#include <sstream>
#include <iostream>
#include <common/defer.h>
#include "parser.h"
#include "ast_formatter.h"

namespace basecode::syntax {

    ///////////////////////////////////////////////////////////////////////////

    static bool create_type_parameter_nodes(
            common::result& r,
            parser* parser,
            const ast_node_shared_ptr& type_parameter_list) {
        if (!parser->peek(token_types_t::less_than))
            return true;

        parser->consume();

        while (true) {
            auto type_parameter_node = parser->ast_builder()->type_parameter_node();
            type_parameter_node->rhs = parser->parse_expression(
                r,
                precedence_t::variable);

            if (parser->peek(token_types_t::colon)) {
                parser->consume();
                type_parameter_node->lhs = parser->expect_expression(
                    r,
                    ast_node_types_t::tuple_expression,
                    precedence_t::variable);
                if (r.is_failed())
                    return false;
            }

            type_parameter_list->children.push_back(type_parameter_node);

            if (!parser->peek(token_types_t::comma))
                break;

            parser->consume();
        }

        token_t greater_than;
        greater_than.type = token_types_t::greater_than;
        return parser->expect(r, greater_than);
    }

    ///////////////////////////////////////////////////////////////////////////

    static size_t collect_comments(
            common::result& r,
            parser* parser,
            ast_node_list& target) {
        size_t count = 0;

        while (parser->peek(token_types_t::line_comment)
            || parser->peek(token_types_t::block_comment)) {

            token_t token;
            if (!parser->consume(token))
                return count;

            ast_node_shared_ptr comment_node;
            switch (token.type) {
                case token_types_t::line_comment:
                    comment_node = parser->ast_builder()->line_comment_node(token);
                    break;
                case token_types_t::block_comment:
                    comment_node = parser->ast_builder()->block_comment_node(token);
                    break;
                default:
                    break;
            }

            target.push_back(comment_node);
            ++count;
        }

        return count;
    }

    ///////////////////////////////////////////////////////////////////////////

    static void pairs_to_list(
            const ast_node_shared_ptr& target,
            const ast_node_shared_ptr& root) {
        if (root == nullptr)
            return;

        if (root->type != ast_node_types_t::pair) {
            target->location = root->location;
            target->children.push_back(root);
            return;
        }

        auto current_pair = root;
        target->location.start(current_pair->location.start());
        while (true) {
            if (current_pair->lhs->type != ast_node_types_t::pair) {
                target->children.push_back(current_pair->lhs);
                target->children.push_back(current_pair->rhs);
                target->location.end(current_pair->rhs->location.end());
                break;
            }
            target->children.push_back(current_pair->rhs);
            current_pair = current_pair->lhs;
        }
    }

    ///////////////////////////////////////////////////////////////////////////

    static ast_node_shared_ptr create_module_expression_node(
            common::result& r,
            parser* parser,
            const ast_node_shared_ptr& lhs,
            token_t& token) {
        auto module_expression_node = parser
            ->ast_builder()
            ->module_expression_node(token);

        token_t left_paren;
        left_paren.type = token_types_t::left_paren;
        if (!parser->expect(r, left_paren))
            return nullptr;

        module_expression_node->rhs = parser->parse_expression(r);

        token_t right_paren;
        right_paren.type = token_types_t::right_paren;
        if (!parser->expect(r, right_paren))
            return nullptr;

        return module_expression_node;
    }

    ///////////////////////////////////////////////////////////////////////////

    static ast_node_shared_ptr create_symbol_node(
            common::result& r,
            parser* parser,
            const ast_node_shared_ptr& lhs,
            token_t& token) {
        auto symbol_node = parser
            ->ast_builder()
            ->symbol_node();
        symbol_node->location.start(token.location.start());

        while (true) {
            auto symbol_part_node = parser
                ->ast_builder()
                ->symbol_part_node(token);
            symbol_node->children.push_back(symbol_part_node);
            if (!parser->peek(token_types_t::scope_operator)) {
                symbol_node->location.end(token.location.end());
                break;
            }
            parser->consume();
            if (!parser->expect(r, token))
                return nullptr;
        }

        // XXX: this isn't quite correct
//        if (parser->peek(token_types_t::less_than)) {
//            symbol_node->lhs = parser->ast_builder()->type_list_node();
//
//            token_t less_than;
//            parser->consume(less_than);
//            symbol_node->lhs->location.start(less_than.location.start());
//
//            while (true) {
//                auto type_node = parser->parse_expression(
//                    r,
//                    precedence_t::variable);
//                symbol_node->lhs->children.push_back(type_node);
//
//                if (parser->peek(token_types_t::comma)) {
//                    parser->consume();
//                } else {
//                    token_t greater_than;
//                    greater_than.type = token_types_t::greater_than;
//                    if (!parser->expect(r, greater_than))
//                        return nullptr;
//                    symbol_node->lhs->location.end(greater_than.location.end());
//                    break;
//                }
//            }
//        }

        if (lhs != nullptr
        &&  (lhs->token.is_block_comment() || lhs->token.is_line_comment())) {
            symbol_node->children.push_back(lhs);
        }

        return symbol_node;
    }

    ///////////////////////////////////////////////////////////////////////////

    static ast_node_shared_ptr create_expression_node(
            common::result& r,
            parser* parser,
            const ast_node_shared_ptr& lhs,
            token_t& token) {
        auto node = parser->ast_builder()->expression_node();
        node->lhs = parser->parse_expression(r);

        token_t right_paren_token;
        right_paren_token.type = token_types_t::right_paren;
        if (!parser->expect(r, right_paren_token))
            return nullptr;

        return node;
    }

    ///////////////////////////////////////////////////////////////////////////

    static ast_node_shared_ptr create_cast_node(
            common::result& r,
            parser* parser,
            token_t& token) {
        auto cast_node = parser
            ->ast_builder()
            ->cast_node(token);

        token_t less_than;
        less_than.type = token_types_t::less_than;
        if (!parser->expect(r, less_than))
            return nullptr;

// XXX: fix this!
//        cast_node->lhs = create_type_identifier_node(r, parser, less_than);

        token_t greater_than;
        greater_than.type = token_types_t::greater_than;
        if (!parser->expect(r, greater_than))
            return nullptr;

        token_t left_paren;
        left_paren.type = token_types_t::left_paren;
        if (!parser->expect(r, left_paren))
            return nullptr;

        cast_node->rhs = parser->parse_expression(r);

        token_t right_paren;
        right_paren.type = token_types_t::right_paren;
        if (!parser->expect(r, right_paren))
            return nullptr;

        return cast_node;
    }

    ///////////////////////////////////////////////////////////////////////////

    static ast_node_shared_ptr create_transmute_node(
            common::result& r,
            parser* parser,
            token_t& token) {
        auto transmute_node = parser
            ->ast_builder()
            ->transmute_node(token);

        token_t less_than;
        less_than.type = token_types_t::less_than;
        if (!parser->expect(r, less_than))
            return nullptr;

        // XXX: fix this!
        //transmute_node->lhs = create_type_identifier_node(r, parser, less_than);

        token_t greater_than;
        greater_than.type = token_types_t::greater_than;
        if (!parser->expect(r, greater_than))
            return nullptr;

        token_t left_paren;
        left_paren.type = token_types_t::left_paren;
        if (!parser->expect(r, left_paren))
            return nullptr;

        transmute_node->rhs = parser->parse_expression(r);

        token_t right_paren;
        right_paren.type = token_types_t::right_paren;
        if (!parser->expect(r, right_paren))
            return nullptr;

        return transmute_node;
    }

    ///////////////////////////////////////////////////////////////////////////

    static ast_node_shared_ptr create_assignment_node(
            common::result& r,
            ast_node_types_t type,
            parser* parser,
            const ast_node_shared_ptr& lhs,
            token_t& token) {
        ast_node_shared_ptr assignment_node;
        if (type == ast_node_types_t::assignment)
            assignment_node = parser->ast_builder()->assignment_node();
        else if (type == ast_node_types_t::constant_assignment)
            assignment_node = parser->ast_builder()->constant_assignment_node();

        pairs_to_list(assignment_node->lhs, lhs);

        collect_comments(r, parser, assignment_node->comments);

        auto rhs = parser->parse_expression(
            r,
            parser->assignment_precedence());
        if (rhs == nullptr) {
            parser->error(
                r,
                "P019",
                "assignment expects right-hand-side expression",
                token.location);
            return nullptr;
        }
        pairs_to_list(assignment_node->rhs, rhs);

        assignment_node->location.start(lhs->location.start());
        assignment_node->location.end(assignment_node->rhs->location.end());

        collect_comments(r, parser, assignment_node->comments);

        return assignment_node;
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr subscript_declaration_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        auto decl_node = parser->ast_builder()->subscript_declaration_node();
        if (!parser->peek(token_types_t::right_square_bracket)) {
            decl_node->lhs = parser->parse_expression(r);
        }
        token_t right_square_bracket;
        right_square_bracket.type = token_types_t::right_square_bracket;
        if (!parser->expect(r, right_square_bracket))
            return nullptr;

        auto current_decl_node = decl_node;
        while (true) {
            if (parser->peek(token_types_t::semi_colon))
                break;
            auto node = parser->parse_expression(r, precedence_t::type);
            current_decl_node->rhs = node;
            if (node->type != ast_node_types_t::subscript_declaration)
                break;
            current_decl_node = node;
        }

        return decl_node;
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr pointer_declaration_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        auto decl_node = parser->ast_builder()->pointer_declaration_node();
        decl_node->rhs = parser->parse_expression(r, precedence_t::type);
        return decl_node;
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr from_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        auto from_node = parser
            ->ast_builder()
            ->from_node(token);
        from_node->rhs = parser->parse_expression(r);
        return from_node;
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr with_member_access_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        auto node = parser->ast_builder()->with_member_access_node();
        node->location.start(token.location.start());

        token_t symbol_token;
        symbol_token.type = token_types_t::identifier;
        if (!parser->expect(r, symbol_token))
            return nullptr;

        node->lhs = parser->ast_builder()->current_with()->lhs;
        node->rhs = create_symbol_node(r, parser, nullptr, symbol_token);
        node->location.end(node->lhs->location.end());

        return node;
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr module_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        return create_module_expression_node(r, parser, nullptr, token);
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr map_expression_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        parser->use_global_comma_precedence();
        defer(parser->use_default_comma_precedence());

        auto node = parser->ast_builder()->map_expression_node();
        node->location.start(token.location.start());

        token_t less_than;
        less_than.type = token_types_t::less_than;
        if (!parser->expect(r, less_than))
            return nullptr;

        // XXX: fix this!
//        auto key_type = create_type_identifier_node(
//            r,
//            parser,
//            less_than);
//        node->lhs->children.push_back(key_type);

        token_t comma;
        comma.type = token_types_t::comma;
        if (!parser->expect(r, comma))
            return nullptr;

// XXX: fix this!
//        auto value_type = create_type_identifier_node(
//            r,
//            parser,
//            comma);
//        node->lhs->children.push_back(value_type);

        token_t greater_than;
        greater_than.type = token_types_t::greater_than;
        if (!parser->expect(r, greater_than))
            return nullptr;

        token_t left_paren;
        left_paren.type = token_types_t::left_paren;
        if (!parser->expect(r, left_paren))
            return nullptr;

        pairs_to_list(
            node->rhs,
            parser->parse_expression(r));

        token_t right_paren;
        right_paren.type = token_types_t::right_paren;
        if (!parser->expect(r, right_paren))
            return nullptr;

        node->location.end(right_paren.location.end());
        return node;
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr array_expression_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        auto node = parser->ast_builder()->array_expression_node();
        node->location.start(token.location.start());

        token_t less_than;
        less_than.type = token_types_t::less_than;
        if (!parser->expect(r, less_than))
            return nullptr;

        // XXX: fix this!
//        node->lhs = create_type_identifier_node(r, parser, less_than);

        token_t greater_than;
        greater_than.type = token_types_t::greater_than;
        if (!parser->expect(r, greater_than))
            return nullptr;

        token_t left_paren;
        left_paren.type = token_types_t::left_paren;
        if (!parser->expect(r, left_paren))
            return nullptr;

        pairs_to_list(
            node->rhs,
            parser->parse_expression(r));

        token_t right_paren;
        right_paren.type = token_types_t::right_paren;
        if (!parser->expect(r, right_paren))
            return nullptr;

        node->location.end(right_paren.location.end());
        return node;
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr new_expression_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        parser->use_global_comma_precedence();
        defer(parser->use_default_comma_precedence());

        auto node = parser->ast_builder()->new_expression_node();
        node->location.start(token.location.start());

        token_t less_than;
        less_than.type = token_types_t::less_than;
        if (!parser->expect(r, less_than))
            return nullptr;

        // XXX: fix this!
//        node->lhs = create_type_identifier_node(r, parser, less_than);

        token_t greater_than;
        greater_than.type = token_types_t::greater_than;
        if (!parser->expect(r, greater_than))
            return nullptr;

        token_t left_paren;
        left_paren.type = token_types_t::left_paren;
        if (!parser->expect(r, left_paren))
            return nullptr;

        pairs_to_list(
            node->rhs,
            parser->parse_expression(r));

        token_t right_paren;
        right_paren.type = token_types_t::right_paren;
        if (!parser->expect(r, right_paren))
            return nullptr;

        node->location.end(right_paren.location.end());
        return node;
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr tuple_expression_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        parser->use_global_comma_precedence();
        defer(parser->use_default_comma_precedence());

        auto node = parser->ast_builder()->tuple_expression_node();
        node->location.start(token.location.start());

        token_t left_paren;
        left_paren.type = token_types_t::left_paren;
        if (!parser->expect(r, left_paren))
            return nullptr;

        pairs_to_list(
            node->rhs,
            parser->parse_expression(r));

        token_t right_paren;
        right_paren.type = token_types_t::right_paren;
        if (!parser->expect(r, right_paren))
            return nullptr;

        node->location.end(right_paren.location.end());
        return node;
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr spread_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        auto node = parser->ast_builder()->spread_operator_node(token);
        node->rhs = parser->parse_expression(r, precedence_t::variable);
        return node;
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr cast_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        return create_cast_node(r, parser, token);
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr transmute_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        return create_transmute_node(r, parser, token);
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr label_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        return parser->ast_builder()->label_node(token);
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr while_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        auto while_node = parser->ast_builder()->while_node(token);

        collect_comments(r, parser, while_node->comments);
        while_node->lhs = parser->parse_expression(r);

        collect_comments(r, parser, while_node->comments);
        while_node->rhs = parser->parse_expression(r);

        return while_node;
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr with_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        auto with_node = parser->ast_builder()->with_node(token);

        // XXX: need to check if there's a current_with on the stack
        //      if true and with_node->lhs is a with_member_access
        //      then create a binary operator for with_node->lhs instead of
        //      just assigning the expression
        collect_comments(r, parser, with_node->comments);
        with_node->lhs = parser->parse_expression(r);

        parser->ast_builder()->push_with(with_node);

        collect_comments(r, parser, with_node->comments);
        with_node->rhs = parser->parse_expression(r);

        parser->ast_builder()->pop_with();

        return with_node;
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr defer_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        auto defer_node = parser->ast_builder()->defer_node(token);
        collect_comments(r, parser, defer_node->comments);
        defer_node->lhs = parser->parse_expression(r);
        return defer_node;
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr union_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        auto union_node = parser->ast_builder()->union_node(token);
        if (!create_type_parameter_nodes(r, parser, union_node->lhs))
            return nullptr;
        collect_comments(r, parser, union_node->comments);
        union_node->rhs = parser->parse_expression(r);
        return union_node;
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr namespace_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        auto namespace_node = parser->ast_builder()->namespace_node(token);
        namespace_node->rhs = parser->parse_expression(r);
        return namespace_node;
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr struct_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        auto struct_node = parser->ast_builder()->struct_node(token);
        if (!create_type_parameter_nodes(r, parser, struct_node->lhs))
            return nullptr;
        collect_comments(r, parser, struct_node->comments);
        struct_node->rhs = parser->parse_expression(r);
        return struct_node;
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr enum_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        auto enum_node = parser->ast_builder()->enum_node(token);
        if (!create_type_parameter_nodes(r, parser, enum_node->lhs))
            return nullptr;
        collect_comments(r, parser, enum_node->comments);
        enum_node->rhs = parser->parse_expression(r);
        return enum_node;
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr for_in_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        auto for_node = parser->ast_builder()->for_in_node(token);

        collect_comments(r, parser, for_node->comments);
        for_node->lhs = parser->parse_expression(r);
        collect_comments(r, parser, for_node->comments);

        token_t in_token;
        in_token.type = token_types_t::in_literal;
        if (!parser->expect(r, in_token))
            return nullptr;

        collect_comments(r, parser, for_node->comments);
        for_node->rhs = parser->parse_expression(
            r,
            precedence_t::variable);
        collect_comments(r, parser, for_node->comments);

        for_node->children.push_back(parser->parse_expression(r));

        return for_node;
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr return_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        auto return_node = parser->ast_builder()->return_node(token);
        if (parser->peek(token_types_t::semi_colon))
            return return_node;
        pairs_to_list(return_node->rhs, parser->parse_expression(r));
        return return_node;
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr if_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        auto if_node = parser->ast_builder()->if_node(token);
        collect_comments(r, parser, if_node->comments);

        if_node->lhs = parser->parse_expression(r);
        collect_comments(r, parser, if_node->comments);

        if_node->children.push_back(parser->parse_expression(r));

        auto current_branch = if_node;
        while (true) {
            collect_comments(r, parser, current_branch->comments);

            if (!parser->peek(token_types_t::else_if_literal))
                break;
            token_t else_if_token;
            parser->current(else_if_token);
            parser->consume();

            current_branch->rhs = parser->ast_builder()->else_if_node(else_if_token);
            collect_comments(r, parser, current_branch->rhs->comments);

            current_branch->rhs->lhs = parser->parse_expression(r);

            collect_comments(r, parser, current_branch->rhs->comments);

            current_branch->rhs->children.push_back(parser->parse_expression(r));

            current_branch = current_branch->rhs;
        }

        if (parser->peek(token_types_t::else_literal)) {
            token_t else_token;
            parser->current(else_token);
            parser->consume();

            current_branch->rhs = parser->ast_builder()->else_node(else_token);
            collect_comments(r, parser, current_branch->rhs->comments);

            current_branch->rhs->children.push_back(parser->parse_expression(r));
        }

        collect_comments(
            r,
            parser,
            current_branch->rhs != nullptr ?
                current_branch->rhs->comments :
                current_branch->comments);

        return if_node;
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr basic_block_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        return parser->parse_scope(r, token);
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr proc_expression_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        auto proc_node = parser->ast_builder()->proc_expression_node(token);

        if (!create_type_parameter_nodes(r, parser, proc_node->lhs->lhs))
            return nullptr;

        token_t left_paren_token;
        left_paren_token.type = token_types_t::left_paren;
        if (!parser->expect(r, left_paren_token))
            return nullptr;

        if (!parser->peek(token_types_t::right_paren)) {
            pairs_to_list(proc_node->rhs, parser->parse_expression(r));
        }

        token_t right_paren_token;
        right_paren_token.type = token_types_t::right_paren;
        if (!parser->expect(r, right_paren_token))
            return nullptr;

        if (parser->peek(token_types_t::colon)) {
            pairs_to_list(
                proc_node->lhs->rhs,
                parser->parse_expression(r));
        }

        while (parser->peek(token_types_t::attribute)) {
            proc_node->attributes.push_back(parser->parse_expression(r));
        }

        if (!parser->peek(token_types_t::semi_colon)) {
            proc_node->children.push_back(parser->parse_expression(r));
        }

        return proc_node;
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr group_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        return create_expression_node(r, parser, nullptr, token);
    }

    ///////////////////////////////////////////////////////////////////////////

    unary_operator_prefix_parser::unary_operator_prefix_parser(
            precedence_t precedence) noexcept : _precedence(precedence) {
    }

    ast_node_shared_ptr unary_operator_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        auto unary_operator_node = parser
            ->ast_builder()
            ->unary_operator_node(token);
        auto rhs = parser->parse_expression(r, _precedence);
        if (rhs == nullptr) {
            parser->error(
                r,
                "P019",
                "unary operator expects right-hand-side expression",
                token.location);
            return nullptr;
        }
        unary_operator_node->rhs = rhs;
        unary_operator_node->location.start(token.location.start());
        unary_operator_node->location.end(unary_operator_node->rhs->location.end());
        return unary_operator_node;
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr keyword_literal_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        switch (token.type) {
            case token_types_t::import_literal: {
                auto import_node = parser->ast_builder()->import_node(token);
                import_node->lhs = parser->parse_expression(r);
                if (import_node->lhs == nullptr) {
                    parser->error(
                        r,
                        "P019",
                        "import expects namespace",
                        token.location);
                    return nullptr;
                }
                if (parser->peek(syntax::token_types_t::from_literal)) {
                    token_t from_token;
                    parser->current(from_token);
                    parser->consume();
                    import_node->rhs = parser->parse_expression(r);
                    if (import_node->rhs == nullptr) {
                        parser->error(
                            r,
                            "P019",
                            "from expects identifier of type module",
                            from_token.location);
                        return nullptr;
                    }
                }

                return import_node;
            }
            case token_types_t::break_literal: {
                auto break_node = parser->ast_builder()->break_node(token);
                if (parser->peek(syntax::token_types_t::label)) {
                    break_node->lhs = parser->parse_expression(r);
                }
                return break_node;
            }
            case token_types_t::continue_literal: {
                auto continue_node = parser->ast_builder()->continue_node(token);
                if (parser->peek(syntax::token_types_t::label)) {
                    continue_node->lhs = parser->parse_expression(r);
                }
                return continue_node;
            }
            case token_types_t::nil_literal: {
                return parser->ast_builder()->nil_literal_node(token);
            }
            case token_types_t::true_literal:
            case token_types_t::false_literal: {
                return parser->ast_builder()->boolean_literal_node(token);
            }
            default:
                return nullptr;
        }
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr number_literal_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        return parser->ast_builder()->number_literal_node(token);
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr string_literal_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        return parser->ast_builder()->string_literal_node(token);
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr char_literal_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        return parser->ast_builder()->character_literal_node(token);
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr line_comment_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        return parser->ast_builder()->line_comment_node(token);
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr block_comment_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        return parser->ast_builder()->block_comment_node(token);
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr symbol_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        return create_symbol_node(r, parser, nullptr, token);
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr pointer_dereference_infix_parser::parse(
            common::result& r,
            parser* parser,
            const ast_node_shared_ptr& lhs,
            token_t& token) {
        auto node = parser->ast_builder()->unary_operator_node(token);
        node->rhs = lhs;
        return node;
    }

    precedence_t pointer_dereference_infix_parser::precedence() const {
        return precedence_t::pointer_dereference;
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr comma_infix_parser::parse(
            common::result& r,
            parser* parser,
            const ast_node_shared_ptr& lhs,
            token_t& token) {
        auto pair_node = parser->ast_builder()->pair_node();
        ast_node_list comments {};
        collect_comments(r, parser, comments);

        pair_node->lhs = lhs;
        pair_node->rhs = parser->parse_expression(
            r,
            parser->comma_precedence());

        if (lhs->type != ast_node_types_t::pair) {
            lhs->comments = comments;
        } else {
            pair_node->rhs->comments = comments;
        }

        return pair_node;
    }

    precedence_t comma_infix_parser::precedence() const {
        return _precedence;
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr cast_infix_parser::parse(
            common::result& r,
            parser* parser,
            const ast_node_shared_ptr& lhs,
            token_t& token) {
        lhs->rhs = create_cast_node(r, parser, token);
        return lhs;
    }

    precedence_t cast_infix_parser::precedence() const {
        return precedence_t::cast;
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr proc_call_infix_parser::parse(
            common::result& r,
            parser* parser,
            const ast_node_shared_ptr& lhs,
            token_t& token) {
        parser->use_global_comma_precedence();
        defer(parser->use_default_comma_precedence());

        auto proc_call_node = parser->ast_builder()->proc_call_node();
        proc_call_node->location.start(lhs->location.start());
        proc_call_node->lhs->rhs = lhs;

        if (!parser->peek(token_types_t::right_paren)) {
            pairs_to_list(
                proc_call_node->rhs,
                parser->parse_expression(r));
        }

        token_t right_paren_token;
        right_paren_token.type = token_types_t::right_paren;
        if (!parser->expect(r, right_paren_token))
            return nullptr;

        proc_call_node->location.end(right_paren_token.location.end());

        return proc_call_node;
    }

    precedence_t proc_call_infix_parser::precedence() const {
        return precedence_t::call;
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr type_declaration_infix_parser::parse(
            common::result& r,
            parser* parser,
            const ast_node_shared_ptr& lhs,
            token_t& token) {
        auto node = parser->ast_builder()->type_declaration_node();
        node->location.start(token.location.start());

        collect_comments(r, parser, node->comments);

        node->lhs = parser->parse_expression(r, precedence_t::type);
        node->location.end(node->lhs->location.end());

        collect_comments(r, parser, node->comments);

        lhs->rhs = node;

        return lhs;
    }

    precedence_t type_declaration_infix_parser::precedence() const {
        return precedence_t::type;
    }

    ///////////////////////////////////////////////////////////////////////////

    binary_operator_infix_parser::binary_operator_infix_parser(
            precedence_t precedence,
            bool is_right_associative,
            bool with_assignment) noexcept : _precedence(precedence),
                                    _with_assignment(with_assignment),
                                    _is_right_associative(is_right_associative) {
    }

    ast_node_shared_ptr binary_operator_infix_parser::parse(
            common::result& r,
            parser* parser,
            const ast_node_shared_ptr& lhs,
            token_t& token) {
        auto associative_precedence = static_cast<precedence_t>(
            static_cast<uint8_t>(_precedence) - (_is_right_associative ? 1 : 0));
        auto rhs = parser->parse_expression(r, associative_precedence);
        if (rhs == nullptr) {
            parser->error(
                r,
                "P019",
                "binary operator expects right-hand-side expression",
                token.location);
            return nullptr;
        }

        auto bin_op_node = parser
            ->ast_builder()
            ->binary_operator_node(lhs, extract_non_assign_operator(token), rhs);
        if (!_with_assignment)
            return bin_op_node;

        auto assignment_node = parser->ast_builder()->assignment_node();
        pairs_to_list(assignment_node->lhs, lhs);
        pairs_to_list(assignment_node->rhs, bin_op_node);
        assignment_node->location.start(lhs->location.start());
        assignment_node->location.end(assignment_node->rhs->location.end());

        return assignment_node;
    }

    precedence_t binary_operator_infix_parser::precedence() const {
        return _precedence;
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr constant_assignment_infix_parser::parse(
            common::result& r,
            parser* parser,
            const ast_node_shared_ptr& lhs,
            token_t& token) {
        return create_assignment_node(
            r,
            ast_node_types_t::constant_assignment,
            parser,
            lhs,
            token);
    }

    precedence_t constant_assignment_infix_parser::precedence() const {
        return _precedence;
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr assignment_infix_parser::parse(
            common::result& r,
            parser* parser,
            const ast_node_shared_ptr& lhs,
            token_t& token) {
        return create_assignment_node(
            r,
            ast_node_types_t::assignment,
            parser,
            lhs,
            token);
    }

    precedence_t assignment_infix_parser::precedence() const {
        return _precedence;
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr raw_block_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        return parser->ast_builder()->raw_block_node(token);
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr directive_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        auto directive_node = parser->ast_builder()->directive_node(token);
        if (parser->peek(token_types_t::semi_colon)) {
            return directive_node;
        }
//        if (token.value == "type") {
//            directive_node->lhs = create_type_identifier_node(r, parser, token);
//        } else {
        directive_node->lhs = parser->parse_expression(r);
//        }
        return directive_node;
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr attribute_prefix_parser::parse(
            common::result& r,
            parser* parser,
            token_t& token) {
        auto attribute_node = parser->ast_builder()->attribute_node(token);
        if (parser->peek(token_types_t::semi_colon)
        ||  parser->peek(token_types_t::attribute)) {
            return attribute_node;
        }
        attribute_node->lhs = parser->parse_expression(r);
        return attribute_node;
    }

    ///////////////////////////////////////////////////////////////////////////

    ast_node_shared_ptr array_subscript_infix_parser::parse(
            common::result& r,
            parser* parser,
            const ast_node_shared_ptr& lhs,
            token_t& token) {
        ast_node_shared_ptr subscript_node = parser->ast_builder()->subscript_operator_node();
        if (parser->peek(token_types_t::right_square_bracket)) {
            parser->error(
                r,
                "B027",
                "subscript index expected.",
                token.location);
            return nullptr;
        }

        subscript_node->lhs = lhs;
        subscript_node->rhs = parser->parse_expression(r);

        token_t right_bracket_token;
        right_bracket_token.type = token_types_t::right_square_bracket;
        if (!parser->expect(r, right_bracket_token))
            return nullptr;

        return subscript_node;
    }

    precedence_t array_subscript_infix_parser::precedence() const {
        return precedence_t::subscript;
    }

    ///////////////////////////////////////////////////////////////////////////

    parser::parser(
        common::source_file* source_file,
        syntax::ast_builder& builder) : _lexer(source_file),
                                        _ast_builder(builder),
                                        _source_file(source_file) {
    }

    void parser::error(
            common::result& r,
            const std::string& code,
            const std::string& message,
            const common::source_location& location) {
        _source_file->error(r, code, message, location);
    }

    bool parser::consume() {
        token_t token;
        return consume(token);
    }

    void parser::write_ast_graph(
            const boost::filesystem::path& path,
            const ast_node_shared_ptr& program_node) {
        auto close_required = false;
        FILE* ast_output_file = stdout;
        if (!path.empty()) {
            ast_output_file = fopen(
                path.string().c_str(),
                "wt");
            close_required = true;
        }

        ast_formatter formatter(
            program_node,
            ast_output_file);
        formatter.format(fmt::format("AST Graph: {}", path.string()));

        if (close_required)
            fclose(ast_output_file);
    }

    bool parser::consume(token_t& token) {
        if (!look_ahead(0))
            return false;

        token = _tokens.front();
        _tokens.erase(_tokens.begin());

        return token.type != token_types_t::end_of_file;
    }

    bool parser::current(token_t& token) {
        if (!look_ahead(0))
            return false;

        token = _tokens.front();

        return token.type != token_types_t::end_of_file;
    }

    bool parser::peek(token_types_t type) {
        if (!look_ahead(0))
            return false;
        auto& token = _tokens.front();
        return token.type == type;
    }

    bool parser::look_ahead(size_t count) {
        while (count >= _tokens.size() && _lexer.has_next()) {
            token_t token;
            if (_lexer.next(token))
                _tokens.push_back(token);
        }
        return !_tokens.empty();
    }

    precedence_t parser::comma_precedence() const {
        return s_comma_infix_parser.precedence();
    }

    precedence_t parser::current_infix_precedence() {
        if (!look_ahead(0))
            return precedence_t::lowest;

        auto& token = _tokens.front();
        auto infix_parser = infix_parser_for(token.type);
        if (infix_parser != nullptr)
            return infix_parser->precedence();

        return precedence_t::lowest;
    }

    syntax::ast_builder* parser::ast_builder() {
        return &_ast_builder;
    }

    void parser::use_global_comma_precedence() {
        s_comma_infix_parser.precedence(precedence_t::assignment);
        s_assignment_infix_parser.precedence(precedence_t::comma);
        s_constant_assignment_infix_parser.precedence(precedence_t::comma);
    }

    void parser::use_default_comma_precedence() {
        s_comma_infix_parser.precedence(precedence_t::comma);
        s_assignment_infix_parser.precedence(precedence_t::assignment);
        s_constant_assignment_infix_parser.precedence(precedence_t::assignment);
    }

    ast_node_shared_ptr parser::parse_expression(
            common::result& r,
            precedence_t precedence) {
        token_t token;
        if (!consume(token))
            return nullptr;

        auto prefix_parser = prefix_parser_for(token.type);
        if (prefix_parser == nullptr) {
            error(
                r,
                "B021",
                fmt::format("prefix parser for token '{}' not found.", token.name()),
                token.location);
            return nullptr;
        }

        auto lhs = prefix_parser->parse(r, this, token);
        if (lhs == nullptr) {
            error(
                r,
                "B021",
                "unexpected empty ast node.",
                token.location);
            return nullptr;
        }

        if (token.is_line_comment()
        ||  token.is_label())
            return lhs;

        while (precedence < current_infix_precedence()) {
            if (!consume(token)) {
                break;
            }

            auto infix_parser = infix_parser_for(token.type);
            if (infix_parser == nullptr) {
                error(
                    r,
                    "B021",
                    fmt::format("infix parser for token '{}' not found.", token.name()),
                    token.location);
                break;
            }
            lhs = infix_parser->parse(r, this, lhs, token);
            if (lhs == nullptr || r.is_failed())
                break;
        }

        return lhs;
    }

    ast_node_shared_ptr parser::expect_expression(
            common::result& r,
            ast_node_types_t expected_type,
            precedence_t precedence) {
        auto node = parse_expression(r, precedence);
        if (node == nullptr)
            return nullptr;

        if (node->type != expected_type) {
            error(
                r,
                "B031",
                fmt::format(
                    "unexpected '{}', wanted '{}'.",
                    node->name(),
                    ast_node_type_name(expected_type)),
                node->token.location);
            return nullptr;
        }

        return node;
    }

    precedence_t parser::assignment_precedence() const {
        return s_assignment_infix_parser.precedence();
    }

    ast_node_shared_ptr parser::parse(common::result& r) {
        token_t empty_token {};
        return parse_scope(r, empty_token);
    }

    bool parser::expect(common::result& r, token_t& token) {
        if (!look_ahead(0))
            return false;

        std::string expected_name = token.name();
        auto expected_type = token.type;
        token = _tokens.front();
        if (token.type != expected_type) {
            error(
                r,
                "B016",
                fmt::format(
                    "expected token '{}' but found '{}'.",
                    expected_name,
                    token.name()),
                token.location);
            return false;
        }

        _tokens.erase(_tokens.begin());

        return true;
    }

    ast_node_shared_ptr parser::parse_scope(
            common::result& r,
            token_t& token) {
        auto scope = _ast_builder.begin_scope();
        scope->location.start(token.location.start());

        auto is_end_of_scope = [&]() -> bool {
            if (peek(token_types_t::end_of_file))
                return true;

            if (peek(token_types_t::right_curly_brace)) {
                token_t right_curly_brace;
                current(right_curly_brace);
                consume();
                scope->location.end(right_curly_brace.location.end());
                return true;
            }

            return false;
        };

        while (!is_end_of_scope()) {
            auto statement = parse_statement(r);
            if (r.is_failed())
                return nullptr;
            if (!scope->attributes.empty()) {
                for (const auto& attr_node : scope->attributes)
                    statement->attributes.push_back(attr_node);
                scope->attributes.clear();
            }
            scope->children.push_back(statement);
        }

        while (peek(token_types_t::attribute)) {
            scope->attributes.push_back(parse_expression(r));
        }

        return _ast_builder.end_scope();
    }

    ast_node_shared_ptr parser::parse_statement(common::result& r) {
        auto statement_node = _ast_builder.statement_node();

        while (true) {
            collect_comments(r, this, statement_node->comments);

            if (peek(token_types_t::right_curly_brace))
                return statement_node;

            auto expr = parse_expression(r);
            if (expr == nullptr)
                return statement_node;

            if (expr->is_attribute()) {
                if (peek(token_types_t::semi_colon)) {
                    consume();
                    _ast_builder.current_scope()->attributes.push_back(expr);
                } else {
                    statement_node->attributes.push_back(expr);
                }
                continue;
            }

            if (expr->is_label()) {
                statement_node->labels.push_back(expr);
                continue;
            }

            statement_node->rhs = expr;
            statement_node->location = expr->location;

            break;
        }

        token_t line_terminator_token;
        line_terminator_token.type = token_types_t::semi_colon;
        if (!expect(r, line_terminator_token)) {
            error(
                r,
                "B031",
                "expected semi-colon",
                statement_node->location);
            return nullptr;
        }

        return statement_node;
    }

    infix_parser* parser::infix_parser_for(token_types_t type) {
        auto it = s_infix_parsers.find(type);
        if (it == s_infix_parsers.end())
            return nullptr;
        return it->second;
    }

    prefix_parser* parser::prefix_parser_for(token_types_t type) {
        auto it = s_prefix_parsers.find(type);
        if (it == s_prefix_parsers.end())
            return nullptr;
        return it->second;
    }

}