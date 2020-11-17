// Copyright(C) 1999-2017 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are
// met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above
//       copyright notice, this list of conditions and the following
//       disclaimer in the documentation and/or other materials provided
//       with the distribution.
//
//     * Neither the name of NTESS nor the names of its
//       contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
// OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
// SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
// LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
// THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
// (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
// OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

#ifndef IOSS_code_types_h
#define IOSS_code_types_h

#include "vtk_ioss_mangle.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#if defined(_MSC_VER)
#ifdef _WIN64
#define ssize_t __int64
#else
#define ssize_t long
#endif
#endif

namespace Ioss {
  using IntVector   = std::vector<int>;
  using Int64Vector = std::vector<int64_t>;
  using NameList    = std::vector<std::string>;
  using IJK_t       = std::array<int, 3>;
} // namespace Ioss

inline const std::string IOSS_SCALAR() { return std::string("scalar"); }
inline const std::string IOSS_VECTOR_2D() { return std::string("vector_2d"); }
inline const std::string IOSS_VECTOR_3D() { return std::string("vector_3d"); }
inline const std::string IOSS_SYM_TENSOR() { return std::string("sym_tensor_33"); }

#if defined(BUILT_IN_SIERRA)
#define SEACAS_HAVE_MPI
/* #undef IOSS_THREADSAFE */
/* #undef SEACAS_HAVE_KOKKOS */
/* #undef SEACAS_HAVE_DATAWAREHOUSE */
#define SEACAS_HAVE_EXODUS
#define SEACAS_HAVE_CGNS
#define SEACAS_HAVE_PAMGEN
#define PARALLEL_AWARE_EXODUS
#else
#include <SEACASIoss_config.h>
#endif

#if defined(PARALLEL_AWARE_EXODUS)
#ifndef SEACAS_HAVE_MPI
#define SEACAS_HAVE_MPI
#endif
#endif

#if defined(IOSS_THREADSAFE)
#include <mutex>
#endif

#if defined(SEACAS_HAVE_MPI)
#include <vtk_mpi.h>
#define PAR_UNUSED(x)
#else
#define PAR_UNUSED(x)                                                                              \
  do {                                                                                             \
    (void)(x);                                                                                     \
  } while (0)

#ifndef MPI_COMM_WORLD
#define MPI_COMM_WORLD 0
using MPI_Comm       = int;
#endif
#endif

#ifdef SEACAS_HAVE_KOKKOS
#include <Kokkos_Core.hpp> // for Kokkos::complex
#endif

#include <complex>
#if defined(FOUR_BYTE_REAL)
//'FOUR_BYTE_REAL' is a sierra macro which may or may not be defined
// when this header is compiled...
// If FOUR_BYTE_REAL is defined then we know we need float, otherwise
// stick with double.
using Complex = std::complex<float>;
#ifdef SEACAS_HAVE_KOKKOS
using Kokkos_Complex = Kokkos::complex<float>;
#endif
#else
using Complex        = std::complex<double>;
#ifdef SEACAS_HAVE_KOKKOS
using Kokkos_Complex = Kokkos::complex<double>;
#endif
#endif
#endif

#if defined(IOSS_THREADSAFE)
#define IOSS_FUNC_ENTER(m) std::lock_guard<std::mutex> guard(m)
#else

#if defined IOSS_TRACE
#include <Ioss_Tracer.h>
#define IOSS_FUNC_ENTER(m) Ioss::Tracer m(__func__)
#else
#define IOSS_FUNC_ENTER(m)
#endif
#endif

#ifndef IOSS_DEBUG_OUTPUT
#define IOSS_DEBUG_OUTPUT 0
#endif
