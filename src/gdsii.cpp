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
#include <gdstk/utils.hpp>

namespace gdstk {

uint64_t gdsii_real_from_double(double value) {
    if (value == 0) return 0;
    uint8_t u8_1 = 0;
    if (value < 0) {
        u8_1 = 0x80;
        value = -value;
    }
    const double fexp = 0.25 * log2(value);
    double exponent = ceil(fexp);
    if (exponent == fexp) exponent++;
    const uint64_t mantissa = (uint64_t)(value * pow(16, 14 - exponent));
    u8_1 += (uint8_t)(64 + exponent);
    const uint64_t result = ((uint64_t)u8_1 << 56) | (mantissa & 0x00FFFFFFFFFFFFFF);
    return result;
}

double gdsii_real_to_double(uint64_t real) {
    const int64_t exponent = ((real & 0x7F00000000000000) >> 54) - 256;
    const double mantissa = ((double)(real & 0x00FFFFFFFFFFFFFF)) / 72057594037927936.0;
    const double result = mantissa * exp2((double)exponent);
    return (real & 0x8000000000000000) ? -result : result;
}

// Throttled: only log the first 2 calls + every 50M calls to keep output sane for multi-GB files
static uint64_t s_read_record_call_count = 0;
static const uint64_t s_read_record_log_interval = 50000000;
// Set by read_gds before the parse loop; guards all debug fprintf in gdsii_read_record
bool g_gdsii_verbose = false;

ErrorCode gdsii_read_record(FILE* in, uint8_t* buffer, uint64_t& buffer_count) {
    s_read_record_call_count++;

    if (buffer_count < 4) {
        if (g_gdsii_verbose) {
            fprintf(stderr, "[V4D-DEBUG-GDS] gdsii_read_record: insufficient buffer (< 4) "
                            "at call #%" PRIu64 "\n", s_read_record_call_count);
            fflush(stderr);
        }
        if (error_logger) fputs("[GDSTK] Insufficient memory in buffer.\n", error_logger);
        return ErrorCode::InsufficientMemory;
    }

    if (g_gdsii_verbose &&
        (s_read_record_call_count <= 2 || s_read_record_call_count % s_read_record_log_interval == 0)) {
        long pos = ftell(in);
        fprintf(stderr, "[V4D-DEBUG-GDS] gdsii_read_record call #%" PRIu64
                        ": about to fread 4-byte header at file_pos %ld. "
                        ">>> If this is the last log, hang is in fread (file I/O). "
                        "Check NFS/network mount/disk. <<<\n",
                s_read_record_call_count, pos);
        fflush(stderr);
    }

    uint64_t read_length = fread(buffer, 1, 4, in);
    if (read_length < 4) {
        if (g_gdsii_verbose) {
            fprintf(stderr, "[V4D-DEBUG-GDS] gdsii_read_record: short header read %" PRIu64
                            "/4 bytes at call #%" PRIu64 ", eof=%d, ferror=%d\n",
                    read_length, s_read_record_call_count, feof(in), ferror(in));
            fflush(stderr);
        }
        DEBUG_PRINT("Read bytes (expected 4): %" PRIu64 "\n", read_length);
        if (feof(in) != 0) {
            if (error_logger)
                fputs("[GDSTK] Unable to read input file. End of file reached unexpectedly.\n",
                      error_logger);
        } else {
            if (error_logger)
                fprintf(error_logger, "[GDSTK] Unable to read input file. Error number %d\n.",
                        ferror(in));
        }
        buffer_count = read_length;
        return ErrorCode::InputFileError;
    }
    big_endian_swap16((uint16_t*)buffer, 1);  // second word is interpreted byte-wise (no swapping);
    const uint32_t record_length = *((uint16_t*)buffer);
    if (record_length < 4) {
        if (g_gdsii_verbose) {
            fprintf(stderr, "[V4D-DEBUG-GDS] gdsii_read_record: invalid record_length=%" PRIu32
                            " at call #%" PRIu64 "\n", record_length, s_read_record_call_count);
            fflush(stderr);
        }
        DEBUG_PRINT("Record length should be at least 4. Found %" PRIu32 "\n", record_length);
        if (error_logger) fputs("[GDSTK] Invalid or corrupted GDSII file.\n", error_logger);
        buffer_count = read_length;
        return ErrorCode::InvalidFile;
    } else if (record_length == 4) {
        buffer_count = read_length;
        return ErrorCode::NoError;
    }
    if (buffer_count < 4 + record_length) {
        if (g_gdsii_verbose) {
            fprintf(stderr, "[V4D-DEBUG-GDS] gdsii_read_record: buffer too small for record "
                            "(need %" PRIu32 "+4, have %" PRIu64 ") at call #%" PRIu64 "\n",
                    record_length, buffer_count, s_read_record_call_count);
            fflush(stderr);
        }
        if (error_logger) fputs("[GDSTK] Insufficient memory in buffer.\n", error_logger);
        buffer_count = read_length;
        return ErrorCode::InsufficientMemory;
    }

    if (g_gdsii_verbose && record_length > 65000) {
        fprintf(stderr, "[V4D-DEBUG-GDS] gdsii_read_record: about to read LARGE record data "
                        "(%" PRIu32 " bytes) at call #%" PRIu64 ". "
                        ">>> If this is the last log, hang is in fread of large record data. <<<\n",
                record_length - 4, s_read_record_call_count);
        fflush(stderr);
    }

    read_length = fread(buffer + 4, 1, record_length - 4, in);
    buffer_count = 4 + read_length;
    if (read_length < record_length - 4) {
        if (g_gdsii_verbose) {
            fprintf(stderr, "[V4D-DEBUG-GDS] gdsii_read_record: short data read %" PRIu64
                            "/%" PRIu32 " bytes at call #%" PRIu64 ", eof=%d, ferror=%d\n",
                    read_length, record_length - 4, s_read_record_call_count, feof(in), ferror(in));
            fflush(stderr);
        }
        DEBUG_PRINT("Read bytes (expected %" PRIu32 "): %" PRIu64 "\n", record_length - 4,
                    read_length);
        if (feof(in) != 0) {
            if (error_logger)
                fputs("[GDSTK] Unable to read input file. End of file reached unexpectedly.\n",
                      error_logger);
        } else {
            if (error_logger)
                fprintf(error_logger, "[GDSTK] Unable to read input file. Error number %d\n.",
                        ferror(in));
        }
        return ErrorCode::InputFileError;
    }
    return ErrorCode::NoError;
}

}  // namespace gdstk
