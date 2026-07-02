/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *
 * Copyright by The HDF Group.                                               *
 * All rights reserved.                                                      *
 *                                                                           *
 * This file is part of HDF5.  The full HDF5 copyright notice, including     *
 * terms governing use, modification, and redistribution, is contained in    *
 * the LICENSE file, which can be found at the root of the source code       *
 * distribution tree, or in https://www.hdfgroup.org/licenses.               *
 * If you do not have access to either file, you may request a copy from     *
 * help@hdfgroup.org.                                                        *
 * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */

/*
 * Macros for suppressing warnings
 *
 * These macros can be used to suppress compiler warnings that are difficult
 * or impossible to engineer around. By suppressing these warnings, we make
 * it easier to identify warnings created by new code and can build with
 * -Werror in CI to ensure new warnings don't get added to the code.
 *
 * There are a few suppression macros that have been added to paper over Java
 * JNI quirks. These macros should never be used outside of the JNI code.
 *
 * USAGE:
 *
 * The macros are used in ON/OFF pairs. (i.e. H5_WARN_FOO_(ON|OFF)). To
 * suppress a warning, add the OFF macro before the offending line(s) of
 * code, and to turn it back on, add the ON macro.
 *
 *      H5_WARN_FOO_OFF
 *      H5_WARN_BAR_OFF
 *      code_that_raises_warnings();
 *      H5_WARN_BAR_ON
 *      H5_WARN_FOO_ON
 *
 * Since the warning macros work by pushing and popping diagnostic contexts,
 * warnings should be switched back on in reverse order. In practice, the ON
 * macro is just a generic pop, so it doesn't matter which one gets used or
 * in what order they get called, but using the wrong macro will make it
 * harder to reason about the code.
 *
 * Suppression macro pairs should span the minimum amount of code that
 * quiets the warning. Ideally, a single line of code. Compilers in the
 * past limited diagnostic pragmas to outside of functions, but this
 * is no longer the case in any compiler we care about.
 *
 * LIMITATIONS:
 *
 * The warning macros use compiler pragmas, which limits where the suppression
 * macros can be used. The most obvious limitation is that they often can't
 * be used in macros.
 *
 * ADDING A NEW MACRO:
 *
 * - First, ask yourself if we really need a new warning macro. Do your best
 *   to actually correct warnings and not just suppress them. If a compiler
 *   update starts raising a bunch of new warnings we are unlikely to fix,
 *   consider shutting down the warning flag globally using the compiler
 *   flags (like -Wno-foo-warning).
 *
 * - The macro should have a helpful comment about why the warning should
 *   be suppressed and not corrected.
 *
 * - The macro name should reflect the actual problem, not the name of the
 *   compiler warning option. Names are of the form H5_WARN_<THING>_(ON|OFF).
 *
 * - Be careful with the ifdefs. clang defines __GNUC__, for example, so
 *   you can't simply check for that if you have a gcc-specific warning.
 *   Check for compiler version numbers in the macros to avoid warnings
 *   about undefined diagnostics in older compilers.
 *
 * - Add the new macro to the list in the .clang-format file.
 */

#ifndef H5warnings_H
#define H5warnings_H

/* Macros for enabling/disabling particular gcc / clang warnings
 *
 * (see the following web-sites for more info:
 *      http://www.dbp-consulting.com/tutorials/SuppressingGCCWarnings.html
 *      http://gcc.gnu.org/onlinedocs/gcc/Diagnostic-Pragmas.html#Diagnostic-Pragmas
 *
 * _Pragma is C99 and should work with all compilers, though permitted
 * placement may vary
 *
 * "GCC" in the pragma works for both clang/llvm and gcc
 */
#define H5_WARN_JOINSTR(x, y) x y
#define H5_WARN_DO_PRAGMA(x)  _Pragma(#x)
#define H5_WARN_PRAGMA(x)     H5_WARN_DO_PRAGMA(GCC diagnostic x)

