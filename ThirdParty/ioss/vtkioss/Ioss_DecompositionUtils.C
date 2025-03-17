/*
 * Copyright(C) 2024 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */

#include <map>
#include <numeric>
#include <string>
#include <vector>

#include "Ioss_ChainGenerator.h"
#include "Ioss_CodeTypes.h"
#include "Ioss_DecompositionUtils.h"
#include "Ioss_ElementBlock.h"
#include "Ioss_NodeBlock.h"
#include "Ioss_ParallelUtils.h"
#include "Ioss_Region.h"
#include "Ioss_SmartAssert.h"

#include "vtk_fmt.h"
#include VTK_FMT(fmt/color.h)
#include VTK_FMT(fmt/format.h)
#include VTK_FMT(fmt/ostream.h)
#include VTK_FMT(fmt/ranges.h)

#if !defined(NO_ZOLTAN_SUPPORT)
#include <zoltan.h>     // for Zoltan_Initialize
#include <zoltan_cpp.h> // for Zoltan
#endif

namespace {
#if !defined(NO_ZOLTAN_SUPPORT)
  /*****************************************************************************/
  /***** Global data structure used by Zoltan callbacks.                   *****/
  /***** Could implement Zoltan callbacks without global data structure,   *****/
  /***** but using the global data structure makes implementation quick.   *****/
  struct
  {
    size_t  ndot; /* Length of x, y, z, and part (== # of elements) */
    float  *vwgt; /* vertex weights */
    double *x;    /* x-coordinates */
    double *y;    /* y-coordinates */
    double *z;    /* z-coordinates */
  } Zoltan_Data;

  /*****************************************************************************/
  /***** ZOLTAN CALLBACK FUNCTIONS *****/
  int zoltan_num_dim(void * /*data*/, int *ierr)
  {
    /* Return dimensionality of coordinate data.
     * Using global data structure Zoltan_Data, initialized in ZOLTAN_RCB_assign.
     */
    *ierr = ZOLTAN_OK;
    if (Zoltan_Data.z != nullptr) {
      return 3;
    }
    if (Zoltan_Data.y != nullptr) {
      return 2;
    }
    return 1;
  }

  int zoltan_num_obj(void * /*data*/, int *ierr)
  {
    /* Return number of objects.
     * Using global data structure Zoltan_Data, initialized in ZOLTAN_RCB_assign.
     */
    *ierr = ZOLTAN_OK;
    return Zoltan_Data.ndot;
  }

  void zoltan_obj_list(void * /*data*/, int /*ngid_ent*/, int /*nlid_ent*/, ZOLTAN_ID_PTR gids,
                       ZOLTAN_ID_PTR /*lids*/, int wdim, float *wgts, int *ierr)
  {
    /* Return list of object IDs.
     * Return only global IDs; don't need local IDs since running in serial.
     * gids are array indices for coordinate and vwgts arrays.
     * Using global data structure Zoltan_Data, initialized in ZOLTAN_RCB_assign.
     */
    std::iota(gids, gids + Zoltan_Data.ndot, 0);
    if (wdim != 0) {
      for (size_t i = 0; i < Zoltan_Data.ndot; i++) {
        wgts[i] = static_cast<float>(Zoltan_Data.vwgt[i]);
      }
    }

    *ierr = ZOLTAN_OK;
  }

  void zoltan_geom(void * /*data*/, int /*ngid_ent*/, int /*nlid_ent*/, int nobj,
                   const ZOLTAN_ID_PTR gids, ZOLTAN_ID_PTR /*lids*/, int ndim, double *geom,
                   int *ierr)
  {
    /* Return coordinates for objects.
     * gids are array indices for coordinate arrays.
     * Using global data structure Zoltan_Data, initialized in ZOLTAN_RCB_assign.
     */

    for (size_t i = 0; i < static_cast<size_t>(nobj); i++) {
      size_t j       = gids[i];
      geom[i * ndim] = Zoltan_Data.x[j];
      if (ndim > 1) {
        geom[i * ndim + 1] = Zoltan_Data.y[j];
      }
      if (ndim > 2) {
        geom[i * ndim + 2] = Zoltan_Data.z[j];
      }
    }

    *ierr = ZOLTAN_OK;
  }
#endif

