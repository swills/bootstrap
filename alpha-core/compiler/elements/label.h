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

    class label : public element {
    public:
        label(
            compiler::module* module,
            block* parent_scope,
            const std::string& name);

        std::string name() const;

    protected:
        bool on_is_constant() const override;

        bool on_emit(compiler::session& session) override;

    private:
        std::string _name;
    };

};

