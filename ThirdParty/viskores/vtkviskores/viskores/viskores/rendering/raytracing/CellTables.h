//============================================================================
//  The contents of this file are covered by the Viskores license. See
//  LICENSE.txt for details.
//
//  By contributing to this file, all contributors agree to the Developer
//  Certificate of Origin Version 1.1 (DCO 1.1) as stated in DCO.txt.
//============================================================================

//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef viskores_rendering_raytracing_CellTables_h
#define viskores_rendering_raytracing_CellTables_h

#include <viskores/Types.h>
#include <viskores/internal/ExportMacros.h>

namespace viskores
{
namespace rendering
{
namespace raytracing
{
struct CellTables
{
  // CellTypeLookUp: LookUp of Shapes to FaceLookUp
  VISKORES_EXEC viskores::Int32 CellTypeLookUp(viskores::Int32 x) const
  {
    VISKORES_STATIC_CONSTEXPR_ARRAY viskores::Int32 lookup[15] = {
      4, // 0 Nothing
      4, // 1 Vertex
      4, // 2 (Not Used) Poly Vertex
      4, // 3 Line
      4, // 4 (Not Used) Poly Line
      4, // 5 Triangle
      4, // 6 (not used) triangle strip
      4, // 7 Polygon
      4, // 8 (Not used)Pixel
      4, // 9 Quad
      1, // 10 Tetra
      4, // 11 (Not used) Voxel
      0, // 12 Hex
      2, // 13 Wedge
      3  // 14  Pyramid
    };
    return lookup[x];
  }

  // FaceLookUp
  VISKORES_EXEC viskores::Int32 FaceLookUp(viskores::Int32 x, viskores::Int32 y) const
  {
    VISKORES_STATIC_CONSTEXPR_ARRAY viskores::Int32 faces[5][3] = {
      { 0, 6, 8 },  //hex offset into shapes face list,  num faces and number of Indices
      { 6, 4, 4 },  //tet
      { 10, 5, 6 }, //wedge
      { 15, 5, 5 }, //pyramid
      { -1, 0, 0 }  //unsupported shape
    };
    return faces[x][y];
  }

  // ShapesFaceList:
  // The convention for the faces is that looking from the outside of
  // the shape at a face, triangles should wind CCW.
  // Quads are broken up by {4=quad,a,b,c,d}:
  // t1 = abc and t2 = acd. Indices of the face are ordered CW, and the mapping
  // of t1 and t2 become CCW.
  // Since we know the triangle winding, we could tell
  // if we hit an inside face or outside face.
  VISKORES_EXEC viskores::Int32 ShapesFaceList(viskores::Int32 x, viskores::Int32 y) const
  {
    VISKORES_STATIC_CONSTEXPR_ARRAY viskores::Int32 shapes[20][5] = {                    //hex
                                                                      { 4, 0, 1, 5, 4 }, //face 0
                                                                      { 4, 1, 2, 6, 5 },
                                                                      { 4, 3, 7, 6, 2 },
                                                                      { 4, 0, 4, 7, 3 },
                                                                      { 4, 0, 3, 2, 1 },
                                                                      { 4, 4, 5, 6, 7 }, //face 5

                                                                      //tet
                                                                      { 3, 0, 3, 1, -1 },
                                                                      { 3, 1, 2, 3, -1 },
                                                                      { 3, 0, 2, 3, -1 },
                                                                      { 3, 0, 2, 1, -1 },

                                                                      //wedge
                                                                      { 3, 0, 1, 2, -1 },
                                                                      { 3, 3, 5, 4, -1 },
                                                                      { 4, 3, 0, 2, 5 },
                                                                      { 4, 1, 4, 5, 2 },
                                                                      { 4, 0, 3, 4, 1 },

                                                                      //pyramid
                                                                      { 3, 0, 4, 1, -1 },
                                                                      { 3, 1, 2, 4, -1 },
                                                                      { 3, 2, 3, 4, -1 },
                                                                      { 3, 0, 4, 3, -1 },
                                                                      { 4, 3, 2, 1, 0 }
    };
    return shapes[x][y];
  }

  // ZooTable:
  // Test of zoo table.
  // Format (faceNumber, triangle)
  VISKORES_EXEC viskores::Int32 ZooTable(viskores::Int32 x, viskores::Int32 y) const
  {
    VISKORES_STATIC_CONSTEXPR_ARRAY viskores::Int32 zoo[30][4] = {
      { 0, 0, 1, 5 }, // hex
      { 0, 0, 5, 4 }, { 1, 1, 2, 6 }, { 1, 1, 6, 5 }, { 2, 3, 7, 6 }, { 2, 3, 6, 2 },
      { 3, 0, 4, 7 }, { 3, 0, 7, 3 }, { 4, 0, 3, 2 }, { 4, 0, 2, 1 }, { 5, 4, 5, 6 },
      { 5, 4, 6, 7 }, { 0, 0, 3, 1 },                                 // Tet
      { 1, 1, 2, 3 }, { 2, 0, 2, 3 }, { 3, 0, 2, 1 }, { 0, 0, 1, 2 }, // Wedge
      { 1, 3, 5, 4 }, { 2, 3, 0, 2 }, { 2, 3, 2, 5 }, { 3, 1, 4, 5 }, { 3, 1, 5, 2 },
      { 4, 0, 3, 4 }, { 4, 0, 4, 1 }, { 0, 0, 4, 1 }, // Pyramid
      { 1, 1, 2, 4 }, { 2, 2, 3, 4 }, { 3, 0, 4, 3 }, { 4, 3, 2, 1 }, { 4, 3, 1, 0 }
    };
    return zoo[x][y];
  }

  // ZooLookUp:
  // Offset into zoo table and the
  // number of triangles for the shape
  //
  VISKORES_EXEC viskores::Int32 ZooLookUp(viskores::Int32 x, viskores::Int32 y) const
  {
    VISKORES_STATIC_CONSTEXPR_ARRAY viskores::Int32 zoo[5][2] = {
      { 0, 12 }, //hex offset into shapes face list,  num faces and number of Indices
      { 12, 4 }, //tet
      { 16, 8 }, //wedge
      { 24, 6 }, //pyramid
      { -1, 0 }  //unsupported shape
    };
    return zoo[x][y];
  }
};

} // namespace raytracing
} // namespace rendering
} // namespace viskores
#endif
