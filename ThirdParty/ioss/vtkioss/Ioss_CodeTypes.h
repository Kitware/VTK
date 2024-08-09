// Copyright(C) 1999-2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "vtk_ioss_mangle.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

#if defined(WIN32) || defined(__WIN32__) || defined(_WIN32) || defined(_MSC_VER) ||                \
    defined(__MINGW32__) || defined(_WIN64) || defined(__MINGW64__)
#define __IOSS_WINDOWS__ 1
#endif

namespace Ioss {
  using IntVector   = std::vector<int>;
  using Int64Vector = std::vector<int64_t>;
  using NameList    = std::vector<std::string>;
  using IJK_t       = std::array<int, 3>;
} // namespace Ioss

inline std::string IOSS_SCALAR() { return {"scalar"}; }
inline std::string IOSS_VECTOR_2D() { return {"vector_2d"}; }
inline std::string IOSS_VECTOR_3D() { return {"vector_3d"}; }
inline std::string IOSS_SYM_TENSOR() { return {"sym_tensor_33"}; }

#if defined(BUILT_IN_SIERRA)
#define MAP_USE_SORTED_VECTOR
#define SEACAS_HAVE_MPI
/* #undef IOSS_THREADSAFE */
/* #undef SEACAS_HAVE_KOKKOS */
#define SEACAS_HAVE_EXODUS
#define SEACAS_HAVE_EXONULL
#define SEACAS_HAVE_CGNS
/* #undef SEACAS_HAVE_FAODEL */
#define SEACAS_HAVE_PAMGEN
#else
#include "SEACASIoss_config.h"
#endif

#if defined(IOSS_THREADSAFE)
#include <mutex>
#endif

#if (__cplusplus >= 201703L)
#define IOSS_MAYBE_UNUSED [[maybe_unused]]
#define IOSS_NODISCARD    [[nodiscard]]
#else
#define IOSS_MAYBE_UNUSED
#define IOSS_NODISCARD
#endif

#if defined(SEACAS_HAVE_MPI)
#include <vtk_mpi.h>
using Ioss_MPI_Comm = MPI_Comm;
#define IOSS_PAR_UNUSED(x)
#define ADIOS2_USE_MPI 1
#else
using Ioss_MPI_Comm = int;
#if (__cplusplus >= 201703L)
// For C++17, we rely on IOSS_MAYBE_UNUSED instead.  Can eventually remove all IOSS_PAR_UNUSED...
#define IOSS_PAR_UNUSED(x)
#else
#define IOSS_PAR_UNUSED(x)                                                                         \
  do {                                                                                             \
    (void)(x);                                                                                     \
  } while (0)
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
using Complex = std::complex<double>;
#ifdef SEACAS_HAVE_KOKKOS
using Kokkos_Complex = Kokkos::complex<double>;
#endif
#endif

#if defined(IOSS_THREADSAFE)
#define IOSS_FUNC_ENTER(m) std::lock_guard<std::mutex> guard(m)
#else

#if defined IOSS_TRACE
#include "Ioss_Tracer.h"
#define IOSS_FUNC_ENTER(m) Ioss::Tracer m(__func__)
#else
#define IOSS_FUNC_ENTER(m)
#endif
#endif

#ifndef IOSS_DEBUG_OUTPUT
#define IOSS_DEBUG_OUTPUT 0
#endif

// For use to create a no-op get or put_field_internal function...
#define IOSS_NOOP_GFI(type)                                                                        \
  int64_t get_field_internal(const type *, const Ioss::Field &, void *, size_t) const override     \
  {                                                                                                \
    return -1;                                                                                     \
  }
#define IOSS_NOOP_PFI(type)                                                                        \
  int64_t put_field_internal(const type *, const Ioss::Field &, void *, size_t) const override     \
  {                                                                                                \
    return -1;                                                                                     \
  }
