/*
 * Copyright(C) 1999-2020, 2022, 2024 National Technology & Engineering Solutions
 * of Sandia, LLC (NTESS).  Under the terms of Contract DE-NA0003525 with
 * NTESS, the U.S. Government retains certain rights in this software.
 *
 * See packages/seacas/LICENSE for details
 */
#pragma once

#include "ioss_export.h"
#include "vtk_ioss_mangle.h"

#include <string>
#include <vector>

namespace Ioss {
  /**
   * Take the 'str' argument and split it using the list of characters
   * in separators as separators. Use tokens to return the result.
   */
  IOSS_EXPORT std::vector<std::string>
  tokenize(const std::string &str, const std::string &separators, bool allow_empty = false);
} // namespace Ioss