  template <typename INT>
  std::map<INT, std::vector<INT>> string_chains(const Ioss::chain_t<INT> &element_chains)
  {
    std::map<INT, std::vector<INT>> chains;

    for (size_t i = 0; i < element_chains.size(); i++) {
      auto &chain_entry = element_chains[i];
      if (chain_entry.link >= 0) {
        chains[chain_entry.element].push_back(i + 1);
      }
    }
    return chains;
  }
} // namespace

namespace Ioss {
  void DecompUtils::output_histogram(const std::vector<size_t> &proc_work, double avg_work,
                                     size_t median)
  {
    fmt::print("Work-per-processor Histogram:\n");
    std::array<size_t, 16> histogram{};

    auto wmin = *std::min_element(proc_work.begin(), proc_work.end());
    auto wmax = *std::max_element(proc_work.begin(), proc_work.end());

    size_t hist_size = std::min(size_t(16), (wmax - wmin));
    hist_size        = std::min(hist_size, proc_work.size());

    if (hist_size <= 1) {
      fmt::print("\tWork is the same on all processors; no histogram needed.\n\n");
      return;
    }

    auto delta = double(wmax + 1 - wmin) / hist_size;
    for (const auto &pw : proc_work) {
      auto bin = size_t(double(pw - wmin) / delta);
      SMART_ASSERT(bin < hist_size)(bin)(hist_size);
      histogram[bin]++;
    }

    size_t proc_width = Ioss::Utils::number_width(proc_work.size(), true);
    size_t work_width = Ioss::Utils::number_width(wmax, true);

    fmt::print("\n\t{:^{}} {:^{}}\n", "Work Range", 2 * work_width + 2, "#", proc_width);
    auto hist_max = *std::max_element(histogram.begin(), histogram.end());
    for (size_t i = 0; i < hist_size; i++) {
      int         max_star = 50;
      int         star_cnt = ((double)histogram[i] / hist_max * max_star);
      std::string stars(star_cnt, '*');
      for (int j = 9; j < star_cnt;) {
        stars[j] = '|';
        j += 10;
      }
      if (histogram[i] > 0 && star_cnt == 0) {
        stars = '.';
      }
      size_t      w1 = wmin + size_t(i * delta);
      size_t      w2 = wmin + size_t((i + 1) * delta);
      std::string postfix;
      if (w1 <= avg_work && avg_work < w2) {
        postfix += "average";
      }
      if (w1 <= median && median < w2) {
        if (!postfix.empty()) {
          postfix += ", ";
        }
        postfix += "median";
      }
      fmt::print("\t{:{}}..{:{}} ({:{}}):\t{:{}}  {}\n", fmt::group_digits(w1), work_width,
                 fmt::group_digits(w2), work_width, fmt::group_digits(histogram[i]), proc_width,
                 stars, max_star, postfix);
    }
    fmt::print("\n");
  }

  template IOSS_EXPORT void
  DecompUtils::decompose_zoltan(const Ioss::Region &region, int ranks, const std::string &method,
                                std::vector<int> &elem_to_proc, const std::vector<float> &weights,
                                bool ignore_x, bool ignore_y, bool ignore_z,
                                IOSS_MAYBE_UNUSED int dummy);
  template IOSS_EXPORT void
  DecompUtils::decompose_zoltan(const Ioss::Region &region, int ranks, const std::string &method,
                                std::vector<int> &elem_to_proc, const std::vector<float> &weights,
                                bool ignore_x, bool ignore_y, bool ignore_z,
                                IOSS_MAYBE_UNUSED int64_t dummy);

