// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWrappingHints
 * @brief   hint macros for wrappers
 *
 * The macros defined in this file can be used to supply hints for the
 * wrappers.
 */

#ifndef vtkWrappingHints_h
#define vtkWrappingHints_h

// When using Clang frontend, define VTK_WRAP_HINTS_USE_CLANG_ANNOTATE to use
// the annotate attribute instead of VTK custom C++ attributes.
// This is useful for external clang-based tool,
// since Clang will remove unknown attributes from its AST.
// When VTK_WRAP_HINTS_USE_CLANG_ANNOTATE is defined,
// `[[vtk::attr(value)]]` will become `annotate("vtk::attr(value)")`,
// which will appear as `AnnotateAttr "vtk::attr(value)"` in the AST.
#ifdef VTK_WRAP_HINTS_USE_CLANG_ANNOTATE
#define VTK_DEFINE_WRAP_HINT(name) __attribute__((annotate(#name)))
#define VTK_DEFINE_WRAP_HINT_ARGS(name, ...) __attribute__((annotate(#name "(" #__VA_ARGS__ ")")))
#else
#define VTK_DEFINE_WRAP_HINT(name) [[name]]
#define VTK_DEFINE_WRAP_HINT_ARGS(name, ...) [[name(__VA_ARGS__)]]
#endif

#ifdef __VTK_WRAP__
#define VTK_WRAP_HINTS_DEFINED
// Exclude a method or class from wrapping
#define VTK_WRAPEXCLUDE VTK_DEFINE_WRAP_HINT(vtk::wrapexclude)
// Exclude a method or class from JavaScript wrapping
#ifdef __EMSCRIPTEN__
#define VTK_WRAPEXCLUDE_JAVASCRIPT VTK_WRAPEXCLUDE
#else
#define VTK_WRAPEXCLUDE_JAVASCRIPT
#endif
// Tell wrappers not to associate this method with any property.
#define VTK_PROPEXCLUDE VTK_DEFINE_WRAP_HINT(vtk::propexclude)
// The return value points to a newly-created VTK object.
#define VTK_NEWINSTANCE VTK_DEFINE_WRAP_HINT(vtk::newinstance)
// The parameter is a pointer to a zerocopy buffer.
#define VTK_ZEROCOPY VTK_DEFINE_WRAP_HINT(vtk::zerocopy)
// The parameter is a path on the filesystem.
#define VTK_FILEPATH VTK_DEFINE_WRAP_HINT(vtk::filepath)
// Release Python GIL for the duration of the call
#define VTK_UNBLOCKTHREADS VTK_DEFINE_WRAP_HINT(vtk::unblockthreads)
// Set preconditions for a function
#define VTK_EXPECTS(x) VTK_DEFINE_WRAP_HINT_ARGS(vtk::expects, x)
// Set size hint for parameter or return value
#define VTK_SIZEHINT(...) VTK_DEFINE_WRAP_HINT_ARGS(vtk::sizehint, __VA_ARGS__)
// Opt-in a class for automatic code generation of (de)serializers.
#define VTK_MARSHALAUTO VTK_DEFINE_WRAP_HINT(vtk::marshalauto)
// Specifies that a class has hand written (de)serializers.
#define VTK_MARSHALMANUAL VTK_DEFINE_WRAP_HINT(vtk::marshalmanual)
// Excludes a function from the auto-generated (de)serialization process.
#define VTK_MARSHALEXCLUDE(reason) VTK_DEFINE_WRAP_HINT_ARGS(vtk::marshalexclude, reason)
// Enforces a function as the getter for `property`
#define VTK_MARSHALGETTER(property) VTK_DEFINE_WRAP_HINT_ARGS(vtk::marshalgetter, #property)
// Enforces a function as the setter for `property`
#define VTK_MARSHALSETTER(property) VTK_DEFINE_WRAP_HINT_ARGS(vtk::marshalsetter, #property)
#endif

#ifndef VTK_WRAP_HINTS_DEFINED
#define VTK_WRAPEXCLUDE
#define VTK_WRAPEXCLUDE_JAVASCRIPT
#define VTK_PROPEXCLUDE
#define VTK_NEWINSTANCE
#define VTK_ZEROCOPY
#define VTK_FILEPATH
#define VTK_UNBLOCKTHREADS
#define VTK_EXPECTS(x)
#define VTK_SIZEHINT(...)
#define VTK_MARSHALAUTO
#define VTK_MARSHALMANUAL
#define VTK_MARSHALEXCLUDE(reason)
#define VTK_MARSHALGETTER(property)
#define VTK_MARSHALSETTER(property)
#endif

#define VTK_MARSHAL_EXCLUDE_REASON_IS_REDUNDANT "is redundant"
#define VTK_MARSHAL_EXCLUDE_REASON_IS_INTERNAL "is internal"
#define VTK_MARSHAL_EXCLUDE_REASON_NOT_SUPPORTED                                                   \
  "(de)serialization is not supported for this type of property"

#endif
// VTK-HeaderTest-Exclude: vtkWrappingHints.h
