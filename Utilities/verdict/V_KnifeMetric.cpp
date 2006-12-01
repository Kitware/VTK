
/*
 *
 * KnifeMetrics.cpp contains quality calculations for knives
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


#define VERDICT_EXPORTS

#include "verdict.h"
#include "VerdictVector.hpp"
#include <memory.h> 


/*  a knife element

          3
         _/\_
       _/  | \_
   0 _/        \_ 2
    |\_    | ___/|
    |  \  __/    |
    |  1\/ |     |
    |    \       |
    |_____\|_____|
   4       5      6      


    (edge 3,5  is is a hidden line if you will)

    if this is hard to visualize, consider a hex
    with nodes 5 and 7 becoming the same node


*/




/*!
  calculates the volume of a knife element

  this is done by dividing the knife into 4 tets
  and summing the volumes of each.
*/

C_FUNC_DEF VERDICT_REAL v_knife_volume( int num_nodes, VERDICT_REAL coordinates[][3] )
{
  double volume = 0;
  VerdictVector side1, side2, side3;
  
  if (num_nodes == 7)
  {
    
    // divide the knife into 4 tets and calculate the volume
    
    side1.set(
        coordinates[1][0] - coordinates[0][0],
        coordinates[1][1] - coordinates[0][1],
        coordinates[1][2] - coordinates[0][2]
        );
    side2.set(
        coordinates[3][0] - coordinates[0][0],
        coordinates[3][1] - coordinates[0][1],
        coordinates[3][2] - coordinates[0][2]
        );
    side3.set(
        coordinates[4][0] - coordinates[0][0],
        coordinates[4][1] - coordinates[0][1],
        coordinates[4][2] - coordinates[0][2]
        );

    volume = side3 % (side1 * side2) / 6;
   

    side1.set(
        coordinates[5][0] - coordinates[1][0],
        coordinates[5][1] - coordinates[1][1],
        coordinates[5][2] - coordinates[1][2]
        );
    side2.set(
        coordinates[3][0] - coordinates[1][0],
        coordinates[3][1] - coordinates[1][1],
        coordinates[3][2] - coordinates[1][2]
        );
    side3.set(
        coordinates[4][0] - coordinates[1][0],
        coordinates[4][1] - coordinates[1][1],
        coordinates[4][2] - coordinates[1][2]
        );

    volume += side3 % (side1 * side2) / 6;
    
    
    side1.set(
        coordinates[2][0] - coordinates[1][0],
        coordinates[2][1] - coordinates[1][1],
        coordinates[2][2] - coordinates[1][2]
        );
    side2.set(
        coordinates[3][0] - coordinates[1][0],
        coordinates[3][1] - coordinates[1][1],
        coordinates[3][2] - coordinates[1][2]
        );
    side3.set(
        coordinates[6][0] - coordinates[1][0],
        coordinates[6][1] - coordinates[1][1],
        coordinates[6][2] - coordinates[1][2]
        );

    volume += side3 % (side1 * side2) / 6;
   
    
    side1.set(
        coordinates[3][0] - coordinates[1][0],
        coordinates[3][1] - coordinates[1][1],
        coordinates[3][2] - coordinates[1][2]
        );
    side2.set(
        coordinates[5][0] - coordinates[1][0],
        coordinates[5][1] - coordinates[1][1],
        coordinates[5][2] - coordinates[1][2]
        );
    side3.set(
        coordinates[6][0] - coordinates[1][0],
        coordinates[6][1] - coordinates[1][1],
        coordinates[6][2] - coordinates[1][2]
        );

    volume += side3 % (side1 * side2) / 6;
  
  }

  return (VERDICT_REAL)volume;
    
}


/*!
 
  calculate the quality metrics of a knife element.

  There is only one, but we put this here to be consistent with
  functions for other element types.  Who knows if we'll add
  more metrics.
*/

C_FUNC_DEF void v_knife_quality( int num_nodes, VERDICT_REAL coordinates[][3], 
    unsigned int metrics_request_flag, KnifeMetricVals *metric_vals )
{
  memset( metric_vals, 0, sizeof(KnifeMetricVals) );

  if(metrics_request_flag & V_KNIFE_VOLUME)
    metric_vals->volume = v_knife_volume(num_nodes, coordinates);
}