  template <typename INT>
  void DecompUtils::decompose_zoltan(const Ioss::Region &region, int ranks,
                                     const std::string &method, std::vector<int> &elem_to_proc,
                                     const std::vector<float> &weights, bool ignore_x,
                                     bool ignore_y, bool ignore_z, IOSS_MAYBE_UNUSED INT dummy)
  {
#if defined(NO_ZOLTAN_SUPPORT)
    fmt::print(stderr, "ERROR: Zoltan library not enabled in this version of slice.\n"
                       "       The 'rcb', 'rib', and 'hsfc' methods are not available.\n\n");
    std::exit(1);
#else
    if (ranks == 1) {
      return;
    }

    size_t element_count = region.get_property("element_count").get_int();
    if (element_count != static_cast<size_t>(static_cast<int>(element_count))) {
      fmt::print(stderr, "ERROR: Cannot have a mesh with more than 2.1 Billion elements in a "
                         "Zoltan decomposition.\n");
      exit(EXIT_FAILURE);
    }

    auto [x, y, z] = Ioss::DecompUtils::get_element_centroid(region, dummy);

    // Copy mesh data and pointers into structure accessible from callback fns.
    Zoltan_Data.ndot = element_count;
    Zoltan_Data.vwgt = const_cast<float *>(Data(weights));

    if (ignore_x && ignore_y) {
      x.clear();
      y.clear();
      Zoltan_Data.x = Data(z);
    }
    else if (ignore_x && ignore_z) {
      x.clear();
      z.clear();
      Zoltan_Data.x = Data(y);
    }
    else if (ignore_y && ignore_z) {
      y.clear();
      z.clear();
      Zoltan_Data.x = Data(x);
    }
    else if (ignore_x) {
      x.clear();
      Zoltan_Data.x = Data(y);
      Zoltan_Data.y = Data(z);
    }
    else if (ignore_y) {
      y.clear();
      Zoltan_Data.x = Data(x);
      Zoltan_Data.y = Data(z);
    }
    else if (ignore_z) {
      z.clear();
      Zoltan_Data.x = Data(x);
      Zoltan_Data.y = Data(y);
    }
    else {
      Zoltan_Data.x = Data(x);
      Zoltan_Data.y = Data(y);
      Zoltan_Data.z = Data(z);
    }

    // Initialize Zoltan
    int    argc = 0;
    char **argv = nullptr;

    float ver = 0.0;
    Zoltan_Initialize(argc, argv, &ver);
    fmt::print("\tUsing Zoltan version {:.2}, method {}\n", static_cast<double>(ver), method);

    Zoltan zz(Ioss::ParallelUtils::comm_self());

    // Register Callback functions
    // Using global Zoltan_Data; could register it here instead as data field.
    zz.Set_Num_Obj_Fn(zoltan_num_obj, nullptr);
    zz.Set_Obj_List_Fn(zoltan_obj_list, nullptr);
    zz.Set_Num_Geom_Fn(zoltan_num_dim, nullptr);
    zz.Set_Geom_Multi_Fn(zoltan_geom, nullptr);

    // Set parameters for Zoltan
    zz.Set_Param("DEBUG_LEVEL", "0");
    std::string str = fmt::format("{}", ranks);
    zz.Set_Param("NUM_GLOBAL_PARTS", str);
    zz.Set_Param("OBJ_WEIGHT_DIM", "1");
    zz.Set_Param("LB_METHOD", method);
    zz.Set_Param("NUM_LID_ENTRIES", "0");
    zz.Set_Param("REMAP", "0");
    zz.Set_Param("RETURN_LISTS", "PARTITION_ASSIGNMENTS");
    zz.Set_Param("RCB_RECTILINEAR_BLOCKS", "1");

    int num_global = sizeof(INT) / sizeof(ZOLTAN_ID_TYPE);
    num_global     = num_global < 1 ? 1 : num_global;

    // Call partitioner
    int           changes           = 0;
    int           num_local         = 0;
    int           num_import        = 1;
    int           num_export        = 1;
    ZOLTAN_ID_PTR import_global_ids = nullptr;
    ZOLTAN_ID_PTR import_local_ids  = nullptr;
    ZOLTAN_ID_PTR export_global_ids = nullptr;
    ZOLTAN_ID_PTR export_local_ids  = nullptr;
    int          *import_procs      = nullptr;
    int          *import_to_part    = nullptr;
    int          *export_procs      = nullptr;
    int          *export_to_part    = nullptr;
    int rc = zz.LB_Partition(changes, num_global, num_local, num_import, import_global_ids,
                             import_local_ids, import_procs, import_to_part, num_export,
                             export_global_ids, export_local_ids, export_procs, export_to_part);

    if (rc != ZOLTAN_OK) {
      fmt::print(stderr, "ERROR: Problem during call to Zoltan LB_Partition.\n");
      goto End;
    }

    // Sanity check
    if (element_count != static_cast<size_t>(num_export)) {
      fmt::print(stderr, "Sanity check failed; ndot {} != num_export {}.\n", element_count,
                 static_cast<size_t>(num_export));
      goto End;
    }

    elem_to_proc.resize(element_count);
    for (size_t i = 0; i < element_count; i++) {
      elem_to_proc[i] = export_to_part[i];
    }

  End:
    /* Clean up */
    Zoltan::LB_Free_Part(&import_global_ids, &import_local_ids, &import_procs, &import_to_part);
    Zoltan::LB_Free_Part(&export_global_ids, &export_local_ids, &export_procs, &export_to_part);
#endif
  }

