
/*
 *
 * V_EdgeMetric.cpp contains quality calcultions for edges
 *
 * Copyright (C) 2003 Sandia National Laboratories <cubit@sandia.gov>
 *
 * This file is part of VERDICT
 *
 * This copy of VERDICT is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * VERDICT is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 */


#include "verdict.h"
#include <math.h>

/*! 
  length of and edge
  length is calculated by taking the distance between the end nodes
 */
C_FUNC_DEF VERDICT_REAL v_edge_length( int /*num_nodes*/, VERDICT_REAL coordinates[][3] )
{

  double x = coordinates[1][0] - coordinates[0][0];
  double y = coordinates[1][1] - coordinates[0][1];
  double z = coordinates[1][2] - coordinates[0][2];
  return (VERDICT_REAL)( sqrt (x*x + y*y + z*z) );
}

/*!
  
  higher order function for calculating multiple metrics at once.

  for an edge, there is only one metric, edge length.
*/

C_FUNC_DEF void edge_quality( int num_nodes, VERDICT_REAL coordinates[][3], 
    unsigned int metrics_request_flag, struct EdgeMetricVals *metric_vals )
{
  if(metrics_request_flag & V_EDGE_LENGTH)
    metric_vals->length = v_edge_length(num_nodes, coordinates);
}


