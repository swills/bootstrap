// ----------------------------------------------------------------------------
//
// Basecode Bootstrap Compiler
// Copyright (C) 2018 Jeff Panici
// All rights reserved.
//
// This software source file is licensed under the terms of MIT license.
// For details, please read the LICENSE.md file.
//
// ----------------------------------------------------------------------------

#pragma once

#include "element.h"

namespace basecode::compiler {

    class namespace_element : public element {
    public:
        namespace_element(
            element* parent,
            element* expr);

        element* expression();

    protected:
        compiler::type* on_infer_type(const compiler::program* program) override;

    private:
        element* _expression = nullptr;
    };

};
