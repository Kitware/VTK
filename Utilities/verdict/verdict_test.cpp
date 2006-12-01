
/*
 *
 * verdict_test.cpp provides routines for testing the quality metrics code
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
#include "v_vector.h"
#include <math.h>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace std;


#define MAX_NODES_PER_ELEMENT 27
#define MAX_TESTS_PER_ELEMENT 20


#ifdef VERDICT_USE_FLOAT
   #define VERDICT_SIGNIFICANT_FIG 7    // 7 significant figures for floats
#else
   #define VERDICT_SIGNIFICANT_FIG 15   // 15 significant figures for doubles
#endif


struct test_case
{
   const char* testname;
   VerdictFunction function[MAX_TESTS_PER_ELEMENT];
   int num_nodes;
   // note: the 1st dim. of coords must bigger than the maximum num_nodes 
   // for any one element being tested
   VERDICT_REAL coords[MAX_NODES_PER_ELEMENT][3];
   VERDICT_REAL answer[MAX_TESTS_PER_ELEMENT];
};


int main( )
{

   // all test cases go here  
   test_case testcases[] = {
/*
      {
  "edge calc 1",
  {edge_length, 0},
  2,
  { { 0,0,0 }, {1,1,1} },
  { 1.732050807568877, 0 }
      },

      {
  "edge calc 2",
  {edge_length, 0 },
  2,
  { { 0,0,0 }, { 1,0,0 } },
  { 1.0, 0 }
      },

      {
  "edge calc 3",
  {edge_length, 0 },
  2,
  { { 0,0,0 }, { 0,0,0 } },
  { 0, 0 }
      },

      { 
  "simple wedge" ,
  {wedge_volume, 0},
  6,
  { { 0,0,0}, {-1,1,0}, {-1,0,0}, {0,0,1}, {-1,1,1}, {-1,0,1} },
  { 0.5, 0 }
      },

      {
  "singularity wedge",
  {wedge_volume, 0},
  6,
  { {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0}, {0,0,0} },
        { 0 }
      },

      {
  "simple tri",
  { tri_area, tri_smallest_angle, tri_largest_angle, tri_condition, 
     tri_normalized_jacobian, tri_shape, tri_relative_size, tri_shape_and_size, tri_distortion, 0},
  3,
  { 
    {0,0,0}, 
    {5,0,0}, 
    {2.5, 4.330127, 0}, 
  },

  { 0,0,0,0,0,0,0,0,0}
      },
*/
      {
  "singular tri",
  {v_tri_area, v_tri_aspect, v_tri_condition, 
         v_tri_distortion, v_tri_minimum_angle, v_tri_maximum_angle,
         v_tri_relative_size_squared, v_tri_shape, v_tri_shape_and_size,
   0}, 
  3,
  { 
   {0,0,0}, 
   {0.5,0.8660254037,0}, 
   {1,0,0} },
  { 123, 1234, 1234, 1234, 1234, 1234, 1234, 1234, 1234,0}
      },
/*
      {
  "simple quad",
  { v_quad_skew, 0},
  4,
   {
    {0,0,0},
    {1,0,0},
    {1,7,0}, 
    {0,7,0 } 
  }, 
  { 1.3333333333333333333, 0 }
      },
      {
  "simple quad",
  { quad_aspect, quad_skew, quad_taper, quad_warpage, quad_area, 
    quad_stretch, quad_largest_angle, quad_smallest_angle,
    quad_condition, quad_jacobian, quad_normalized_jacobian, quad_shear,
    quad_shape, quad_relative_size, quad_shape_and_size, quad_shear_and_size,
    quad_distortion, 0},
  4,
   {
    {2,0,0},    //1
    {1,1,2},   //2
    {0,1,0 },      //3
    {0,0,0},   //0



  }, 
  { 1.34, .30, .20, .20, .23, 0,0, 0, 0, 0 , 0, 0, 0, 0, 0, 0, 0, 0}
      },
*/
/*
      {
  "tet test",
  { tet_shear, 
          tet_volume, tet_condition, tet_jacobian, 
    tet_shape, tet_relative_size, tet_shape_and_size, tet_distortion, 0 },
  4,
  {
  {-5, -5, -5 },
  {-5, 5, -5 },
  {-5, -5, 5 },
  {5, -5, -5 },
 
  },

  {0,0,0,0,0,0,0,0,0}
      },
*/
/*
      {
  "hex test",
  { hex_aspect ,hex_skew, hex_taper, hex_volume, hex_stretch, hex_diagonal,
    hex_dimension, hex_condition, hex_jacobian, hex_normalized_jacobian, hex_shear,
    hex_shape, hex_relative_size, hex_shear_and_size, hex_shape_and_size,
    hex_distortion, 0 },
  8,

  { 
  { -.2, -.7, -.3 },  //1
  { -.7, .4, -.6 },   //2
  { -.5, .5, .3  },  //3
     { -.3, -.5, .5  }, //0
  
  { .5, -.8, -.2 },   //5
  { .4, .4, -.6  },    //6
  { .2, .5, .2   },   //7 
  { .5, -.3, .8  }  //4
  },


  {0.34, 0.34,0.34,0.34,0.34,0.34,0.34,0.34,0.34,0.34,0.34,0.34,0.34,0.34,0.34,0}
      },

*/


      // keep this one last
      { 0, {0} , 0, {{0}} , {0} } };


     
      
    
   int i;
   int j = 0;
   VERDICT_REAL answer_from_lib;
   VERDICT_REAL tolerance;
//   VERDICT_REAL norm_answer_from_lib;
   
#define MAX_STR_LEN 30

   char exponent[MAX_STR_LEN];
   char *base_ptr;
   int base;
   bool passed = true; // have all the tests performed so far passed?

   cout.setf( ios::scientific, ios::floatfield );
   cout.precision(VERDICT_SIGNIFICANT_FIG + 3 );

   cout << endl;

   
   // loop through each test
   for ( i = 0; testcases[i].testname != 0; i++ )
   {
     
     for ( j = 0;  testcases[i].function[j] != 0; j++ )
     {       
       answer_from_lib = 
         (testcases[i].function[j])
         (testcases[i].num_nodes, testcases[i].coords);
       
       sprintf(exponent, "%e", testcases[i].answer[j]);
       base_ptr = strstr( exponent, "e" );
       
       base_ptr = &base_ptr[1];

       base = atoi(base_ptr);

       tolerance = pow (10.0, - VERDICT_SIGNIFICANT_FIG ) * pow ( 10.0, base );

       if ( fabs( answer_from_lib - testcases[i].answer[j] ) > tolerance )
       {
           cout << endl;
           cout << "Test case \"" << testcases[i].testname
           << "\" #" << j+1 << " FAILED" << endl;

         cout    << "answer calculated was    " 
           << answer_from_lib << endl
                << "answer expected was      " 
           << testcases[i].answer[j] 
             << endl << endl;
                           passed = false;
            }
       else
       {
         cout << "Test case \"" << testcases[i].testname 
           << "\" #" << j+1 << " passed" << endl;
//           << "answer calculated was    " 
//                << answer_from_lib << endl
//           << "answer expected was      " 
//           << testcases[i].answer[j] << endl;
//    //     cout << endl;
       }
     }
   }

   cout << endl;



   return passed ? 0 : 1;

}


