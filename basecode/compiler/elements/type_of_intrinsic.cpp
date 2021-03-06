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

#include <compiler/session.h>
#include <compiler/scope_manager.h>
#include "type.h"
#include "pointer_type.h"
#include "argument_list.h"
#include "symbol_element.h"
#include "type_reference.h"
#include "integer_literal.h"
#include "type_of_intrinsic.h"
#include "assembly_literal_label.h"

namespace basecode::compiler {

    type_of_intrinsic::type_of_intrinsic(
            compiler::module* module,
            compiler::block* parent_scope,
            compiler::argument_list* args,
            compiler::procedure_type* proc_type,
            const compiler::type_reference_list_t& type_params) : intrinsic(module,
                                                                            parent_scope,
                                                                            args,
                                                                            proc_type,
                                                                            type_params) {
    }

    bool type_of_intrinsic::on_fold(
            compiler::session& session,
            fold_result_t& result) {
        auto args = arguments()->elements();
        if (args.empty() || args.size() > 1) {
            session.error(
                module(),
                "P091",
                "type_of expects a single argument.",
                location());
            return false;
        }

        auto arg = args[0];
        infer_type_result_t infer_type_result {};
        if (!arg->infer_type(session, infer_type_result)) {
            session.error(
                module(),
                "P091",
                "type_of unable to infer type.",
                location());
            return false;
        }

        auto label_name = compiler::type::make_info_label_name(infer_type_result.inferred_type);
        result.element = session.builder().make_assembly_literal_label(
            parent_scope(),
            infer_type_result.inferred_type,
            label_name,
            module());
        result.element->location(location());

        return true;
    }

    bool type_of_intrinsic::on_infer_type(
            compiler::session& session,
            infer_type_result_t& result) {
        auto& builder = session.builder();
        auto& scope_manager = session.scope_manager();

        qualified_symbol_t type_name("type");
        auto type_info_type = scope_manager.find_type(type_name);
        auto ptr_type = scope_manager.find_pointer_type(
            type_info_type,
            parent_scope());
        if (ptr_type == nullptr) {
            ptr_type = builder.make_pointer_type(
                parent_scope(),
                type_name,
                type_info_type);
        }

        result.inferred_type = ptr_type;
        result.reference = builder.make_type_reference(
            parent_scope(),
            type_name,
            ptr_type);

        return true;
    }

    bool type_of_intrinsic::can_fold() const {
        return true;
    }

    std::string type_of_intrinsic::name() const {
        return "type_of";
    }

};