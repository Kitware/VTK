// Copyright(C) 2022 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include <string>
#include <vector>

#include "Ioss_Region.h"

namespace Ioss {
  template <typename INT> struct chain_entry_t
  {
    chain_entry_t() = default;
    chain_entry_t(int64_t el, int lnk) : element(el), link(lnk) {}
    bool    operator==(const chain_entry_t<INT> &other) { return (element == other.element); }
    int64_t element{}; // Element at root of chain
    int     link{};    // How far is this element in the chain (1-based)
  };

  template <typename INT> using chain_t = std::vector<chain_entry_t<INT>>;

  template <typename INT>
  chain_t<INT> generate_element_chains(Ioss::Region &region, const std::string &surface_list,
                                       int debug_level, INT /*dummy*/);
} // namespace Ioss
