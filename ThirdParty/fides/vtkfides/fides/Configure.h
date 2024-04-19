//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef fides_Configure_H_
#define fides_Configure_H_

#if defined(_MSC_VER)
//MSVC 2015+ can use a clang frontend, so we want to label it only as MSVC
//and not MSVC and clang
#define FIDES_MSVC

#elif defined(__INTEL_COMPILER)
//Intel 14+ on OSX uses a clang frontend, so again we want to label them as
//intel only, and not intel and clang
#define FIDES_ICC

#elif defined(__PGI)
// PGI reports as GNUC as it generates the same ABI, so we need to check for
// it before gcc.
#define FIDES_PGI

#elif defined(__ibmxl__)
//Check for xl before GCC and clang, as xl claims it is many things
#define FIDES_XL

#elif defined(__clang__)
//Check for clang before GCC, as clang says it is GNUC since it has ABI
//compliance
#define FIDES_CLANG

#elif defined(__GNUC__)
// Several compilers pretend to be GCC but have minor differences. Try to
// compensate for that, by checking for those compilers first
#define FIDES_GCC
#endif

#if __cplusplus >= 201103L || (defined(FIDES_MSVC) && _MSC_VER >= 1900) || \
  (defined(FIDES_ICC) && defined(__INTEL_CXX11_MODE__))
#define FIDES_HAVE_CXX_11
#else
#error "Fides requires at least a C++11 compiler"
#endif

#endif //fides_Configure_H_
