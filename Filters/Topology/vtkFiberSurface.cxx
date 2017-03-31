/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFiberSurface.h"

#include "vtkCell.h"
#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkUnstructuredGrid.h"

// lookup table for powers of 3 shifts in the marching tetrahedra cases
// which is described in greyTetTriangles table.
// remind that we use 0, 1 and 2 to represent (W)hite, (G)rey and (B)lack cases. For each
// tetrahedron, cases for four vertices can be represented
// by a four-digit number, such as 0001. We assume that all vertices are in CCW order.
// The array greyTetTriangles actually records all of 81 such cases. The order of case
// index starts from right-most to the left-most digit: starting from 0000 to 0002,
// then from 0010 to 0022, then from 0100 to 0222, finally from 1000 to 2222.
// It is easy to observe that:
//   from 0001 to 0002, as case number in the first dight is incremented by 1,
//                      we only need to skip 1 index in the greyTetTriangles table.
//
//   from 0010 to 0020, as case number in the second dight is incremented by 1,
//                      we need to skip 3 indices (0010, 0011, 0012, 0020)
//
//   from 0100 to 0200, as case number in the third dight is incremented by 1,
//                      we need to skip 9 indices  (0100, 0101, 0102, 0110, 0111,
//                                                  0112, 0120, 0121, 0122, 0200)
//
//   from 1000 to 2000, as case number in the fourth dight is incremented by 1,
//                      we need to skip 27 indices (1000, 1001, 1002, 1010, 1011,
//                                                  1012, 1020, 1021, 1022, 1100,
//                                                  1101, 1102, 1110, 1111, 1112,
//                                                  1120, 1121, 1122, 1200, 1201,
//                                                  1202, 1210, 1211, 1212, 1220,
//                                                  1221, 1222, 2000)
// Given case classifications for four vertices in a tetrahedron, this ternaryShift array
// could be used to quickly locate the index number in the marching tetrahedron case
// table. This array can also be used in the clipping case look-up table
// clipTriangleVertices.
static const char ternaryShift[4] = { 1, 3, 9, 27 };

//----------------------------------------------------------------------------

// In the Marching Tethrhedron with Grey case, the iso-surface can be either a triangle
// ,quad or null. The number of triangles in each case is at most 2. This array
// records the number of triangles for every case.
static const int nTriangles[81] = {
  0, 0, 1, 0, 0, 1, 1, 1, 2, // cases 0000-0022
  0, 0, 1, 0, 1, 1, 1, 1, 1, // cases 0100-0122
  1, 1, 2, 1, 1, 1, 2, 1, 1, // cases 0200-0222

  0, 0, 1, 0, 1, 1, 1, 1, 1, // cases 1000-1022
  0, 1, 1, 1, 0, 1, 1, 1, 0, // cases 1100-1122
  1, 1, 1, 1, 1, 0, 1, 0, 0, // cases 1200-1222

  1, 1, 2, 1, 1, 1, 2, 1, 1, // cases 2000-2022
  1, 1, 1, 1, 1, 0, 1, 0, 0, // cases 2100-2122
  2, 1, 1, 1, 0, 0, 1, 0, 0  // cases 2200-2222
};

//----------------------------------------------------------------------------

