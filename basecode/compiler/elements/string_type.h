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

#include "composite_type.h"

namespace basecode::compiler {

    class string_type : public compiler::composite_type {
    public:
        string_type(
            compiler::module* module,
            compiler::block* parent_scope,
            compiler::block* scope);

    protected:
        bool on_emit_finalizer(
            compiler::session& session,
            compiler::variable* var) override;

        bool on_emit_initializer(
            compiler::session& session,
            compiler::variable* var) override;

        bool on_type_check(compiler::type* other) override;

        type_access_model_t on_access_model() const override;

        bool on_initialize(compiler::session& session) override;
    };

};

