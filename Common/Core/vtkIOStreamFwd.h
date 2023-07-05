// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkIOStreamFwd
 * @brief   Forward-declare C++ iostreams as used by VTK.
 *
 * This header forward-declares the proper streams.
 */

#ifndef vtkIOStreamFwd_h
#define vtkIOStreamFwd_h

#ifdef _MSC_VER
#pragma warning(push, 3)
#endif

// Forward-declare ansi streams.
#include <iosfwd>
using std::filebuf;
using std::fstream;
using std::ios;
using std::iostream;
using std::istream;
using std::ostream;
using std::streambuf;

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#endif // vtkIOStreamFwd_h
// VTK-HeaderTest-Exclude: vtkIOStreamFwd.h
