/*
Copyright 2020 Lucas Heitzmann Gabrielli.
This file is part of gdstk, distributed under the terms of the
Boost Software License - Version 1.0.  See the accompanying
LICENSE file or <http://www.boost.org/LICENSE_1_0.txt>
*/

#define __STDC_FORMAT_MACROS 1
#define _USE_MATH_DEFINES

#include <math.h>
#include <stdint.h>
#include <stdio.h>

#include <gdstk/gdsii.hpp>
#include <gdstk/label.hpp>
#include <gdstk/v4item.hpp>
#include <gdstk/utils.hpp>

namespace gdstk {

void V4Item::print() {
    printf("V4Item <%p> %s, layer %" PRIu32 ", texttype %" PRIu32 ", owner <%p>\n",
           this, text, get_layer(tag), get_type(tag), owner);
}

void V4Item::clear() {
    if (text) {
        free_allocation(text);
        text = NULL;
    }
}

void V4Item::copy_from(const V4Item& v4item) {
    tag = v4item.tag;
    text = copy_string(v4item.text, NULL);
}

}  // namespace gdstk