  template <typename INT>
  void DecompUtils::line_decompose(Region &region, size_t num_ranks, const std::string &method,
                                   const std::string &surface_list,
                                   std::vector<int> &element_to_proc, INT dummy)
  {

    Ioss::chain_t<INT> element_chains =
        Ioss::generate_element_chains(region, surface_list, 0, dummy);
    region.get_database()->progress("Ioss::generate_element_chains");

    std::vector<float> weights =
        line_decomp_weights(element_chains, region.get_property("element_count").get_int());
    region.get_database()->progress("generate_element_weights");

    double start = Ioss::Utils::timer();
    decompose_zoltan(region, num_ranks, method, element_to_proc, weights, false, false, false,
                     dummy);
    double end = Ioss::Utils::timer();
    fmt::print(stderr, "\tDecompose elements = {:.5}\n", end - start);
    region.get_database()->progress("exit decompose_elements");

    // Make sure all elements on a chain are on the same processor rank...
    line_decomp_modify(element_chains, element_to_proc, num_ranks);
  }

  template IOSS_EXPORT void DecompUtils::line_decompose(Region &region, size_t num_ranks,
                                                        const std::string &method,
                                                        const std::string &surface_list,
                                                        std::vector<int>  &element_to_proc,
                                                        int                dummy);
  template IOSS_EXPORT void DecompUtils::line_decompose(Region &region, size_t num_ranks,
                                                        const std::string &method,
                                                        const std::string &surface_list,
                                                        std::vector<int>  &element_to_proc,
                                                        int64_t            dummy);

  template <typename INT>
  std::vector<float> DecompUtils::line_decomp_weights(const Ioss::chain_t<INT> &element_chains,
                                                      size_t                    element_count)
  {
    int  debug_level = 0;
    auto chains      = string_chains(element_chains);

    if ((debug_level & 16) != 0) {
      for (const auto &[chain_root, chain_elements] : chains) {
        fmt::print("Chain Root: {} contains: {}\n", chain_root, fmt::join(chain_elements, ", "));
      }
    }

    std::vector<float> weights(element_count, 1);
    // Now, for each chain...
    for (const auto &[chain_root, chain_elements] : chains) {
      // * Set the weights of all elements in the chain...
      // * non-root = 0, root = length of chain.
      for (const auto &element : chain_elements) {
        weights[element - 1] = 0;
      }
      weights[chain_root - 1] = static_cast<float>(chain_elements.size());
    }
    return weights;
  }
  template IOSS_EXPORT std::vector<float>
  DecompUtils::line_decomp_weights(const Ioss::chain_t<int> &element_chains, size_t element_count);
  template IOSS_EXPORT std::vector<float>
  DecompUtils::line_decomp_weights(const Ioss::chain_t<int64_t> &element_chains,
                                   size_t                        element_count);

