/*=========================================================================

  Module:    V_EdgeMetric.cpp

  Copyright 2003,2006,2019 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
  Under the terms of Contract DE-NA0003525 with NTESS,
  the U.S. Government retains certain rights in this software.

  See LICENSE for details.

=========================================================================*/

/*
 *
 * V_EdgeMetric.cpp contains quality calculations for edges
 *
 * This file is part of VERDICT
 *
 */

#include "verdict.h"

#include <cmath>

namespace VERDICT_NAMESPACE
{
/*!\brief Length of and edge.
 * Length is calculated by taking the distance between the end nodes.
 */
double edge_length(int /*num_nodes*/, const double coordinates[][3])
{

  double x = coordinates[1][0] - coordinates[0][0];
  double y = coordinates[1][1] - coordinates[0][1];
  double z = coordinates[1][2] - coordinates[0][2];
  return (double)(std::sqrt(x * x + y * y + z * z));
}
} // namespace verdict
