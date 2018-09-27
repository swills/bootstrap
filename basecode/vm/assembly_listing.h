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

#include <stack>
#include <string>
#include <vector>
#include <unordered_map>
#include <boost/filesystem.hpp>

namespace basecode::vm {

    struct listing_source_line_t {
        uint64_t address = 0;
        std::string source {};
    };

    struct listing_source_file_t {
        void add_source_line(
                uint64_t address,
                const std::string& source) {
            lines.push_back(listing_source_line_t {
                .address = address,
                .source = source
            });
        }

        void add_blank_lines(
                uint64_t address,
                uint16_t count = 1) {
            for (uint16_t i = 0; i < count; i++) {
                lines.push_back(listing_source_line_t {
                    .address = address,
                    .source = ""
                });
            }
        }

        boost::filesystem::path path;
        std::vector<listing_source_line_t> lines {};
    };

    class assembly_listing {
    public:
        assembly_listing();

        void reset();

        void write(FILE* file);

        listing_source_file_t* current_source_file();

        void add_source_file(const boost::filesystem::path& path);

        void select_source_file(const boost::filesystem::path& path);

    private:
        listing_source_file_t* _current_source_file = nullptr;
        std::unordered_map<std::string, listing_source_file_t> _source_files {};
    };

};