  template <typename INT>
  void DecompUtils::line_decomp_modify(const Ioss::chain_t<INT> &element_chains,
                                       std::vector<int> &elem_to_proc, int proc_count)
  {
    int debug_level = 0;
    // Get a map of all chains and the elements in the chains.  Map key will be root.
    auto chains = string_chains(element_chains);

    // Delta: elements added/removed from each processor...
    std::vector<int> delta(proc_count);

    // Now, for each chain...
    for (const auto &[chain_root, chain_elements] : chains) {
      if ((debug_level & 16) != 0) {
        fmt::print("Chain Root: {} contains: {}\n", chain_root, fmt::join(chain_elements, ", "));
      }

      std::vector<INT> chain_proc_count(proc_count);

      // * get processors used by elements in the chain...
      for (const auto &element : chain_elements) {
        auto proc = elem_to_proc[element - 1];
        chain_proc_count[proc]++;
      }

      // * Now, subtract the `delta` from each count
      for (int i = 0; i < proc_count; i++) {
        chain_proc_count[i] -= delta[i];
      }

      // * Assign all elements in the chain to processor at chain root
      // * Update the deltas for all processors that gain/lose elements...
      auto root_proc = elem_to_proc[chain_root - 1];
      for (const auto &element : chain_elements) {
        if (elem_to_proc[element - 1] != root_proc) {
          auto old_proc             = elem_to_proc[element - 1];
          elem_to_proc[element - 1] = root_proc;
          delta[root_proc]++;
          delta[old_proc]--;
        }
      }
    }

    std::vector<INT> proc_element_count(proc_count);
    for (auto proc : elem_to_proc) {
      proc_element_count[proc]++;
    }
    if ((debug_level & 32) != 0) {
      fmt::print("\nElements/Processor: {}\n", fmt::join(proc_element_count, ", "));
      fmt::print("Delta/Processor:    {}\n", fmt::join(delta, ", "));
    }
  }

  template IOSS_EXPORT void
  DecompUtils::line_decomp_modify(const Ioss::chain_t<int> &element_chains,
                                  std::vector<int> &elem_to_proc, int proc_count);
  template IOSS_EXPORT void
  DecompUtils::line_decomp_modify(const Ioss::chain_t<int64_t> &element_chains,
                                  std::vector<int> &elem_to_proc, int proc_count);

  std::vector<size_t> DecompUtils::get_work_per_rank(const std::vector<int> &elem_to_proc,
                                                     int                     proc_count)
  {
    std::vector<size_t> work_per_rank(proc_count);
    for (int proc : elem_to_proc) {
      work_per_rank[proc]++;
    }
    return work_per_rank;
  }

