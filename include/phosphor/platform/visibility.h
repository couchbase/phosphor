/* -*- Mode: C++; tab-width: 4; c-basic-offset: 4; indent-tabs-mode: nil -*- */
/*
 *     Copyright 2016-Present Couchbase, Inc.
 *
 *   Use of this software is governed by the Business Source License included
 *   in the file licenses/BSL-Couchbase.txt.  As of the Change Date specified
 *   in that file, in accordance with the Business Source License, use of this
 *   software will be governed by the Apache License, Version 2.0, included in
 *   the file licenses/APL2.txt.
 */

#pragma once

/* Macros for handling symbol visibility */

/* EXPORT_SYMBOL
 *
 * Use for symbols which should be exported (externally visible) from a
 * library.
 */
#if (defined(__SUNPRO_C) && (__SUNPRO_C >= 0x550)) || \
        (defined(__SUNPRO_CC) && (__SUNPRO_CC >= 0x550))
#define PHOSPHOR_API __global
#elif defined __GNUC__
#define PHOSPHOR_API __attribute__((visibility("default")))
#elif defined(_MSC_VER)
#define PHOSPHOR_API __declspec(dllexport)
#else
#define PHOSPHOR_API
#endif
