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

#include <cstdint>

namespace basecode::common {

    class source_location {
    public:
        source_location();

        source_location(
            uint32_t line,
            uint32_t start_column,
            uint32_t end_column);

        uint32_t line() const;

        void line(uint32_t value);

        uint32_t end_column() const;

        uint32_t start_column() const;

        void end_column(uint32_t value);

        void start_column(uint32_t value);

    private:
        uint32_t _line = 0;
        uint32_t _end_column = 0;
        uint32_t _start_column = 0;
    };

};
