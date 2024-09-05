/*
Copyright 2020 Lucas Heitzmann Gabrielli.
This file is part of gdstk, distributed under the terms of the
Boost Software License - Version 1.0.  See the accompanying
LICENSE file or <http://www.boost.org/LICENSE_1_0.txt>
*/

#ifndef GDSTK_HEADER_V4ITEM
#define GDSTK_HEADER_V4ITEM

#define __STDC_FORMAT_MACROS 1
#define _USE_MATH_DEFINES

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <vector>

#include "allocator.hpp"
#include "property.hpp"
#include "repetition.hpp"
#include "utils.hpp"
#include "vec.hpp"

namespace gdstk {

struct V4Item {

    Tag tag;
    char* text;  // NULL-terminated text string
    Array<double> arr0;
    Array<double> arr1;
    std::vector<std::string> layer_names;
    std::vector<int>         layer_numbers;
    // Used by the python interface to store the associated PyObject* (if any).
    // No functions in gdstk namespace should touch this value!
    void* owner;

    void init(const char* text_) {
        text = copy_string(text_, NULL);
    }

    void print();

    void clear();

    // This label instance must be zeroed before copy_from
    void copy_from(const V4Item& label);
};

}  // namespace gdstk

#endif
