/// @file
/// @brief pre-C23 `typeof` support
///
/// Any code consuming this header should be prepared for `TYPEOF` to _not_ be
/// defined. This header opportunistically implements `TYPEOF` when there exists
/// a way to do it, but this is not true for all C17 compilers.
///
/// When transitioning to C23, this header should be removed.

#pragma once

#if defined(__GNUC__) || defined(_MSC_VER) // Clang, GCC, MSVC
#define TYPEOF(expr) __typeof__(expr)
#else
// no typeof support
#endif