/* Allow suppression of compiler diagnostics unless H5_SHOW_ALL_WARNINGS is
 * defined
 */
/* clang-format off */
#ifdef H5_SHOW_ALL_WARNINGS
    #define H5_WARN_OFF(x)
    #define H5_WARN_ON(x)
#else
    #if (((__GNUC__ * 100) + __GNUC_MINOR__) >= 406) || defined(__clang__)
        /* gcc 4.6+ /clang */
        #define H5_WARN_OFF(x) H5_WARN_PRAGMA(push) H5_WARN_PRAGMA(ignored H5_WARN_JOINSTR("-W", x))
        #define H5_WARN_ON(x)  H5_WARN_PRAGMA(pop)
    #endif
#endif
/* clang-format on */

/*********************
 * SPECIFIC WARNINGS *
 *********************/

/* Suppress warnings about C11 extensions */
#if defined(__clang__)
#define H5_WARN_C11_EXTENSIONS_OFF H5_WARN_OFF("c11-extensions")
#define H5_WARN_C11_EXTENSIONS_ON  H5_WARN_ON("c11-extensions")
#else
#define H5_WARN_C11_EXTENSIONS_OFF
#define H5_WARN_C11_EXTENSIONS_ON
#endif

/* Suppress warnings about bad cast alignment. These should be corrected,
 * not suppressed in the main library, but might appear in test code.
 */
#if defined(__clang__) || defined(__GNUC__)
#define H5_WARN_CAST_ALIGNMENT_OFF H5_WARN_OFF("cast-align")
#define H5_WARN_CAST_ALIGNMENT_ON  H5_WARN_ON("cast-align")
#else
#define H5_WARN_CAST_ALIGNMENT_OFF
#define H5_WARN_CAST_ALIGNMENT_ON
#endif

/* Suppress warnings about casting away const */
#if defined(__clang__) || defined(__GNUC__)
#define H5_WARN_CAST_AWAY_CONST_OFF H5_WARN_OFF("cast-qual")
#define H5_WARN_CAST_AWAY_CONST_ON  H5_WARN_ON("cast-qual")
#else
#define H5_WARN_CAST_AWAY_CONST_OFF
#define H5_WARN_CAST_AWAY_CONST_ON
#endif

/* Suppress warnings about duplicate logic branches. These can
 * show up when we take different branches based on type sizes.
 * On some platforms, the type sizes will be the same, leading
 * to the warning.
 */
#if defined(__GNUC__) && !defined(__clang__)
#define H5_WARN_DUPLICATED_BRANCHES_OFF H5_WARN_OFF("duplicated-branches")
#define H5_WARN_DUPLICATED_BRANCHES_ON  H5_WARN_ON("duplicated-branches")
#else
#define H5_WARN_DUPLICATED_BRANCHES_OFF
#define H5_WARN_DUPLICATED_BRANCHES_ON
#endif

/* Suppress warnings about operations that reduce floating-point
 * precision. This is only a problem in a few places in the tests
 * where we do some number munging.
 */
#if defined(__clang__) || defined(__GNUC__)
#define H5_WARN_FLOAT_CONVERSION_OFF H5_WARN_OFF("float-conversion")
#define H5_WARN_FLOAT_CONVERSION_ON  H5_WARN_ON("float-conversion")
#else
#define H5_WARN_FLOAT_CONVERSION_OFF
#define H5_WARN_FLOAT_CONVERSION_ON
#endif

/* Suppress warnings about directly comparing floats. This is
 * typically where we are comparing a floating point value to
 * an exact value like 0.
 */
#if defined(__clang__) || defined(__GNUC__)
#define H5_WARN_FLOAT_EQUAL_OFF H5_WARN_OFF("float-equal")
#define H5_WARN_FLOAT_EQUAL_ON  H5_WARN_ON("float-equal")
#else
#define H5_WARN_FLOAT_EQUAL_OFF
#define H5_WARN_FLOAT_EQUAL_ON
#endif

/* Suppress warnings about using format strings that aren't string
 * literals
 */
