
/*
 *
 * PyramidMetrics.cpp contains quality calculations for Pyramids
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



/*
  the pyramid element

       5
       ^
       |\ 
      /| \\_
     |  \   \
     |  | \_ \_
     /   \/4\  \
    |   /|    \ \_
    | /  \      \ \
    /     |       \
  1 \_    |      _/3
      \_   \   _/
        \_ | _/
          \_/
          2

    a quadrilateral base and a pointy peak like a pyramid
          
*/


/*!
  the volume of a pyramid

  the volume is calculated by dividing the pyramid into
  2 tets and summing the volumes of the 2 tets.
*/

C_FUNC_DEF VERDICT_REAL v_pyramid_volume( int num_nodes, VERDICT_REAL coordinates[][3] )
{
    
  double volume = 0;
  VerdictVector side1, side2, side3;
  
  if (num_nodes == 5)
  {
    // divide the pyramid into 2 tets and calculate each

    side1.set( coordinates[1][0] - coordinates[0][0],
        coordinates[1][1] - coordinates[0][1],
        coordinates[1][2] - coordinates[0][2] );
    
    side2.set( coordinates[3][0] - coordinates[0][0],
        coordinates[3][1] - coordinates[0][1],
        coordinates[3][2] - coordinates[0][2] );
    
    side3.set( coordinates[4][0] - coordinates[0][0],
        coordinates[4][1] - coordinates[0][1], 
        coordinates[4][2] - coordinates[0][2] );
    
    // volume of the first tet
    volume = (side3 % (side1 * side2 ))/6.0;
    
    
    side1.set( coordinates[3][0] - coordinates[2][0],
        coordinates[3][1] - coordinates[2][1],
        coordinates[3][2] - coordinates[2][2] );
    
    side2.set( coordinates[1][0] - coordinates[2][0],
        coordinates[1][1] - coordinates[2][1],
        coordinates[1][2] - coordinates[2][2] );
    
    side3.set( coordinates[4][0] - coordinates[2][0],
        coordinates[4][1] - coordinates[2][1],
        coordinates[4][2] - coordinates[2][2] );
    
    // volume of the second tet
    volume += (side3 % (side1 * side2 ))/6.0;
 
  }   
  return (VERDICT_REAL)volume;
    
}



C_FUNC_DEF void v_pyramid_quality( int num_nodes, VERDICT_REAL coordinates[][3], 
    unsigned int metrics_request_flag, PyramidMetricVals *metric_vals )
{
  memset( metric_vals, 0, sizeof( PyramidMetricVals ) );

  if(metrics_request_flag & V_PYRAMID_VOLUME)
    metric_vals->volume = v_pyramid_volume(num_nodes, coordinates);
}

