// Copyright(C) 2020, 2021, 2022, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

#include "Ioss_CodeTypes.h"
#include <cstdlib>
#include <vector>

namespace Ioss {
  struct IOSS_EXPORT DataPool
  {
    // Data space shared by most field input/output routines...
    std::vector<char> data{};
#ifdef SEACAS_HAVE_KOKKOS
    Kokkos::View<char *>    data_view_char{};
    Kokkos::View<int *>     data_view_int{};
    Kokkos::View<int64_t *> data_view_int64{};
    Kokkos::View<double *>  data_view_double{};
    // Kokkos::View<Kokkos_Complex *> data_view_complex cannot be a global variable,
    // Since Kokkos::initialize() has not yet been called. Also, a Kokkos:View cannot
    // have type std::complex entities.
    Kokkos::View<char **>    data_view_2D_char{};
    Kokkos::View<int **>     data_view_2D_int{};
    Kokkos::View<int64_t **> data_view_2D_int64{};
    Kokkos::View<double **>  data_view_2D_double{};
    // Kokkos::View<Kokkos_Complex **> data_view_2D_complex cannot be a global variable,
    // Since Kokkos::initialize() has not yet been called. Also, a Kokkos:View cannot
    // have type std::complex entities.
    Kokkos::View<char **, Kokkos::LayoutRight, Kokkos::HostSpace> data_view_2D_char_layout_space{};
    Kokkos::View<int **, Kokkos::LayoutRight, Kokkos::HostSpace>  data_view_2D_int_layout_space{};
    Kokkos::View<int64_t **, Kokkos::LayoutRight, Kokkos::HostSpace>
        data_view_2D_int64_layout_space{};
    Kokkos::View<double **, Kokkos::LayoutRight, Kokkos::HostSpace>
        data_view_2D_double_layout_space{};
    // Kokkos::View<Kokkos_Complex **, Kokkos::LayoutRight, Kokkos::HostSpace>
    // data_view_2D_complex_layout_space cannot be a global variable,
    // Since Kokkos::initialize() has not yet been called. Also, a Kokkos:View cannot
    // have type std::complex entities.
#endif
  };
} // namespace Ioss
