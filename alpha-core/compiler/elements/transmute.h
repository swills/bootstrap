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

#include "element.h"

namespace basecode::compiler {

    class transmute  : public element {
    public:
        transmute(
            compiler::module* module,
            block* parent_scope,
            compiler::type* type,
            element* expr);

        element* expression();

        compiler::type* type();

        void type_location(const common::source_location& loc);

    protected:
        bool on_emit(compiler::session& session) override;

        void on_owned_elements(element_list_t& list) override;

        compiler::type* on_infer_type(const compiler::session& session) override;

    private:
        element* _expression = nullptr;
        compiler::type* _type = nullptr;
        common::source_location _type_location;
    };

};
