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
// The return value points to a newly-created VTK object.
#define VTK_NEWINSTANCE [[vtk::newinstance]]
// The parameter is a pointer to a zerocopy buffer.
#define VTK_ZEROCOPY [[vtk::zerocopy]]
// The parameter is a path on the filesystem.
#define VTK_FILEPATH [[vtk::filepath]]
// Set preconditions for a function
#define VTK_EXPECTS(x) [[vtk::expects(x)]]
// Set size hint for parameter or return value
#define VTK_SIZEHINT(...) [[vtk::sizehint(__VA_ARGS__)]]
#endif

#ifndef VTK_WRAP_HINTS_DEFINED
#define VTK_WRAPEXCLUDE
#define VTK_NEWINSTANCE
#define VTK_ZEROCOPY
#define VTK_FILEPATH
#define VTK_EXPECTS(x)
#define VTK_SIZEHINT(...)
#endif

#endif
// VTK-HeaderTest-Exclude: vtkWrappingHints.h
