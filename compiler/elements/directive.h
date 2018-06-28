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

#include "block.h"

namespace basecode::compiler {

    class directive : public element {
    public:
        using directive_callable = std::function<bool (
            compiler::directive*,
            common::result&,
            compiler::program*)>;

        directive(
            block* parent,
            const std::string& name,
            element* expression);

        bool execute(
            common::result& r,
            compiler::program* program);

        element* expression();

        std::string name() const;

    private:
        bool on_run(common::result& r, compiler::program* program);

        bool on_load(common::result& r, compiler::program* program);

        bool on_foreign(common::result& r, compiler::program* program);

    private:
        inline static std::unordered_map<std::string, directive_callable> s_directive_handlers = {
            {"run",     std::bind(&directive::on_run,     std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)},
            {"load",    std::bind(&directive::on_load,    std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)},
            {"foreign", std::bind(&directive::on_foreign, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3)},
        };

        std::string _name;
        element* _expression = nullptr;
    };

};

