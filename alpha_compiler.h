#pragma once

#include <cstdint>
#include "terp.h"
#include "alpha_parser.h"
#include "symbol_table.h"
#include "evaluator.h"

namespace basecode {

    class alpha_compiler {
    public:
        explicit alpha_compiler(size_t heap_size);

        virtual ~alpha_compiler();

        inline uint64_t address() const {
            return _address;
        }

        bool initialize(result& r);

        inline basecode::symbol_table* symbol_table() {
            return &_symbol_table;
        }

        bool compile(result& r, const parser_input_t& input);

        bool compile_stream(result& r, const parser_input_t& input);

    private:
        terp _terp;
        uint64_t _address;
        evaluator _evaluator;
        alpha_parser _parser {};
        basecode::symbol_table _symbol_table {};
    };

};

