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

#include "composite_type.h"

namespace basecode::compiler {

    class any_type : public compiler::composite_type {
    public:
        explicit any_type(element* parent);

        compiler::type* underlying_type();

        void underlying_type(compiler::type* value);

    protected:
        bool on_initialize(
            common::result& r,
            compiler::program* program) override;

    private:
        compiler::type* _underlying_type = nullptr;
    };

};

