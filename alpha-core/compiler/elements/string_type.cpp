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

#include "program.h"
#include "string_type.h"

namespace basecode::compiler {

    string_type::string_type(element* parent) : compiler::composite_type(
                                                    parent,
                                                    composite_types_t::struct_type,
                                                    "string",
                                                    element_type_t::string_type) {
    }

    // string_type := struct {
    //      length:u32;
    //      capacity:u32;
    //      data:address;
    // }; 16 bytes

    bool string_type::on_initialize(
            common::result& r,
            compiler::program* program) {
        auto block_scope = dynamic_cast<compiler::block*>(parent());

        auto u32_type = program->find_type_down("u32");
        auto address_type = program->find_type_down("address");

        auto length_identifier = program->make_identifier(
            block_scope,
            "length",
            nullptr);
        length_identifier->type(u32_type);
        auto length_field = program->make_field(block_scope, length_identifier);

        auto capacity_identifier = program->make_identifier(
            block_scope,
            "capacity",
            nullptr);
        capacity_identifier->type(u32_type);
        auto capacity_field = program->make_field(block_scope, capacity_identifier);

        auto data_identifier = program->make_identifier(
            block_scope,
            "data",
            nullptr);
        data_identifier->type(address_type);
        auto data_field = program->make_field(block_scope, data_identifier);

        fields().add(length_field);
        fields().add(capacity_field);
        fields().add(data_field);

        return composite_type::on_initialize(r, program);
    }

};