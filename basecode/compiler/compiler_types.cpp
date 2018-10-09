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

#include "variable.h"
#include "compiler_types.h"

namespace basecode::compiler {

    variable_handle_t::~variable_handle_t() {
        if (_instance != nullptr && !_skip_deactivate)
            _instance->deactivate();
    }

    void variable_handle_t::skip_deactivate() {
        _skip_deactivate = true;
    }

    void variable_handle_t::set(variable* instance) {
        _instance = instance;
        _instance->activate();
    }

};