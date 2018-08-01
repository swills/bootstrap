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

#pragma once

#include "operator_base.h"
#include "element_types.h"

namespace basecode::compiler {

    class unary_operator : public operator_base {
    public:
        unary_operator(
            block* parent_scope,
            operator_type_t type,
            element* rhs);

        element* rhs();

    protected:
        bool on_emit(
            common::result& r,
            emit_context_t& context) override;

        bool on_is_constant() const override;

        element* on_fold(
            common::result& r,
            compiler::program* program) override;

        void on_owned_elements(element_list_t& list) override;

        compiler::type* on_infer_type(const compiler::program* program) override;

    private:
        element* _rhs = nullptr;
    };

};

