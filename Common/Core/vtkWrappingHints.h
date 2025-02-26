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

#ifdef __VTK_WRAP__
#define VTK_WRAP_HINTS_DEFINED
// Exclude a method or class from wrapping
#define VTK_WRAPEXCLUDE [[vtk::wrapexclude]]
// Tell wrappers not to associate this method with any property.
#define VTK_PROPEXCLUDE [[vtk::propexclude]]
// The return value points to a newly-created VTK object.
#define VTK_NEWINSTANCE [[vtk::newinstance]]
// The parameter is a pointer to a zerocopy buffer.
#define VTK_ZEROCOPY [[vtk::zerocopy]]
// The parameter is a path on the filesystem.
#define VTK_FILEPATH [[vtk::filepath]]
// Release Python GIL for the duration of the call
#define VTK_UNBLOCKTHREADS [[vtk::unblockthreads]]
// Set preconditions for a function
#define VTK_EXPECTS(x) [[vtk::expects(x)]]
// Set size hint for parameter or return value
#define VTK_SIZEHINT(...) [[vtk::sizehint(__VA_ARGS__)]]
// Opt-in a class for automatic code generation of (de)serializers.
#define VTK_MARSHALAUTO [[vtk::marshalauto]]
// Specifies that a class has hand written (de)serializers.
#define VTK_MARSHALMANUAL [[vtk::marshalmanual]]
// Excludes a function from the auto-generated (de)serialization process.
#define VTK_MARSHALEXCLUDE(reason) [[vtk::marshalexclude(reason)]]
// Enforces a function as the getter for `property`
#define VTK_MARSHALGETTER(property) [[vtk::marshalgetter(#property)]]
// Enforces a function as the setter for `property`
#define VTK_MARSHALSETTER(property) [[vtk::marshalsetter(#property)]]
#endif

#ifndef VTK_WRAP_HINTS_DEFINED
#define VTK_WRAPEXCLUDE
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