// array of vertices for triangles in the marching tetrahedron cases
// each vertex on the tetra is marked as (B)lack, (W)hite or (G)rey
// there are total 81 cases. Each case contains at most two triangles.
// The order these cases arranged are as follows: starting from 0000 to 0002,
// then from 0010 to 0022, then from 0100 to 0222, finally from 1000 to 2222.
static const vtkFiberSurface::BaseVertexType greyTetTriangles[81][2][3] = {
  { // 0. case 0000 (A)
    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 1. case 0001 (B)
    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 2. case 0002 (D)
    { vtkFiberSurface::bv_edge_01, vtkFiberSurface::bv_edge_02, vtkFiberSurface::bv_edge_03 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 3. case 0010 (B)
    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 4. case 0011 (C)
    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 5. case 0012 (F)
    { vtkFiberSurface::bv_vertex_1, vtkFiberSurface::bv_edge_02, vtkFiberSurface::bv_edge_03 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 6. case 0020 (D)
    { vtkFiberSurface::bv_edge_01, vtkFiberSurface::bv_edge_13, vtkFiberSurface::bv_edge_12 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 7. case 0021 (F)
    { vtkFiberSurface::bv_vertex_0, vtkFiberSurface::bv_edge_13, vtkFiberSurface::bv_edge_12 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 8. case 0022 (I)
    { vtkFiberSurface::bv_edge_02, vtkFiberSurface::bv_edge_03, vtkFiberSurface::bv_edge_13 },

    { vtkFiberSurface::bv_edge_02, vtkFiberSurface::bv_edge_13, vtkFiberSurface::bv_edge_12 } },
  { // 9. case 0100 (B)
    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 10. case 0101 (C)
    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 11. case 0102 (F)
    { vtkFiberSurface::bv_vertex_2, vtkFiberSurface::bv_edge_03, vtkFiberSurface::bv_edge_01 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 12. case 0110 (C)
    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 13. case 0111 (E)
    { vtkFiberSurface::bv_vertex_0, vtkFiberSurface::bv_vertex_1, vtkFiberSurface::bv_vertex_2 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 14. case 0112 (H)
    { vtkFiberSurface::bv_vertex_1, vtkFiberSurface::bv_vertex_2, vtkFiberSurface::bv_edge_03 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 15. case 0120 (F)
    { vtkFiberSurface::bv_vertex_2, vtkFiberSurface::bv_edge_01, vtkFiberSurface::bv_edge_13 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 16. case 0121 (H)
    { vtkFiberSurface::bv_vertex_2, vtkFiberSurface::bv_vertex_0, vtkFiberSurface::bv_edge_13 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 17. case 0122 (K)
    { vtkFiberSurface::bv_vertex_2, vtkFiberSurface::bv_edge_03, vtkFiberSurface::bv_edge_13 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 18. case 0200 (D)
    { vtkFiberSurface::bv_edge_02, vtkFiberSurface::bv_edge_12, vtkFiberSurface::bv_edge_23 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 19. case 0201 (F)
    { vtkFiberSurface::bv_vertex_0, vtkFiberSurface::bv_edge_12, vtkFiberSurface::bv_edge_23 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 20. case 0202 (I)
    { vtkFiberSurface::bv_edge_12, vtkFiberSurface::bv_edge_23, vtkFiberSurface::bv_edge_03 },

    { vtkFiberSurface::bv_edge_12, vtkFiberSurface::bv_edge_03, vtkFiberSurface::bv_edge_01 } },
  { // 21. case 0210 (F)
    { vtkFiberSurface::bv_vertex_1, vtkFiberSurface::bv_edge_23, vtkFiberSurface::bv_edge_02 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 22. case 0211 (H)
    { vtkFiberSurface::bv_vertex_0, vtkFiberSurface::bv_vertex_1, vtkFiberSurface::bv_edge_23 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 23. case 0212 (K)
    { vtkFiberSurface::bv_vertex_1, vtkFiberSurface::bv_edge_03, vtkFiberSurface::bv_edge_23 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 24. case 0220 (I)
    { vtkFiberSurface::bv_edge_01, vtkFiberSurface::bv_edge_13, vtkFiberSurface::bv_edge_23 },

    { vtkFiberSurface::bv_edge_01, vtkFiberSurface::bv_edge_23, vtkFiberSurface::bv_edge_02 } },
  { // 25. case 0221 (K)
    { vtkFiberSurface::bv_vertex_0, vtkFiberSurface::bv_edge_13, vtkFiberSurface::bv_edge_23 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 26. case 0222 (M)
    { vtkFiberSurface::bv_edge_03, vtkFiberSurface::bv_edge_13, vtkFiberSurface::bv_edge_23 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 27. case 1000 (B)
    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 28. case 1001 (C)
    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 29. case 1002 (F)
    { vtkFiberSurface::bv_vertex_3, vtkFiberSurface::bv_edge_01, vtkFiberSurface::bv_edge_02 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 30. case 1010 (C)
    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 31. case 1011 (E)
    { vtkFiberSurface::bv_vertex_0, vtkFiberSurface::bv_vertex_3, vtkFiberSurface::bv_vertex_1 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 32. case 1012 (H)
    { vtkFiberSurface::bv_vertex_3, vtkFiberSurface::bv_vertex_1, vtkFiberSurface::bv_edge_02 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 33. case 1020 (F)
    { vtkFiberSurface::bv_vertex_3, vtkFiberSurface::bv_edge_12, vtkFiberSurface::bv_edge_01 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 34. case 1021 (H)
    { vtkFiberSurface::bv_vertex_0, vtkFiberSurface::bv_vertex_3, vtkFiberSurface::bv_edge_12 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 35. case 1022 (K)
    { vtkFiberSurface::bv_vertex_3, vtkFiberSurface::bv_edge_12, vtkFiberSurface::bv_edge_02 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 36. case 1100 (C)
    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 37. case 1101 (E)
    { vtkFiberSurface::bv_vertex_0, vtkFiberSurface::bv_vertex_2, vtkFiberSurface::bv_vertex_3 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 38. case 1102 (H)
    { vtkFiberSurface::bv_vertex_2, vtkFiberSurface::bv_vertex_3, vtkFiberSurface::bv_edge_01 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 39. case 1110 (E)
    { vtkFiberSurface::bv_vertex_1, vtkFiberSurface::bv_vertex_3, vtkFiberSurface::bv_vertex_2 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 40. case 1111 (G)
    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 41. case 1112 (J)
    { vtkFiberSurface::bv_vertex_1, vtkFiberSurface::bv_vertex_2, vtkFiberSurface::bv_vertex_3 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 42. case 1120 (H)
    { vtkFiberSurface::bv_vertex_3, vtkFiberSurface::bv_vertex_2, vtkFiberSurface::bv_edge_01 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 43. case 1121 (J)
    { vtkFiberSurface::bv_vertex_0, vtkFiberSurface::bv_vertex_3, vtkFiberSurface::bv_vertex_2 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 44. case 1122 (L)
    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 45. case 1200 (F)
    { vtkFiberSurface::bv_vertex_3, vtkFiberSurface::bv_edge_02, vtkFiberSurface::bv_edge_12 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 46. case 1201 (H)
    { vtkFiberSurface::bv_vertex_3, vtkFiberSurface::bv_vertex_0, vtkFiberSurface::bv_edge_12 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 47. case 1202 (K)
    { vtkFiberSurface::bv_vertex_3, vtkFiberSurface::bv_edge_01, vtkFiberSurface::bv_edge_12 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 48. case 1210 (H)
    { vtkFiberSurface::bv_vertex_1, vtkFiberSurface::bv_vertex_3, vtkFiberSurface::bv_edge_02 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 49. case 1211 (J)
    { vtkFiberSurface::bv_vertex_0, vtkFiberSurface::bv_vertex_1, vtkFiberSurface::bv_vertex_3 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 50. case 1212 (L)
    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 51. case 1220 (K)
    { vtkFiberSurface::bv_vertex_3, vtkFiberSurface::bv_edge_02, vtkFiberSurface::bv_edge_01 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 52. case 1221 (L)
    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 53. case 1222 (N)
    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 54. case 2000 (D)
    { vtkFiberSurface::bv_edge_03, vtkFiberSurface::bv_edge_23, vtkFiberSurface::bv_edge_13 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 55. case 2001 (F)
    { vtkFiberSurface::bv_vertex_0, vtkFiberSurface::bv_edge_23, vtkFiberSurface::bv_edge_13 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 56. case 2002 (I)
    { vtkFiberSurface::bv_edge_23, vtkFiberSurface::bv_edge_13, vtkFiberSurface::bv_edge_01 },

    { vtkFiberSurface::bv_edge_23, vtkFiberSurface::bv_edge_01, vtkFiberSurface::bv_edge_02 } },
  { // 57. case 2010 (F)
    { vtkFiberSurface::bv_vertex_1, vtkFiberSurface::bv_edge_03, vtkFiberSurface::bv_edge_23 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 58. case 2011 (H)
    { vtkFiberSurface::bv_vertex_1, vtkFiberSurface::bv_vertex_0, vtkFiberSurface::bv_edge_23 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 59. case 2012 (K)
    { vtkFiberSurface::bv_vertex_1, vtkFiberSurface::bv_edge_02, vtkFiberSurface::bv_edge_23 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 60. case 2020 (I)
    { vtkFiberSurface::bv_edge_12, vtkFiberSurface::bv_edge_01, vtkFiberSurface::bv_edge_03 },

    { vtkFiberSurface::bv_edge_12, vtkFiberSurface::bv_edge_03, vtkFiberSurface::bv_edge_23 } },
  { // 61. case 2021 (K)
    { vtkFiberSurface::bv_vertex_0, vtkFiberSurface::bv_edge_23, vtkFiberSurface::bv_edge_12 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 62. case 2022 (M)
    { vtkFiberSurface::bv_edge_02, vtkFiberSurface::bv_edge_23, vtkFiberSurface::bv_edge_12 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 63. case 2100 (F)
    { vtkFiberSurface::bv_vertex_2, vtkFiberSurface::bv_edge_13, vtkFiberSurface::bv_edge_03 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 64. case 2101 (H)
    { vtkFiberSurface::bv_vertex_0, vtkFiberSurface::bv_vertex_2, vtkFiberSurface::bv_edge_13 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 65. case 2102 (K)
    { vtkFiberSurface::bv_vertex_2, vtkFiberSurface::bv_edge_13, vtkFiberSurface::bv_edge_01 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 66. case 2110 (H)
    { vtkFiberSurface::bv_vertex_2, vtkFiberSurface::bv_vertex_1, vtkFiberSurface::bv_edge_03 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 67. case 2111 (J)
    { vtkFiberSurface::bv_vertex_0, vtkFiberSurface::bv_vertex_2, vtkFiberSurface::bv_vertex_1 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 68. case 2112 (L)
    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 69. case 2120 (K)
    { vtkFiberSurface::bv_vertex_2, vtkFiberSurface::bv_edge_01, vtkFiberSurface::bv_edge_03 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 70. case 2121 (L)
    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 71. case 2122 (N)
    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 72. case 2200 (I)
    { vtkFiberSurface::bv_edge_13, vtkFiberSurface::bv_edge_03, vtkFiberSurface::bv_edge_02 },

    { vtkFiberSurface::bv_edge_13, vtkFiberSurface::bv_edge_02, vtkFiberSurface::bv_edge_12 } },
  { // 73. case 2201 (K)
    { vtkFiberSurface::bv_vertex_0, vtkFiberSurface::bv_edge_12, vtkFiberSurface::bv_edge_13 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 74. case 2202 (M)
    { vtkFiberSurface::bv_edge_01, vtkFiberSurface::bv_edge_12, vtkFiberSurface::bv_edge_13 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 75. case 2210 (K)
    { vtkFiberSurface::bv_vertex_1, vtkFiberSurface::bv_edge_03, vtkFiberSurface::bv_edge_02 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 76. case 2211 (L)
    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 77. case 2212 (N)
    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 78. case 2220 (M)
    { vtkFiberSurface::bv_edge_01, vtkFiberSurface::bv_edge_03, vtkFiberSurface::bv_edge_02 },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 79. case 2221 (N)
    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } },
  { // 80. case 2222 (O)
    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used },

    { vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used, vtkFiberSurface::bv_not_used } }
};

//----------------------------------------------------------------------------

// conversion from the enum semantics for edges to actual edge numbers
// depends on the ordering of bv_edge_** in BaseVertexType enum
static const int edge2endpoints[6][2] = { { 0, 1 }, { 0, 2 }, { 0, 3 }, { 1, 2 }, { 1, 3 },
  { 2, 3 } };

//----------------------------------------------------------------------------

// convert edge_*_param_* enum to edge numbers
// depends on the ordering of edge_0 and edge_1 enums (i.e. edge_0 + 2 == edge_1 + 1
// == edge_2)
static const int clip2points[3][2] = { { 1, 2 }, { 2, 0 }, { 0, 1 } };

//----------------------------------------------------------------------------

// this table lists the number of triangles per case for fiber clipping
static const int nClipTriangles[27] = {
  0, 1, 2, 1, 2, 3, 2, 3, 2, // cases 000 - 022
  1, 2, 3, 2, 1, 2, 3, 2, 1, // cases 100 - 122
  2, 3, 2, 3, 2, 1, 2, 1, 0  // cases 200 - 222
};

//----------------------------------------------------------------------------

// with up to three triangles, we can have up to 9 vertices specified
// note that this may lead to redundant interpolation (as in MC/MT), but we gain in
// clarity by doing it this way.
// This array therefore specifies the vertices of each triangle to be rendered in the
// clipping process.
static const int clipTriangleVertices[27][3][3] = {
  // 0. case 000: A - empty
  { { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used },

    { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used },

    { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used } },
  // 1. case 001: B - point-triangle
  { { vtkFiberSurface::vertex_0, vtkFiberSurface::edge_2_parm_0, vtkFiberSurface::edge_1_parm_0 },

    { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used },

    { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used } },
  // 2. case 002: D - stripe
  { { vtkFiberSurface::edge_2_parm_0, vtkFiberSurface::edge_1_parm_0,
      vtkFiberSurface::edge_1_parm_1 },

    { vtkFiberSurface::edge_2_parm_0, vtkFiberSurface::edge_1_parm_1,
      vtkFiberSurface::edge_2_parm_1 },

    { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used } },
  // 3. case 010: B - point-triangle
  { { vtkFiberSurface::vertex_1, vtkFiberSurface::edge_0_parm_0, vtkFiberSurface::edge_2_parm_0 },

    { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used },

    { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used } },
  // 4. case 011: C - edge-quad
  { { vtkFiberSurface::vertex_0, vtkFiberSurface::vertex_1, vtkFiberSurface::edge_0_parm_0 },

    { vtkFiberSurface::vertex_0, vtkFiberSurface::edge_0_parm_0, vtkFiberSurface::edge_1_parm_0 },

    { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used } },
  // 5. case 012: E - point-stripe
  { { vtkFiberSurface::vertex_1, vtkFiberSurface::edge_0_parm_0, vtkFiberSurface::edge_2_parm_1 },

    { vtkFiberSurface::edge_2_parm_1, vtkFiberSurface::edge_0_parm_0,
      vtkFiberSurface::edge_1_parm_1 },

    { vtkFiberSurface::edge_1_parm_1, vtkFiberSurface::edge_0_parm_0,
      vtkFiberSurface::edge_1_parm_0 } },
  // 6. case 020: D - stripe
  { { vtkFiberSurface::edge_0_parm_0, vtkFiberSurface::edge_2_parm_0,
      vtkFiberSurface::edge_2_parm_1 },

    { vtkFiberSurface::edge_0_parm_0, vtkFiberSurface::edge_2_parm_1,
      vtkFiberSurface::edge_0_parm_1 },

    { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used } },
  // 7. case 021: E - point-stripe
  { { vtkFiberSurface::vertex_0, vtkFiberSurface::edge_2_parm_1, vtkFiberSurface::edge_1_parm_0 },

    { vtkFiberSurface::edge_1_parm_0, vtkFiberSurface::edge_2_parm_1,
      vtkFiberSurface::edge_0_parm_0 },

    { vtkFiberSurface::edge_0_parm_0, vtkFiberSurface::edge_2_parm_1,
      vtkFiberSurface::edge_0_parm_1 } },
  // 8. case 022: D - stripe
  { { vtkFiberSurface::edge_1_parm_1, vtkFiberSurface::edge_0_parm_1,
      vtkFiberSurface::edge_0_parm_0 },

    { vtkFiberSurface::edge_1_parm_1, vtkFiberSurface::edge_0_parm_0,
      vtkFiberSurface::edge_1_parm_0 },

    { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used } },
  // 9. case 100: B - point-triangle
  { { vtkFiberSurface::vertex_2, vtkFiberSurface::edge_1_parm_0, vtkFiberSurface::edge_0_parm_0 },

    { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used },

    { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used } },
  // 10. case 101: C - edge-quad
  { { vtkFiberSurface::vertex_2, vtkFiberSurface::vertex_0, vtkFiberSurface::edge_2_parm_0 },

    { vtkFiberSurface::vertex_2, vtkFiberSurface::edge_2_parm_0, vtkFiberSurface::edge_0_parm_0 },

    { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used } },
  // 11. case 102: E - point-stripe
  { { vtkFiberSurface::vertex_2, vtkFiberSurface::edge_1_parm_1, vtkFiberSurface::edge_0_parm_0 },

    { vtkFiberSurface::edge_0_parm_0, vtkFiberSurface::edge_1_parm_1,
      vtkFiberSurface::edge_2_parm_0 },

    { vtkFiberSurface::edge_2_parm_0, vtkFiberSurface::edge_1_parm_1,
      vtkFiberSurface::edge_2_parm_1 } },
  // 12. case 110: C - edge-quad
  { { vtkFiberSurface::vertex_1, vtkFiberSurface::vertex_2, vtkFiberSurface::edge_1_parm_0 },

    { vtkFiberSurface::vertex_1, vtkFiberSurface::edge_1_parm_0, vtkFiberSurface::edge_2_parm_0 },

    { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used } },
  // 13. case 111: F - entire triangle
  { { vtkFiberSurface::vertex_0, vtkFiberSurface::vertex_1, vtkFiberSurface::vertex_2 },

    { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used },

    { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used } },
  // 14. case 112: C - edge-quad
  { { vtkFiberSurface::vertex_1, vtkFiberSurface::vertex_2, vtkFiberSurface::edge_1_parm_1 },

    { vtkFiberSurface::vertex_1, vtkFiberSurface::edge_1_parm_1, vtkFiberSurface::edge_2_parm_1 },

    { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used } },
  // 15. case 120: E - point-stripe
  { { vtkFiberSurface::vertex_2, vtkFiberSurface::edge_1_parm_0, vtkFiberSurface::edge_0_parm_1 },

    { vtkFiberSurface::edge_0_parm_1, vtkFiberSurface::edge_1_parm_0,
      vtkFiberSurface::edge_2_parm_1 },

    { vtkFiberSurface::edge_2_parm_1, vtkFiberSurface::edge_1_parm_0,
      vtkFiberSurface::edge_2_parm_0 } },
  // 16. case 121: C - edge-quad
  { { vtkFiberSurface::vertex_2, vtkFiberSurface::vertex_0, vtkFiberSurface::edge_2_parm_1 },

    { vtkFiberSurface::vertex_2, vtkFiberSurface::edge_2_parm_1, vtkFiberSurface::edge_0_parm_1 },

    { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used } },
  // 17. case 122: B - point-triangle
  { { vtkFiberSurface::vertex_2, vtkFiberSurface::edge_1_parm_1, vtkFiberSurface::edge_0_parm_1 },

    { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used },

    { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used } },
  // 18. case 200: D - stripe
  { { vtkFiberSurface::edge_1_parm_0, vtkFiberSurface::edge_0_parm_0,
      vtkFiberSurface::edge_0_parm_1 },

    { vtkFiberSurface::edge_1_parm_0, vtkFiberSurface::edge_0_parm_1,
      vtkFiberSurface::edge_1_parm_1 },

    { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used } },
  // 19. case 201: E - point-stripe
  { { vtkFiberSurface::vertex_0, vtkFiberSurface::edge_2_parm_0, vtkFiberSurface::edge_1_parm_1 },

    { vtkFiberSurface::edge_1_parm_1, vtkFiberSurface::edge_2_parm_0,
      vtkFiberSurface::edge_0_parm_1 },

    { vtkFiberSurface::edge_0_parm_1, vtkFiberSurface::edge_2_parm_0,
      vtkFiberSurface::edge_0_parm_0 } },
  // 20. case 202: D - stripe
  { { vtkFiberSurface::edge_0_parm_1, vtkFiberSurface::edge_2_parm_1,
      vtkFiberSurface::edge_2_parm_0 },

    { vtkFiberSurface::edge_0_parm_1, vtkFiberSurface::edge_2_parm_0,
      vtkFiberSurface::edge_0_parm_0 },

    { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used } },
  // 21. case 210: E - point-stripe
  { { vtkFiberSurface::vertex_1, vtkFiberSurface::edge_0_parm_1, vtkFiberSurface::edge_2_parm_0 },

    { vtkFiberSurface::edge_2_parm_0, vtkFiberSurface::edge_0_parm_1,
      vtkFiberSurface::edge_1_parm_0 },

    { vtkFiberSurface::edge_1_parm_0, vtkFiberSurface::edge_0_parm_1,
      vtkFiberSurface::edge_1_parm_1 } },
  // 22. case 211: C - edge-quad
  { { vtkFiberSurface::vertex_0, vtkFiberSurface::vertex_1, vtkFiberSurface::edge_0_parm_1 },

    { vtkFiberSurface::vertex_0, vtkFiberSurface::edge_0_parm_1, vtkFiberSurface::edge_1_parm_1 },

    { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used } },
  // 23. case 212: B - point-triangle
  { { vtkFiberSurface::vertex_1, vtkFiberSurface::edge_0_parm_1, vtkFiberSurface::edge_2_parm_1 },

    { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used },

    { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used } },
  // 24. case 220: D - stripe
  { { vtkFiberSurface::edge_2_parm_1, vtkFiberSurface::edge_1_parm_1,
      vtkFiberSurface::edge_1_parm_0 },

    { vtkFiberSurface::edge_2_parm_1, vtkFiberSurface::edge_1_parm_0,
      vtkFiberSurface::edge_2_parm_0 },

    { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used } },
  // 25. case 221: B - point-triangle
  { { vtkFiberSurface::vertex_0, vtkFiberSurface::edge_2_parm_1, vtkFiberSurface::edge_1_parm_1 },

    { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used },

    { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used } },
  // 26. case 222: A - empty
  { { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used },

    { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used },

    { vtkFiberSurface::not_used, vtkFiberSurface::not_used, vtkFiberSurface::not_used } }
};

//----------------------------------------------------------------------------

vtkStandardNewMacro(vtkFiberSurface);

//----------------------------------------------------------------------------

vtkFiberSurface::vtkFiberSurface()
{
  // number of input ports is 2
  this->SetNumberOfInputPorts(2);
  this->Fields[0] = this->Fields[1] = NULL;
}

//----------------------------------------------------------------------------

void vtkFiberSurface::SetField1(char* nm)
{
  this->Fields[0] = nm;
}

//----------------------------------------------------------------------------

void vtkFiberSurface::SetField2(char* nm)
{
  this->Fields[1] = nm;
}

//----------------------------------------------------------------------------

void vtkFiberSurface::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------

int vtkFiberSurface::FillInputPortInformation(int port, vtkInformation* info)
{
  switch (port)
  {
    // port 0 expects a tetrahedral mesh as input data
    case 0:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkUnstructuredGrid");
      return 1;
    // port 1 expects a fiber surface control polygon (FSCP)
    case 1:
      info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
      return 1;
  }
  return 0;
}

//----------------------------------------------------------------------------

int vtkFiberSurface::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // obtain the input/output port info
  vtkInformation* inMeshInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* inLinesInfo = inputVector[1]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input regular grid and fiber surface control polygon (FSCP)
  vtkUnstructuredGrid* mesh =
    vtkUnstructuredGrid::SafeDownCast(inMeshInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* lines = vtkPolyData::SafeDownCast(inLinesInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // get the dataset statistics
  vtkIdType numCells = mesh->GetNumberOfCells();
  vtkIdType numPts = mesh->GetNumberOfPoints();
  vtkIdType numArrays = mesh->GetPointData()->GetNumberOfArrays();

  // check if the data set is empty or not.
  // check if the data set contains at least two scalar arrays.
  if (numCells < 1 || numPts < 1 || numArrays < 2)
  {
    vtkErrorMacro("No input data. Two fields are required for fiber surface generation");
    return 1;
  }

  // check if two scalar fields are specified by the user.
  // if not specified, an error message will be shown.
  if (!this->Fields[0] || !this->Fields[1])
  {
    vtkErrorMacro("Two scalar fields need to be specified.");
    return 1;
  }
  else
  {
    vtkSmartPointer<vtkDataArray> array;
    for (int i = 0; i < 2; i++)
    {
      array = mesh->GetPointData()->GetArray(this->Fields[i]);
      if (!array)
      {
        vtkErrorMacro("Names of the scalar array do not exist.");
        return 1;
      }
    }
  }

  // extract the points of the subdivided tetrahedra
  vtkPoints* meshPoints = mesh->GetPoints();

  // allocate points and cell storage for computing fiber surface structure
  vtkSmartPointer<vtkPoints> newPoints = vtkSmartPointer<vtkPoints>::New();
  vtkSmartPointer<vtkCellArray> newPolys = vtkSmartPointer<vtkCellArray>::New();

  // extract two scalar field arrays and put into one structure
  vtkDataArray* fieldScalars[2] = { mesh->GetPointData()->GetArray(this->Fields[0]),
    mesh->GetPointData()->GetArray(this->Fields[1]) };

  // extract a fiber surface for every edge in the FSCP.
  // if the FSCP has no edges, this loop will not start.
  // Algorithm:
  // 1. Extract base fiber surface using marching tetrahedron
  // 2. Clip the base fiber surface to extract the exact fiber surface with respect to
  //    each line segment in FSCP
  const vtkIdType numberOfLines = lines->GetNumberOfCells();
  for (vtkIdType lineIndex = 0; lineIndex < numberOfLines; ++lineIndex)
  {
    // For each line segment of the FSCP
    vtkCell* line = lines->GetCell(lineIndex);

    // test if the current cell of FSCP is a line or not.
    // The computation only proceed if the current cell is a line.
    if (line->GetNumberOfPoints() != 2)
    {
      vtkWarningMacro("Current cell index " << lineIndex << " in the FSCP is not of a line type.");
      continue;
    }

    // get the starting point of the line
    double pointStart[3], pointEnd[3];
    lines->GetPoints()->GetPoint(line->GetPointId(0), pointStart);

    // get the end point of the line
    lines->GetPoints()->GetPoint(line->GetPointId(1), pointEnd);

    // first point is the origin of the parametric form of line
    const double origin[2] = { pointStart[0], pointStart[1] };

    // if the input point coordinates are normalised values. We need to interpolate these
    // values to the actual scalar values.
    // obtain the direction vector of the line segment
    const double direction[2] = { pointEnd[0] - origin[0], pointEnd[1] - origin[1] };

    // compute length of the line segment
    const double length = sqrt(direction[0] * direction[0] + direction[1] * direction[1]);

    // if the length of the current line is zero, then skip to the next cell.
    if (length == 0.0f)
    {
      vtkWarningMacro("End points of the current line index "
        << lineIndex << " in the FSCP colocate on the same point.");
      continue;
    }

    // compute the normal vector to the line segment
    const double normal[2] = { direction[1] / length, -direction[0] / length };

    // given a line segment with one of its endpoint origin and its normal vector normal
    // given an arbitrary point p
    // the signed distance from p to the line can be computed using the Hess Normal Form:
    //    signedDistance =  dot(p - origin, normal)
    //                   =  dot(p,normal) - dot(origin,normal)
    // since dot(origin,normal) is an invariant, therefore we compute it first to avoid
    // duplicate computation in the following steps.
    const double dotOriginNormal = normal[0] * origin[0] + normal[1] * origin[1];

    // get the number of tetras in the domain
    const vtkIdType numberOfTets = mesh->GetNumberOfCells();

    // iterate through every cell of the domain and extract its fiber surface.
    // note that each cell is a tetrahedron.
    for (vtkIdType tetIndex = 0; tetIndex < numberOfTets; ++tetIndex)
    {
      // update progress of the extraction
      this->UpdateProgress((tetIndex + 1.0) / numberOfTets);

      // obtain the current tetra cell
      vtkCell* tet = mesh->GetCell(tetIndex);

      // check if the current cell is a tetrahedron type or not
      // if not, skip this cell.
      if (mesh->GetCellType(tetIndex) != VTK_TETRA || tet->GetNumberOfPoints() != 4)
      {
        vtkWarningMacro("Current cell " << tetIndex << "is not of a tetrahedron type.");
        continue;
      }

      // case number for the current tetra cell, initially set to zero
      unsigned caseNumber = 0;

      // classify four vertices of the tetra with respect to the signed distance to the
      // line
      double distancesToLine[4];

      // for each vertex in the tetra cell
      for (int vertexIndex = 0; vertexIndex < 4; ++vertexIndex)
      {
        // get the Id of the vertex of the tetra
        const vtkIdType pointId = tet->GetPointId(vertexIndex);

        // compute signed distance between the image of tetra vertex in the range
        // and the control line using Hessian Normal Form:
        //    signedDistance =  dot(p - origin, normal)
        //                   =  dot(p,normal) - dot(origin,normal)
        distancesToLine[vertexIndex] = fieldScalars[0]->GetTuple1(pointId) * normal[0] +
          fieldScalars[1]->GetTuple1(pointId) * normal[1] - dotOriginNormal;

        // classify the tetra vertex based on the sign of the distance
        // if distance == 0 : then p is on the line
        // if distance > 0 : then p is on the right side
        // if distance < 0; then p is on the left side
        if (distancesToLine[vertexIndex] == 0.0)
        {
          // locate the index number using ternaryShift array.
          // please see the comment on this array.
          caseNumber += ternaryShift[vertexIndex];
        }
        else if (distancesToLine[vertexIndex] > 0.0)
        {
          // locate the index number using ternaryShift array.
          // please see the comment on this array.
          caseNumber += 2 * ternaryShift[vertexIndex];
        }
      }

      // extract the base fiber surface using Marching Tetrahedra
      // loop starts only when there is at least one triangle in this case.
      for (int triangleIndex = 0; triangleIndex < nTriangles[caseNumber]; ++triangleIndex)
      {
        // Coordinates for each triangle points
        double trianglePoints[3][3];

        // Clipping parameter for the base fiber surface
        double triangleParameters[3];

        // Clipping case number is initially set to zero.
        unsigned triangleCaseNumber = 0;

        // global index in the input data of the current vertex
        vtkIdType dataIndex;

        // scalar values of the two fields of the current vertex;
        double pointField1, pointField2;

        // For each vertex in the base fiber surface
        for (int pointIndex = 0; pointIndex < 3; ++pointIndex)
        {
          // get the type of the triangle vertex from the lookup table
          const int type = greyTetTriangles[caseNumber][triangleIndex][pointIndex];
          switch (type)
          {
            case bv_vertex_0:
            case bv_vertex_1:
            case bv_vertex_2:
            case bv_vertex_3:
              // if the triangle vertex coincides with the tetra vertex,
              // there will be a grey case.
              // copy grey vertex to output triangle point array
              double point[3];
              dataIndex = tet->GetPointId(type - bv_vertex_0);
              meshPoints->GetPoint(dataIndex, point);
              memcpy(trianglePoints[pointIndex], point, sizeof(point));

              // get the current vertex scalar values
              pointField1 = fieldScalars[0]->GetTuple1(dataIndex);
              pointField2 = fieldScalars[1]->GetTuple1(dataIndex);

              // compute parameter on the parametric line for each triangle point.
              // Assume edgeRange is a vector to represent the vector beteen interpolated
              // range values and origin of polygon line edge.
              // Assume direction is the direction vector of the current polygon edge.
              // The projection of the range values onto the polygon edge
              // can be computed as:
              //     Proj^direction_edgeRange = dot(edgeRange,direction) / |direction|^2
              // this projection is the parameter t used in the clipping case.
              // if t < 0 or t > 1: then the current vertex is outside the current line
              //                    segment of FSCP
              // if 0 << t << 1 : then the current vertex is within the current line
              //                    segment of FSCP
              triangleParameters[pointIndex] = ((pointField1 - origin[0]) * direction[0] +
                                                 (pointField2 - origin[1]) * direction[1]) /
                (length * length);

              if (triangleParameters[pointIndex] > 1.0)
              {
                // locate the index number in the clipping case table
                triangleCaseNumber += 2 * ternaryShift[pointIndex];
              }
              else if (triangleParameters[pointIndex] >= 0.0)
              {
                // locate the index number in the clipping case table
                triangleCaseNumber += ternaryShift[pointIndex];
              }
              // less than zero case adds nothing to triangleCaseNumber
              break;

            case bv_edge_01:
            case bv_edge_02:
            case bv_edge_03:
            case bv_edge_12:
            case bv_edge_13:
            case bv_edge_23:
            {
              // If the triangle vertex happens on the tetra edges,
              // compute interpolation mixing value.
              // Note that (type - bv_edge_01) represents the current edge index.
              // Assume an edge with two end points u and v.
              // Since we are now in edge case, given the signed distances of u and v,
              // the alpha value can be computed as:
              //    alpha = signedDistance(u) / (signedDistance(u) - signedDistance(v))
              const double alpha = distancesToLine[edge2endpoints[type - bv_edge_01][0]] /
                (distancesToLine[edge2endpoints[type - bv_edge_01][0]] -
                                     distancesToLine[edge2endpoints[type - bv_edge_01][1]]);

              // convert enum to pair of endpoints and get their id in the point set
              const vtkIdType pointIds[2] = { tet->GetPointId(edge2endpoints[type - bv_edge_01][0]),
                tet->GetPointId(edge2endpoints[type - bv_edge_01][1]) };

              // get coordinates of the edge end points
              double point0[3], point1[3];
              meshPoints->GetPoint(pointIds[0], point0);
              meshPoints->GetPoint(pointIds[1], point1);

              // compute triangle vertex coordinates using linear interpolation
              trianglePoints[pointIndex][0] = (1.0 - alpha) * point0[0] + alpha * point1[0];
              trianglePoints[pointIndex][1] = (1.0 - alpha) * point0[1] + alpha * point1[1];
              trianglePoints[pointIndex][2] = (1.0 - alpha) * point0[2] + alpha * point1[2];

              // compute triangle vertex range values using linear interpolation.
              const double edgeFields[2] = { (1.0 - alpha) *
                    fieldScalars[0]->GetTuple1(pointIds[0]) +
                  alpha * fieldScalars[0]->GetTuple1(pointIds[1]),
                (1.0 - alpha) * fieldScalars[1]->GetTuple1(pointIds[0]) +
                  alpha * fieldScalars[1]->GetTuple1(pointIds[1]) };

              // compute parameter on the parametric line for each triangle point.
              // Assume edgeRange is a vector to represent the vector beteen interpolated
              // range values and origin of polygon line edge.
              // Assume direction is the direction vector of the current polygon edge.
              // The projection of the range values onto the polygon edge
              // can be computed as:
              //     Proj^direction_edgeRange = dot(edgeRange,direction) / |direction|^2
              // this projection is the parameter t used in the clipping case.
              // if t < 0 or t > 1: then the current vertex is outside the current line
              //                    segment of FSCP
              // if 0 << t << 1 : then the current vertex is within the current line
              //                    segment of FSCP
              triangleParameters[pointIndex] = ((edgeFields[0] - origin[0]) * direction[0] +
                                                 (edgeFields[1] - origin[1]) * direction[1]) /
                (length * length);

              if (triangleParameters[pointIndex] > 1.0)
              {
                // locate the index number in the clipping case table
                triangleCaseNumber += 2 * ternaryShift[pointIndex];
              }
              else if (triangleParameters[pointIndex] >= 0.0)
              {
                // locate the index number in the clipping case table
                triangleCaseNumber += ternaryShift[pointIndex];
              }
              // less than zero case adds nothing to triangleCaseNumber
              break;
            }
            default:
              // report error in case an invalid triangle is being extracted
              vtkErrorMacro(<< "Invalid value in the marching tetrahedra case: " << caseNumber);
              break;
          }
        }
        // clip or cull the triangle from the base fiber surface.
        int counter = 0;
        vtkIdType pts[3];
        for (int tindex = 0; tindex != nClipTriangles[triangleCaseNumber];
             ++tindex)
        {
          for (int vertexIndex = 0; vertexIndex != 3; ++vertexIndex)
          {
            // array to holds indices of the two edge endpoints
            int ids[2];

            // interpolant value of the triangle vertex with respect to the control line
            double alpha;
            const int type = clipTriangleVertices[triangleCaseNumber][tindex][vertexIndex];
            switch (type)
            {
              case vertex_0:
              case vertex_1:
              case vertex_2:
                pts[counter] = newPoints->InsertNextPoint(trianglePoints[type - vertex_0]);
                break;
              case edge_0_parm_0:
              case edge_1_parm_0:
              case edge_2_parm_0:
                ids[0] = clip2points[type - edge_0_parm_0][0];
                ids[1] = clip2points[type - edge_0_parm_0][1];

                // compute which endpoint we are clipping and its interpolant
                alpha = (0.0 - triangleParameters[ids[0]]) /
                  (triangleParameters[ids[1]] - triangleParameters[ids[0]]);

                // interpolate point position to clipping corner
                pts[counter] = newPoints->InsertNextPoint(
                  (1.0 - alpha) * trianglePoints[ids[0]][0] + alpha * trianglePoints[ids[1]][0],
                  (1.0 - alpha) * trianglePoints[ids[0]][1] + alpha * trianglePoints[ids[1]][1],
                  (1.0 - alpha) * trianglePoints[ids[0]][2] + alpha * trianglePoints[ids[1]][2]);
                break;
              case edge_0_parm_1:
              case edge_1_parm_1:
              case edge_2_parm_1:
                ids[0] = clip2points[type - edge_0_parm_1][0];
                ids[1] = clip2points[type - edge_0_parm_1][1];

                // compute which endpoint we are clipping and its interpolant
                alpha = (1.0 - triangleParameters[ids[0]]) /
                  (triangleParameters[ids[1]] - triangleParameters[ids[0]]);

                // interpolate point position to clipping corner
                pts[counter] = newPoints->InsertNextPoint(
                  (1.0 - alpha) * trianglePoints[ids[0]][0] + alpha * trianglePoints[ids[1]][0],
                  (1.0 - alpha) * trianglePoints[ids[0]][1] + alpha * trianglePoints[ids[1]][1],
                  (1.0 - alpha) * trianglePoints[ids[0]][2] + alpha * trianglePoints[ids[1]][2]);
                break;
              default:
                vtkErrorMacro(<< "Invalid value in clipping triangle case: " << triangleCaseNumber);
            }
            if (++counter == 3)
            {
              newPolys->InsertNextCell(3, pts);
              counter = 0;
            }
          }
        }
      }
    }
  }
  // store the fiber surface structure to the output polyData
  output->SetPoints(newPoints);
  output->SetPolys(newPolys);
  return 1;
}