#if defined(__clang__) || defined(__GNUC__)
#define H5_WARN_FORMAT_NONLITERAL_OFF H5_WARN_OFF("format-nonliteral")
#define H5_WARN_FORMAT_NONLITERAL_ON  H5_WARN_ON("format-nonliteral")
#else
#define H5_WARN_FORMAT_NONLITERAL_OFF
#define H5_WARN_FORMAT_NONLITERAL_ON
#endif

/* Suppress warnings about possible truncation in format strings */
#if defined(__GNUC__) && !defined(__clang__)
#define H5_WARN_FORMAT_TRUNCATION_OFF H5_WARN_OFF("format-truncation")
#define H5_WARN_FORMAT_TRUNCATION_ON  H5_WARN_ON("format-truncation")
#else
#define H5_WARN_FORMAT_TRUNCATION_OFF
#define H5_WARN_FORMAT_TRUNCATION_ON
#endif

/* Suppress warnings about implicit fallthrough. Currently,
 * clang is the only compiler that has trouble with this.
 */
#if defined(__clang__)
#define H5_WARN_IMPLICIT_FALLTHROUGH_OFF H5_WARN_OFF("implicit-fallthrough")
#define H5_WARN_IMPLICIT_FALLTHROUGH_ON  H5_WARN_ON("implicit-fallthrough")
#else
#define H5_WARN_IMPLICIT_FALLTHROUGH_OFF
#define H5_WARN_IMPLICIT_FALLTHROUGH_ON
#endif

/* Suppress warnings about large stack and frame objects. We
 * could also include -Wframe-larger-than, but all those warnings
 * have been squashed so it's not necessary at this time.
 */
#if defined(__clang__)
/* clang can only suppress warnings about oversize strings */
#define H5_WARN_LARGE_STACK_OBJECTS_OFF H5_WARN_OFF("overlength-strings")
#define H5_WARN_LARGE_STACK_OBJECTS_ON  H5_WARN_ON("overlength-strings")
#elif defined(__GNUC__)
/* gcc can suppress warnings about any oversize object */
#define H5_WARN_LARGE_STACK_OBJECTS_OFF H5_WARN_OFF("larger-than=")
#define H5_WARN_LARGE_STACK_OBJECTS_ON  H5_WARN_ON("larger-than=")
#else
#define H5_WARN_LARGE_STACK_OBJECTS_OFF
#define H5_WARN_LARGE_STACK_OBJECTS_ON
#endif

/* Suppress warnings about possible string overflow.
 *
 * This warning may have to be suppressed in MPI code where
 * MPI_STATUSES_IGNORE is passed as an MPI_Status array.
 *
 * https://github.com/pmodels/mpich/issues/5687
 */
#if defined(__GNUC__) && !defined(__clang__)
#define H5_WARN_MPI_STATUSES_IGNORE_OFF H5_WARN_OFF("stringop-overflow")
#define H5_WARN_MPI_STATUSES_IGNORE_ON  H5_WARN_ON("stringop-overflow")
#else
#define H5_WARN_MPI_STATUSES_IGNORE_OFF
#define H5_WARN_MPI_STATUSES_IGNORE_ON
#endif

/* Disable warnings concerning non-standard extensions, like F16 */
/* clang */
#if defined(__clang__)
#define H5_WARN_NONSTD_SUFFIX_OFF H5_WARN_OFF("pedantic")
#define H5_WARN_NONSTD_SUFFIX_ON  H5_WARN_ON("pedantic")
/* gcc 14+ */
#elif defined(__GNUC__) && __GNUC__ >= 14
#define H5_WARN_NONSTD_SUFFIX_OFF H5_WARN_OFF("c11-c23-compat")
#define H5_WARN_NONSTD_SUFFIX_ON  H5_WARN_ON("c11-c23-compat")
/* gcc 9-13 */
#elif defined(__GNUC__) && __GNUC__ >= 9
#define H5_WARN_NONSTD_SUFFIX_OFF                                                                            \
    H5_WARN_OFF("pedantic")                                                                                  \
    H5_WARN_OFF("c11-c2x-compat")
