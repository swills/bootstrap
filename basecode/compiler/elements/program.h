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

#include <functional>
#include <parser/ast.h>
#include <vm/vm_types.h>
#include <common/id_pool.h>
#include <common/source_file.h>
#include "element.h"

namespace basecode::compiler {

    class program : public element {
    public:
        program();

        ~program() override;

        compiler::block* block();

        void block(compiler::block* value);

    private:
        bool on_emit(
            compiler::session& session,
            compiler::emit_context_t& context,
            compiler::emit_result_t& result) override;

        bool emit_section_variable(
            compiler::session& session,
            compiler::element* e,
            vm::instruction_block* instruction_block);

        void initialize_variable_sections();

        bool emit_sections(compiler::session& session);

        bool emit_end_block(compiler::session& session);

        bool emit_type_info(compiler::session& session);

        bool emit_finalizers(compiler::session& session);

        bool emit_start_block(compiler::session& session);

        bool emit_initializers(compiler::session& session);

        bool emit_bootstrap_block(compiler::session& session);

        bool emit_implicit_blocks(compiler::session& session);

        bool emit_procedure_types(compiler::session& session);

        element_list_t* variable_section(vm::section_t section);

        void intern_string_literals(compiler::session& session);

        bool group_identifiers_by_section(compiler::session& session);

    private:
        compiler::block* _block = nullptr;
        compiler::type* _string_type = nullptr;
        std::map<vm::section_t, element_list_t> _vars_by_section {};
    };

};
