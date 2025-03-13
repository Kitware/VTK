// Copyright(C) 2022, 2024 National Technology & Engineering Solutions
// of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
// NTESS, the U.S. Government retains certain rights in this software.
//
// See packages/seacas/LICENSE for details

#pragma once

#include <stdint.h>
#include <string>
#include <vector>

#include "Ioss_Region.h"

namespace Ioss {
  class Region;

  template <typename INT> struct chain_entry_t
  {
    IOSS_NODISCARD bool operator==(const chain_entry_t<INT> &other) const
    {
      return (other.element == element);
    }
    int64_t element{}; // Element at root of chain
    int     link{-1};  // How far is this element in the chain (1-based)
  };

  template <typename INT> using chain_t = std::vector<chain_entry_t<INT>>;

  template <typename INT>
  IOSS_NODISCARD chain_t<INT> generate_element_chains(Ioss::Region      &region,
                                                      const std::string &surface_list,
                                                      int debug_level, INT /*dummy*/);
} // namespace Ioss