#define H5_WARN_NONSTD_SUFFIX_ON                                                                             \
    H5_WARN_ON("c11-c2x-compat")                                                                             \
    H5_WARN_ON("pedantic")
#else
/* Everything else */
#define H5_WARN_NONSTD_SUFFIX_OFF
#define H5_WARN_NONSTD_SUFFIX_ON
#endif

/* Suppress warnings about converting between function and object
 * pointers. This is technically incorrect but unavoidable in the
 * plugin code, since it uses dlsym() to load plugins. POSIX allows
 * this, so it's fine.
 */
#if defined(__GNUC__) && !defined(__clang__)
#define H5_WARN_OBJ_FXN_POINTER_CONVERSION_OFF H5_WARN_OFF("pedantic")
#define H5_WARN_OBJ_FXN_POINTER_CONVERSION_ON  H5_WARN_ON("pedantic")
#else
#define H5_WARN_OBJ_FXN_POINTER_CONVERSION_OFF
#define H5_WARN_OBJ_FXN_POINTER_CONVERSION_ON
#endif

/* Suppress warnings about comparisons that will always be true or
 * false, typically when checking for negative values w/ unsigned
 * types.
 */
#if defined(__clang__) || defined(__GNUC__)
#define H5_WARN_USELESS_COMPARISON_OFF H5_WARN_OFF("type-limits")
#define H5_WARN_USELESS_COMPARISON_ON  H5_WARN_ON("type-limits")
#else
#define H5_WARN_USELESS_COMPARISON_OFF
#define H5_WARN_USELESS_COMPARISON_ON
#endif

/* Suppress warnings about functions that return structs. This
 * warning should only be suppressed when it's known that the
 * structures are very small.
 */
#if defined(__clang__) || defined(__GNUC__)
#define H5_WARN_AGGREGATE_RETURN_OFF H5_WARN_OFF("aggregate-return")
#define H5_WARN_AGGREGATE_RETURN_ON  H5_WARN_ON("aggregate-return")
#else
#define H5_WARN_AGGREGATE_RETURN_OFF
#define H5_WARN_AGGREGATE_RETURN_ON
#endif

/*********************
 * JAVA JNI WARNINGS *
 *********************/

/* These warning suppression macros are ONLY used in the Java wrappers
 * and are due to quirks of the JNI interface. In normal HDF5 code,
 * these warnings should be corrected, not suppressed.
 */

/* Suppress warnings about missing prototypes */
#if defined(__clang__) || defined(__GNUC__)
#define H5_WARN_MISSING_PROTOTYPE_OFF H5_WARN_OFF("missing-prototypes")
#define H5_WARN_MISSING_PROTOTYPE_ON  H5_WARN_ON("missing-prototypes")
#else
#define H5_WARN_MISSING_PROTOTYPE_OFF
#define H5_WARN_MISSING_PROTOTYPE_ON
#endif

/* Suppress warnings about signed/unsigned conversions */
#if defined(__clang__) || defined(__GNUC__)
#define H5_WARN_SIGN_CONVERSION_OFF H5_WARN_OFF("sign-conversion")
#define H5_WARN_SIGN_CONVERSION_ON  H5_WARN_ON("sign-conversion")
#else
#define H5_WARN_SIGN_CONVERSION_OFF
#define H5_WARN_SIGN_CONVERSION_ON
#endif

/* Suppress warnings about unused parameters */
#if defined(__clang__) || defined(__GNUC__)
#define H5_WARN_UNUSED_PARAMETER_OFF H5_WARN_OFF("unused-parameter")
#define H5_WARN_UNUSED_PARAMETER_ON  H5_WARN_ON("unused-parameter")
#else
#define H5_WARN_UNUSED_PARAMETER_OFF
#define H5_WARN_UNUSED_PARAMETER_ON
#endif

#endif /* H5warnings_H */
