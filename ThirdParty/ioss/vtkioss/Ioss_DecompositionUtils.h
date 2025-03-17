/*
 * Copyright(C) 2024 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
#pragma once

#include "Ioss_ChainGenerator.h"
#include "Ioss_CodeTypes.h"
#include "Ioss_Region.h"
#include <string>
#include <vector>

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

namespace Ioss {
  class IOSS_EXPORT DecompUtils
  {
  public:
    static void output_histogram(const std::vector<size_t> &proc_work, double avg_work,
                                 size_t median);

    template <typename INT>
    static void line_decomp_modify(const Ioss::chain_t<INT> &element_chains,
                                   std::vector<int> &element_to_proc, int proc_count);

    static std::vector<size_t> get_work_per_rank(const std::vector<int> &elem_to_proc,
                                                 int                     proc_count);
    static std::pair<double, size_t>
    output_decomposition_statistics(const std::vector<size_t> work_per_rank);

    template <typename INT>
    static std::vector<float> line_decomp_weights(const Ioss::chain_t<INT> &element_chains,
                                                  size_t                    element_count);

    template <typename INT>
    static void line_decompose(Region &region, size_t num_ranks, const std::string &method,
                               const std::string &surface_list, std::vector<int> &element_to_proc,
                               INT dummy);

    template <typename INT>
    static void decompose_zoltan(const Ioss::Region &region, int ranks, const std::string &method,
                                 std::vector<int> &elem_to_proc, const std::vector<float> &weights,
                                 bool ignore_x, bool ignore_y, bool ignore_z,
                                 IOSS_MAYBE_UNUSED INT dummy);

    template <typename INT>
    static std::tuple<std::vector<double>, std::vector<double>, std::vector<double>>
    get_element_centroid(const Ioss::Region &region, IOSS_MAYBE_UNUSED INT dummy);
  };
} // namespace Ioss