  std::pair<double, size_t>
  DecompUtils::output_decomposition_statistics(const std::vector<size_t> work_per_rank)
  {
    size_t total_work = std::accumulate(work_per_rank.begin(), work_per_rank.end(), size_t(0));
    size_t proc_count = work_per_rank.size();
    size_t proc_width = Ioss::Utils::number_width(proc_count, false);
    size_t work_width = Ioss::Utils::number_width(total_work, true);

    auto   min_work = *std::min_element(work_per_rank.begin(), work_per_rank.end());
    auto   max_work = *std::max_element(work_per_rank.begin(), work_per_rank.end());
    double avg_work = 0.0;
    size_t median   = 0;
    {
      auto pw_copy(work_per_rank);
      std::nth_element(pw_copy.begin(), pw_copy.begin() + pw_copy.size() / 2, pw_copy.end());
      median = pw_copy[pw_copy.size() / 2];
      fmt::print("\nWork per processor:\n\tMinimum = {}, Maximum = {}, Median = {}, Ratio = "
                 "{:.3}\n\n",
                 fmt::group_digits(min_work), fmt::group_digits(max_work),
                 fmt::group_digits(median), (double)(max_work) / min_work);
    }
    if (min_work == max_work) {
      fmt::print("Work on all processors is {}\n\n", fmt::group_digits(min_work));
    }
    else {
      int max_star = 40;
      int min_star = max_star * ((double)min_work / (double)(max_work));
      min_star     = std::max(1, min_star);
      int delta    = max_star - min_star;

      avg_work = (double)total_work / (double)proc_count;
      for (size_t i = 0; i < work_per_rank.size(); i++) {
        int star_cnt =
            (double)(work_per_rank[i] - min_work) / (max_work - min_work) * delta + min_star;
        std::string stars(star_cnt, '*');
        auto tmp = fmt::format(fmt::runtime("\tProcessor {:{}}, work = {:{}}  ({:.2f})\t{}\n"), i,
                               proc_width, fmt::group_digits(work_per_rank[i]), work_width,
                               work_per_rank[i] / avg_work, stars);

#if !defined __NVCC__
        if (work_per_rank[i] == max_work) {
          fmt::print("{}", fmt::styled(tmp, fmt::fg(fmt::color::red)));
        }
        else if (work_per_rank[i] == min_work) {
          fmt::print("{}", fmt::styled(tmp, fmt::fg(fmt::color::green)));
        }
        else {
          fmt::print("{}", tmp);
        }
#else
        fmt::print("{}", tmp);
#endif
      }
    }

    // Imbalance penalty -- max work / avg work.  If perfect balance, then all processors would have
    // "avg_work" work to do. With current decomposition, every processor has to wait until
    // "max_work" is done.  Penalty = max_work / avg_work.
    fmt::print("\nImbalance Penalty:\n\tMaximum Work = {}, Average Work = {}, Penalty (max/avg) "
               "= {:.2f}\n\n",
               fmt::group_digits(max_work), fmt::group_digits((size_t)avg_work),
               (double)max_work / avg_work);

    return std::make_pair(avg_work, median);
  }

  template <typename INT>
  std::tuple<std::vector<double>, std::vector<double>, std::vector<double>>
  DecompUtils::get_element_centroid(const Ioss::Region &region, IOSS_MAYBE_UNUSED INT dummy)
  {
    size_t element_count = region.get_property("element_count").get_int();

    // The zoltan methods supported in slice are all geometry based
    // and use the element centroid.
    std::vector<double> x(element_count);
    std::vector<double> y(element_count);
    std::vector<double> z(element_count);

    const auto         *nb = region.get_node_blocks()[0];
    std::vector<double> coor;
    nb->get_field_data("mesh_model_coordinates", coor);

    const auto &blocks = region.get_element_blocks();
    size_t      el     = 0;
    for (auto &eb : blocks) {
      std::vector<INT> connectivity;
      eb->get_field_data("connectivity_raw", connectivity);
      size_t blk_element_count = eb->entity_count();
      size_t blk_element_nodes = eb->topology()->number_nodes();

      for (size_t j = 0; j < blk_element_count; j++) {
        for (size_t k = 0; k < blk_element_nodes; k++) {
          auto node = connectivity[j * blk_element_nodes + k] - 1;
          x[el] += coor[node * 3 + 0];
          y[el] += coor[node * 3 + 1];
          z[el] += coor[node * 3 + 2];
        }
        x[el] /= blk_element_nodes;
        y[el] /= blk_element_nodes;
        z[el] /= blk_element_nodes;
        el++;
      }
    }
    return {x, y, z};
  }

  template IOSS_EXPORT std::tuple<std::vector<double>, std::vector<double>, std::vector<double>>
  DecompUtils::get_element_centroid(const Ioss::Region &region, IOSS_MAYBE_UNUSED int dummy);

  template IOSS_EXPORT std::tuple<std::vector<double>, std::vector<double>, std::vector<double>>
  DecompUtils::get_element_centroid(const Ioss::Region &region, IOSS_MAYBE_UNUSED int64_t dummy);

} // namespace Ioss
