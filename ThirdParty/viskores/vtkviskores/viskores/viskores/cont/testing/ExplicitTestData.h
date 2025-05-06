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

#include <viskores/CellShape.h>
#include <viskores/Types.h>

namespace viskores
{
namespace cont
{
namespace testing
{

// VTK dataset created from noise.silo in VisIt.
// Resample(5,5,5), IsoVolume(hardglobal [3.8,max])
namespace ExplicitData0
{

static const std::size_t numPoints = 48;
viskores::Float32 coords[numPoints * 3] = {
  -5.000f,  0.000f,  -10.000f, 5.000f,  0.000f,  -5.000f,  0.000f,   5.000f,  -5.000f, 5.000f,
  5.000f,   -5.000f, 10.000f,  5.000f,  -5.000f, 5.000f,   5.000f,   0.000f,  5.000f,  0.000f,
  0.000f,   5.000f,  5.000f,   5.000f,  -5.000f, -1.227f,  -10.000f, -6.101f, 0.000f,  -10.000f,
  -5.000f,  0.000f,  -7.102f,  -1.656f, 0.000f,  -10.000f, 5.000f,   -3.541f, -5.000f, 5.000f,
  0.000f,   -6.985f, 1.451f,   0.000f,  -5.000f, 7.666f,   0.000f,   -5.000f, -5.000f, 1.987f,
  -10.000f, 0.000f,  5.000f,   -6.763f, -1.197f, 5.000f,   -5.000f,  0.000f,  1.663f,  -5.000f,
  5.000f,   5.000f,  -6.749f,  10.000f, 5.000f,  -5.900f,  10.000f,  3.188f,  -5.000f, 0.000f,
  6.828f,   -5.000f, 5.000f,   7.036f,  -5.000f, 10.000f,  5.958f,   -5.000f, 2.119f,  0.000f,
  0.000f,   5.000f,  -4.680f,  0.000f,  9.847f,  0.000f,   0.000f,   0.000f,  5.000f,  -0.505f,
  0.373f,   5.000f,  0.000f,   10.000f, 5.000f,  -3.573f,  7.394f,   5.000f,  0.000f,  5.000f,
  7.425f,   0.000f,  5.000f,   0.000f,  4.298f,  4.414f,   5.000f,   5.000f,  5.000f,  3.063f,
  5.000f,   7.478f,  5.000f,   5.000f,  5.000f,  5.283f,   5.000f,   5.000f,  5.000f,  5.321f,
  2.290f,   2.333f,  -6.099f,  7.533f,  2.638f,  -5.927f,  0.789f,   2.333f,  -2.101f, 8.981f,
  2.638f,   -2.715f, 2.075f,   6.258f,  -2.101f, 7.479f,   6.084f,   -2.715f, 3.381f,  2.613f,
  2.860f,   6.944f,  2.613f,   2.860f,
};

static const std::size_t numCells = 74;
static const std::size_t numConn = 336;
viskores::Id conn[numConn] = {
  8,  9,  10, 0,  10, 11, 8,  0,  12, 13, 14, 1,  12, 15, 13, 1,  16, 10, 9,  0,  11, 10, 16, 0,
  17, 18, 19, 2,  2,  19, 17, 40, 14, 1,  13, 40, 2,  3,  1,  40, 1,  15, 13, 41, 22, 4,  21, 41,
  1,  3,  4,  41, 17, 23, 18, 2,  19, 18, 29, 2,  5,  42, 2,  3,  2,  1,  3,  42, 2,  29, 19, 42,
  5,  4,  43, 3,  4,  3,  1,  43, 4,  22, 31, 43, 23, 29, 18, 2,  2,  23, 29, 44, 30, 33, 5,  44,
  2,  5,  3,  44, 4,  31, 25, 45, 32, 5,  33, 45, 4,  3,  5,  45, 27, 26, 34, 6,  34, 28, 27, 6,
  6,  26, 34, 46, 36, 35, 7,  46, 6,  7,  5,  46, 6,  34, 28, 47, 36, 7,  37, 47, 6,  5,  7,  47,
  36, 35, 39, 7,  39, 37, 36, 7,  38, 39, 35, 7,  38, 37, 39, 7,  3,  2,  17, 20, 40, 1,  3,  20,
  13, 40, 19, 2,  1,  14, 40, 3,  1,  13, 20, 41, 4,  3,  20, 21, 41, 15, 1,  4,  22, 41, 1,  6,
  5,  3,  42, 6,  26, 30, 5,  42, 14, 26, 6,  1,  42, 2,  19, 14, 1,  42, 30, 29, 2,  5,  42, 1,
  3,  5,  6,  43, 6,  5,  32, 28, 43, 15, 1,  6,  28, 43, 4,  1,  15, 22, 43, 32, 5,  4,  31, 43,
  3,  24, 23, 2,  44, 5,  33, 24, 3,  44, 29, 30, 5,  2,  44, 3,  4,  25, 24, 45, 5,  3,  24, 33,
  45, 31, 4,  5,  32, 45, 5,  30, 26, 6,  46, 7,  35, 30, 5,  46, 34, 36, 7,  6,  46, 5,  6,  28,
  32, 47, 7,  5,  32, 37, 47, 34, 6,  7,  36, 47, 3,  20, 24, 2,  17, 23, 4,  21, 25, 3,  20, 24,
  6,  26, 27, 1,  14, 12, 6,  27, 28, 1,  12, 15, 5,  30, 33, 7,  35, 38, 7,  37, 38, 5,  32, 33,
};

viskores::IdComponent numIndices[numCells] = {
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6,
};

viskores::UInt8 shapes[numCells] = {
  viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON,
  viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON,
  viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON,
  viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON,
  viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON,
  viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON,
  viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON,
  viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON,
  viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON,
  viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON,
  viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON,
  viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON,
  viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON,
  viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_WEDGE,
  viskores::CELL_SHAPE_WEDGE,      viskores::CELL_SHAPE_WEDGE,      viskores::CELL_SHAPE_WEDGE,
  viskores::CELL_SHAPE_WEDGE,      viskores::CELL_SHAPE_WEDGE,
};

viskores::Float32 pointData[numPoints] = {
  4.078f, 4.368f, 4.266f, 4.356f, 4.083f, 4.450f, 4.373f, 3.859f, 3.800f, 3.800f, 3.800f, 3.800f,
  3.800f, 3.800f, 3.800f, 3.800f, 3.800f, 3.800f, 3.800f, 3.800f, 3.800f, 3.800f, 3.800f, 3.800f,
  3.800f, 3.800f, 3.800f, 3.800f, 3.800f, 3.800f, 3.800f, 3.800f, 3.800f, 3.800f, 3.800f, 3.800f,
  3.800f, 3.800f, 3.800f, 3.800f, 3.800f, 3.800f, 3.800f, 3.800f, 3.800f, 3.800f, 3.800f, 3.800f,
};
} //ExplicitData0

// VTK dataset created from noise.silo in VisIt.
// Resample(5,5,5), CylinderSlice((-10,-10,-10),(10,10,10), r=6), field=hardyglobal
namespace ExplicitData1
{
static const std::size_t numPoints = 107;
viskores::Float32 coords[numPoints * 3] = {
  -5.000f,  -10.000f, -10.000f, -10.000f, -5.000f,  -10.000f, -10.000f, -10.000f, -5.000f,
  -5.000f,  -5.000f,  -10.000f, -10.000f, -5.000f,  -5.000f,  -5.000f,  -10.000f, -5.000f,
  0.000f,   -5.000f,  -5.000f,  -5.000f,  -5.000f,  -5.000f,  -5.000f,  0.000f,   -5.000f,
  0.000f,   0.000f,   -5.000f,  -5.000f,  -5.000f,  0.000f,   5.000f,   0.000f,   0.000f,
  0.000f,   0.000f,   0.000f,   0.000f,   -5.000f,  0.000f,   -5.000f,  0.000f,   0.000f,
  0.000f,   5.000f,   0.000f,   5.000f,   5.000f,   0.000f,   0.000f,   0.000f,   5.000f,
  10.000f,  5.000f,   5.000f,   5.000f,   5.000f,   5.000f,   5.000f,   0.000f,   5.000f,
  0.000f,   5.000f,   5.000f,   5.000f,   10.000f,  5.000f,   5.000f,   5.000f,   10.000f,
  5.000f,   10.000f,  10.000f,  10.000f,  10.000f,  5.000f,   10.000f,  5.000f,   10.000f,
  -10.000f, -10.000f, -10.000f, -10.000f, -10.000f, -10.000f, -10.000f, -10.000f, -10.000f,
  -3.540f,  -10.000f, -5.000f,  -4.570f,  -10.000f, -10.000f, 0.000f,   -7.260f,  -5.000f,
  0.000f,   -5.000f,  -7.260f,  -3.540f,  -5.000f,  -10.000f, 1.368f,   -5.000f,  -5.000f,
  -5.000f,  -3.540f,  -10.000f, -10.000f, -4.570f,  -10.000f, -5.000f,  0.000f,   -7.260f,
  -7.260f,  0.000f,   -5.000f,  -10.000f, -3.540f,  -5.000f,  0.000f,   0.000f,   -6.600f,
  2.538f,   0.000f,   -5.000f,  -5.000f,  1.368f,   -5.000f,  0.000f,   2.538f,   -5.000f,
  -5.000f,  -10.000f, -3.540f,  -10.000f, -10.000f, -4.570f,  -5.000f,  -7.260f,  0.000f,
  -7.260f,  -5.000f,  0.000f,   -10.000f, -5.000f,  -3.540f,  0.000f,   -6.600f,  0.000f,
  2.538f,   -5.000f,  0.000f,   -6.600f,  0.000f,   0.000f,   5.000f,   -2.900f,  0.000f,
  5.000f,   0.000f,   -2.900f,  6.933f,   0.000f,   0.000f,   -5.000f,  2.538f,   0.000f,
  0.000f,   5.000f,   -2.900f,  -2.900f,  5.000f,   0.000f,   5.000f,   5.000f,   -1.933f,
  7.900f,   5.000f,   0.000f,   0.000f,   6.933f,   0.000f,   5.000f,   7.900f,   0.000f,
  -5.000f,  -5.000f,  1.368f,   0.000f,   -5.000f,  2.538f,   -5.000f,  0.000f,   2.538f,
  0.000f,   -2.900f,  5.000f,   -2.900f,  0.000f,   5.000f,   5.000f,   -1.933f,  5.000f,
  7.900f,   0.000f,   5.000f,   -1.933f,  5.000f,   5.000f,   10.000f,  2.100f,   5.000f,
  10.000f,  5.000f,   2.100f,   0.000f,   7.900f,   5.000f,   5.000f,   10.000f,  2.100f,
  2.100f,   10.000f,  5.000f,   10.000f,  10.000f,  3.067f,   0.000f,   0.000f,   6.933f,
  5.000f,   0.000f,   7.900f,   0.000f,   5.000f,   7.900f,   5.000f,   2.100f,   10.000f,
  2.100f,   5.000f,   10.000f,  10.000f,  3.067f,   10.000f,  3.067f,   10.000f,  10.000f,
  10.000f,  10.000f,  10.000f,  10.000f,  10.000f,  10.000f,  10.000f,  10.000f,  10.000f,
  -7.143f,  -7.143f,  -7.143f,  -2.330f,  -7.452f,  -7.452f,  -7.452f,  -2.330f,  -7.452f,
  -2.708f,  -2.708f,  -8.224f,  -7.452f,  -7.452f,  -2.330f,  -2.708f,  -8.224f,  -2.708f,
  -8.224f,  -2.708f,  -2.708f,  3.289f,   -2.580f,  -2.580f,  -2.580f,  3.289f,   -2.580f,
  2.508f,   2.508f,   -3.547f,  -2.580f,  -2.580f,  3.289f,   2.508f,   -3.547f,  2.508f,
  -3.547f,  2.508f,   2.508f,   8.547f,   2.420f,   2.420f,   2.420f,   8.547f,   2.420f,
  7.580f,   7.580f,   1.453f,   2.420f,   2.420f,   8.547f,   7.580f,   1.453f,   7.580f,
  1.453f,   7.580f,   7.580f,   7.143f,   7.143f,   7.143f,
};

static const std::size_t numCells = 186;
static const std::size_t numConn = 876;
viskores::Id conn[numConn] = {
  0,   1,   2,   87,  0,  3,   1,  87,  1,  4,   2,   87,  2,  5,  0,   87,  3,  88,  6,  7,
  6,   5,   7,   88,  6,  33,  32, 88,  32, 35,  33,  6,   4,  89, 8,   7,   8,  3,   7,  89,
  8,   39,  38,  89,  6,  90,  3,  7,   3,  8,   7,   90,  3,  34, 36,  90,  38, 43,  39, 8,
  44,  41,  42,  9,   4,  10,  91, 7,   10, 7,   5,   91,  10, 47, 48,  91,  6,  5,   92, 7,
  5,   7,   10,  92,  5,  45,  30, 92,  10, 4,   93,  7,   4,  7,  8,   93,  4,  40,  49, 93,
  9,   94,  11,  12,  11, 13,  12, 94,  11, 54,  53,  94,  53, 55, 54,  11,  14, 95,  15, 12,
  15,  9,   12,  95,  15, 58,  57, 95,  11, 96,  9,   12,  9,  15, 12,  96,  9,  42,  44, 96,
  57,  61,  58,  15,  62, 59,  60, 16,  47, 48,  63,  10,  64, 51, 50,  13,  14, 17,  97, 12,
  17,  12,  13,  97,  17, 66,  67, 97,  11, 13,  98,  12,  13, 12, 17,  98,  13, 64,  51, 98,
  56,  65,  52,  14,  17, 14,  99, 12,  14, 12,  15,  99,  14, 56, 65,  99,  16, 100, 18, 19,
  18,  20,  19,  100, 18, 72,  71, 100, 21, 101, 22,  19,  22, 16, 19,  101, 22, 75,  74, 101,
  18,  102, 16,  19,  16, 22,  19, 102, 16, 60,  62,  102, 66, 67, 77,  17,  78, 69,  68, 20,
  21,  23,  103, 19,  23, 19,  20, 103, 23, 80,  81,  103, 18, 20, 104, 19,  20, 19,  23, 104,
  20,  78,  69,  104, 73, 79,  70, 21,  23, 21,  105, 19,  21, 19, 22,  105, 21, 73,  79, 105,
  24,  25,  26,  106, 24, 26,  23, 106, 26, 25,  18,  106, 25, 24, 22,  106, 7,  4,   1,  3,
  87,  5,   7,   3,   0,  87,  2,  4,   7,  5,   87,  5,   0,  3,  7,   88,  0,  31,  34, 3,
  88,  30,  31,  0,   5,  88,  6,  32,  30, 5,   88,  34,  33, 6,  3,   88,  3,  1,   4,  7,
  89,  1,   37,  40,  4,  89,  36, 37,  1,  3,   89,  8,   38, 36, 3,   89,  40, 39,  8,  4,
  89,  8,   9,   6,   7,  90,  9,  41,  33, 6,   90,  38,  41, 9,  8,   90,  3,  36,  38, 8,
  90,  33,  34,  3,   6,  90,  5,  7,   4,  2,   91,  2,   4,  49, 46,  91,  45, 5,   2,  46,
  91,  10,  5,   45,  47, 91,  49, 4,   10, 48,  91,  10,  7,  6,  13,  92,  13, 6,   32, 50,
  92,  47,  10,  13,  50, 92,  5,  10,  47, 45,  92,  32,  6,  5,  30,  92,  8,  7,   10, 14,
  93,  14,  10,  48,  52, 93,  39, 8,   14, 52,  93,  4,   8,  39, 40,  93,  48, 10,  4,  49,
  93,  13,  6,   9,   12, 94,  6,  35,  42, 9,   94,  51,  35, 6,  13,  94,  11, 53,  51, 13,
  94,  42,  54,  11,  9,  94,  9,  8,   14, 12,  95,  8,   43, 56, 14,  95,  44, 43,  8,  9,
  95,  15,  57,  44,  9,  95,  56, 58,  15, 14,  95,  15,  16, 11, 12,  96,  16, 59,  54, 11,
  96,  57,  59,  16,  15, 96,  9,  44,  57, 15,  96,  54,  42, 9,  11,  96,  13, 12,  14, 10,
  97,  10,  14,  65,  63, 97,  64, 13,  10, 63,  97,  17,  13, 64, 66,  97,  65, 14,  17, 67,
  97,  17,  12,  11,  20, 98,  20, 11,  53, 68,  98,  66,  17, 20, 68,  98,  13, 17,  66, 64,
  98,  53,  11,  13,  51, 98,  15, 12,  17, 21,  99,  21,  17, 67, 70,  99,  58, 15,  21, 70,
  99,  14,  15,  58,  56, 99,  67, 17,  14, 65,  99,  20,  11, 16, 19,  100, 11, 55,  60, 16,
  100, 69,  55,  11,  20, 100, 18, 71,  69, 20,  100, 60,  72, 18, 16,  100, 16, 15,  21, 19,
  101, 15,  61,  73,  21, 101, 62, 61,  15, 16,  101, 22,  74, 62, 16,  101, 73, 75,  22, 21,
  101, 22,  25,  18,  19, 102, 25, 76,  72, 18,  102, 74,  76, 25, 22,  102, 16, 62,  74, 22,
  102, 72,  60,  16,  18, 102, 20, 19,  21, 17,  103, 17,  21, 79, 77,  103, 78, 20,  17, 77,
  103, 23,  20,  78,  80, 103, 79, 21,  23, 81,  103, 23,  19, 18, 26,  104, 26, 18,  71, 82,
  104, 80,  23,  26,  82, 104, 20, 23,  80, 78,  104, 71,  18, 20, 69,  104, 22, 19,  23, 24,
  105, 24,  23,  81,  83, 105, 75, 22,  24, 83,  105, 21,  22, 75, 73,  105, 81, 23,  21, 79,
  105, 19,  23,  26,  18, 106, 22, 24,  23, 19,  106, 25,  22, 19, 18,  106, 0,  1,   2,  27,
  28,  29,  6,   33,  35, 9,   41, 42,  9,  41,  44,  8,   38, 43, 13,  50,  51, 6,   32, 35,
  8,   39,  43,  14,  52, 56,  11, 54,  55, 16,  59,  60,  16, 59, 62,  15,  57, 61,  10, 47,
  63,  13,  50,  64,  14, 52,  65, 10,  48, 63,  20,  68,  69, 11, 53,  55,  15, 58,  61, 21,
  70,  73,  17,  66,  77, 20,  68, 78,  21, 70,  79,  17,  67, 77, 84,  85,  86, 24,  26, 25,
  7,   6,   9,   8,   10, 13,  12, 14,  12, 11,  16,  15,  17, 20, 19,  21,
};

viskores::IdComponent numIndices[numCells] = {
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 8, 8,
};

viskores::UInt8 shapes[numCells] = {
  viskores::CELL_SHAPE_TETRA,   viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,   viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,   viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,   viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,   viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,   viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,   viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,   viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,   viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,   viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,   viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,   viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,   viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,   viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,   viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,   viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,   viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,   viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,   viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,   viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,   viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,   viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,   viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,   viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,   viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID, viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_WEDGE,
  viskores::CELL_SHAPE_WEDGE,   viskores::CELL_SHAPE_WEDGE,      viskores::CELL_SHAPE_WEDGE,
  viskores::CELL_SHAPE_WEDGE,   viskores::CELL_SHAPE_WEDGE,      viskores::CELL_SHAPE_WEDGE,
  viskores::CELL_SHAPE_WEDGE,   viskores::CELL_SHAPE_WEDGE,      viskores::CELL_SHAPE_WEDGE,
  viskores::CELL_SHAPE_WEDGE,   viskores::CELL_SHAPE_WEDGE,      viskores::CELL_SHAPE_WEDGE,
  viskores::CELL_SHAPE_WEDGE,   viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON,
};

viskores::Float32 pointData[numPoints] = {
  2.235f, 2.609f, 2.448f, 2.944f, 3.198f, 2.609f, 3.015f, 3.392f, 3.598f, 3.568f, 2.812f, 4.373f,
  3.379f, 3.462f, 3.018f, 3.748f, 4.450f, 3.485f, 3.740f, 3.859f, 3.706f, 3.354f, 2.815f, 2.937f,
  2.460f, 2.368f, 2.759f, 2.085f, 2.085f, 2.085f, 2.507f, 2.234f, 2.672f, 2.907f, 2.895f, 3.165f,
  3.275f, 2.626f, 3.815f, 3.392f, 3.182f, 3.598f, 3.974f, 3.248f, 3.922f, 2.620f, 2.496f, 2.737f,
  3.065f, 3.249f, 3.093f, 3.614f, 3.026f, 4.018f, 4.370f, 4.144f, 2.801f, 4.048f, 3.076f, 4.414f,
  3.662f, 3.530f, 3.673f, 2.889f, 3.578f, 2.995f, 3.604f, 3.188f, 3.608f, 3.432f, 3.326f, 3.446f,
  3.364f, 3.133f, 2.986f, 2.906f, 2.391f, 3.423f, 3.470f, 3.237f, 3.147f, 3.062f, 2.796f, 2.553f,
  2.206f, 2.206f, 2.206f, 2.776f, 2.643f, 3.258f, 3.298f, 2.834f, 2.726f, 3.183f, 3.828f, 3.419f,
  4.146f, 3.251f, 3.684f, 3.077f, 3.610f, 3.245f, 3.215f, 3.268f, 3.258f, 2.978f, 2.991f,
};
} //ExplicitData1

// VTK dataset created from noise.silo in VisIt.
// Resample(6,6,6), CylinderClip((0,0,0) r=8), field=hardyglobal
namespace ExplicitData2
{
static const std::size_t numPoints = 104;
viskores::Float32 coords[numPoints * 3] = {
  -2.000f, -2.000f, -6.000f, 2.000f,  -2.000f, -6.000f, -2.000f, 2.000f,  -6.000f, 2.000f,  2.000f,
  -6.000f, -2.000f, -6.000f, -2.000f, 2.000f,  -6.000f, -2.000f, -6.000f, -2.000f, -2.000f, -2.000f,
  -2.000f, -2.000f, 6.000f,  -2.000f, -2.000f, 2.000f,  -2.000f, -2.000f, -6.000f, 2.000f,  -2.000f,
  -2.000f, 6.000f,  -2.000f, -2.000f, 2.000f,  -2.000f, 2.000f,  6.000f,  -2.000f, 2.000f,  2.000f,
  -2.000f, 6.000f,  2.000f,  -2.000f, -2.000f, -6.000f, 2.000f,  2.000f,  -6.000f, 2.000f,  -6.000f,
  -2.000f, 2.000f,  -2.000f, -2.000f, 6.000f,  -2.000f, -2.000f, 2.000f,  2.000f,  -2.000f, 6.000f,
  2.000f,  -2.000f, 2.000f,  6.000f,  -2.000f, 2.000f,  -6.000f, 2.000f,  2.000f,  -2.000f, 6.000f,
  2.000f,  -2.000f, 2.000f,  2.000f,  -2.000f, 2.000f,  6.000f,  2.000f,  6.000f,  2.000f,  2.000f,
  2.000f,  2.000f,  6.000f,  2.000f,  2.000f,  2.000f,  2.000f,  6.000f,  -2.000f, -4.500f, -6.000f,
  -2.000f, -2.000f, -7.250f, -4.500f, -2.000f, -6.000f, 2.000f,  -2.000f, -7.250f, 2.000f,  -4.500f,
  -6.000f, 4.500f,  -2.000f, -6.000f, -4.500f, 2.000f,  -6.000f, -2.000f, 2.000f,  -7.250f, 2.000f,
  2.000f,  -7.250f, 4.500f,  2.000f,  -6.000f, -2.000f, 4.500f,  -6.000f, 2.000f,  4.500f,  -6.000f,
  -2.000f, -7.250f, -2.000f, -2.000f, -6.000f, -4.500f, -4.500f, -6.000f, -2.000f, 2.000f,  -6.000f,
  -4.500f, 2.000f,  -7.250f, -2.000f, 4.500f,  -6.000f, -2.000f, -6.000f, -4.500f, -2.000f, -6.000f,
  -2.000f, -4.500f, -7.250f, -2.000f, -2.000f, 6.000f,  -4.500f, -2.000f, 6.000f,  -2.000f, -4.500f,
  7.250f,  -2.000f, -2.000f, -7.250f, 2.000f,  -2.000f, -6.000f, 2.000f,  -4.500f, 6.000f,  2.000f,
  -4.500f, 7.250f,  2.000f,  -2.000f, -6.000f, 4.500f,  -2.000f, -2.000f, 6.000f,  -4.500f, -4.500f,
  6.000f,  -2.000f, 2.000f,  6.000f,  -4.500f, 6.000f,  4.500f,  -2.000f, 4.500f,  6.000f,  -2.000f,
  -2.000f, 7.250f,  -2.000f, 2.000f,  7.250f,  -2.000f, -4.500f, -6.000f, 2.000f,  -2.000f, -7.250f,
  2.000f,  2.000f,  -7.250f, 2.000f,  4.500f,  -6.000f, 2.000f,  -7.250f, -2.000f, 2.000f,  -6.000f,
  -4.500f, 2.000f,  6.000f,  -4.500f, 2.000f,  7.250f,  -2.000f, 2.000f,  -7.250f, 2.000f,  2.000f,
  7.250f,  2.000f,  2.000f,  -6.000f, 4.500f,  2.000f,  -4.500f, 6.000f,  2.000f,  4.500f,  6.000f,
  2.000f,  6.000f,  4.500f,  2.000f,  -2.000f, 7.250f,  2.000f,  2.000f,  7.250f,  2.000f,  -2.000f,
  -6.000f, 4.500f,  2.000f,  -6.000f, 4.500f,  -6.000f, -2.000f, 4.500f,  -2.000f, -4.500f, 6.000f,
  -4.500f, -2.000f, 6.000f,  2.000f,  -4.500f, 6.000f,  6.000f,  -2.000f, 4.500f,  4.500f,  -2.000f,
  6.000f,  -6.000f, 2.000f,  4.500f,  -4.500f, 2.000f,  6.000f,  6.000f,  2.000f,  4.500f,  4.500f,
  2.000f,  6.000f,  -2.000f, 4.500f,  6.000f,  -2.000f, 6.000f,  4.500f,  2.000f,  4.500f,  6.000f,
  2.000f,  6.000f,  4.500f,  -2.000f, -2.000f, 7.250f,  2.000f,  -2.000f, 7.250f,  -2.000f, 2.000f,
  7.250f,  2.000f,  2.000f,  7.250f,
};

static const std::size_t numCells = 125;
static const std::size_t numConn = 704;
viskores::Id conn[numConn] = {
  32,  33, 34,  0,  36, 37,  35, 1,   39,  42,  38,  2,   43, 40, 41,  3,  44,  45, 46, 4,   48,
  49,  47, 5,   50, 51, 52,  6,  4,   6,   7,   0,   8,   9,  1,  5,   53, 55,  54, 8,  57,  60,
  56,  10, 11,  12, 10, 2,   13, 14,  3,   15,  64,  58,  59, 15, 61,  66, 62,  11, 67, 63,  65,
  13,  69, 68,  84, 16, 85,  71, 70,  17,  73,  72,  86,  18, 19, 18,  20, 16,  21, 17, 22,  23,
  90,  75, 74,  23, 78, 92,  76, 24,  25,  24,  26,  27,  28, 29, 30,  31, 81,  77, 94, 30,  82,
  97,  79, 25,  83, 80, 99,  28, 87,  88,  100, 19,  101, 91, 89, 21,  96, 102, 93, 27, 98,  95,
  103, 31, 34,  0,  6,  51,  50, 32,  45,  4,   0,   46,  6,  4,  46,  50, 0,   50, 46, 32,  34,
  0,   47, 36,  1,  5,  37,  49, 5,   8,   53,  54,  1,   37, 54, 8,   5,  37,  47, 49, 54,  5,
  38,  57, 10,  2,  60, 42,  2,  11,  61,  62,  10,  60,  62, 11, 2,   60, 38,  42, 62, 2,   58,
  41,  3,  15,  43, 64, 15,  13, 65,  63,  3,   43,  63,  13, 15, 43,  58, 64,  63, 15, 68,  16,
  18,  73, 86,  84, 87, 19,  16, 88,  18,  19,  88,  86,  16, 86, 88,  84, 68,  16, 74, 23,  17,
  71,  85, 90,  91, 21, 23,  89, 17,  21,  89,  85,  23,  85, 89, 90,  74, 23,  93, 27, 24,  92,
  78,  96, 97,  25, 27, 79,  24, 25,  79,  78,  27,  78,  79, 96, 93,  27, 95,  94, 30, 31,  81,
  98,  31, 28,  99, 80, 30,  81, 80,  28,  31,  81,  95,  98, 80, 31,  0,  33,  32, 1,  35,  36,
  0,   34, 33,  2,  38, 39,  1,  35,  37,  3,   40,  41,  3,  40, 43,  2,  39,  42, 4,  45,  44,
  5,   47, 48,  1,  9,  5,   0,  7,   4,   6,   52,  51,  10, 56, 57,  10, 12,  2,  6,  7,   0,
  3,   14, 15,  1,  9,  8,   8,  54,  55,  15,  58,  59,  2,  12, 11,  3,  14,  13, 13, 63,  67,
  11,  61, 66,  16, 68, 69,  4,  46,  44,  17,  70,  71,  5,  48, 49,  18, 72,  73, 6,  52,  50,
  6,   7,  4,   18, 20, 16,  5,  9,   8,   17,  22,  23,  23, 74, 75,  8,  53,  55, 10, 56,  60,
  24,  76, 78,  24, 26, 25,  10, 12,  11,  15,  14,  13,  30, 29, 28,  30, 77,  81, 15, 59,  64,
  11,  62, 66,  25, 79, 82,  28, 80,  83,  13,  65,  67,  16, 69, 84,  17, 70,  85, 17, 22,  21,
  16,  20, 19,  24, 76, 92,  18, 72,  86,  18,  20,  19,  24, 26, 27,  30, 29,  31, 23, 22,  21,
  23,  75, 90,  30, 77, 94,  31, 29,  28,  27,  26,  25,  25, 97, 82,  28, 99,  83, 19, 87,  100,
  21,  89, 101, 27, 93, 102, 19, 88,  100, 21,  91,  101, 31, 95, 103, 27, 102, 96, 31, 103, 98,
  33,  35, 40,  39, 0,  1,   3,  2,   36,  1,   0,   32,  47, 5,  4,   45, 57,  10, 6,  51,  38,
  2,   0,  34,  0,  1,  3,   2,  7,   9,   14,  12,  41,  3,  1,  37,  58, 15,  8,  54, 63,  13,
  11,  61, 43,  3,  2,  42,  4,  5,   17,  16,  44,  48,  70, 69, 68,  16, 4,   46, 73, 18,  6,
  50,  4,  5,   9,  7,  16,  17, 22,  20,  74,  23,  8,   53, 71, 17,  5,  49,  6,  18, 24,  10,
  52,  72, 76,  56, 6,  7,   12, 10,  18,  20,  26,  24,  7,  9,  14,  12, 20,  22, 29, 26,  9,
  8,   15, 14,  22, 23, 30,  29, 55,  75,  77,  59,  8,   23, 30, 15,  78, 24,  10, 60, 79,  25,
  11,  62, 12,  14, 13, 11,  26, 29,  28,  25,  80,  28,  13, 65, 81,  30, 15,  64, 66, 67,  83,
  82,  11, 13,  28, 25, 85,  17, 16,  84,  89,  21,  19,  87, 93, 27,  19, 88,  92, 24, 18,  86,
  20,  22, 29,  26, 19, 21,  31, 27,  94,  30,  23,  90,  95, 31, 21,  91, 98,  31, 27, 96,  99,
  28,  25, 97,  19, 21, 31,  27, 100, 101, 103, 102,
};

viskores::IdComponent numIndices[numCells] = {
  4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4,
  5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5, 5,
  6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
  6, 6, 6, 6, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8, 8,
};

viskores::UInt8 shapes[numCells] = {
  viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,
  viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_TETRA,      viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_PYRAMID,
  viskores::CELL_SHAPE_PYRAMID,    viskores::CELL_SHAPE_WEDGE,      viskores::CELL_SHAPE_WEDGE,
  viskores::CELL_SHAPE_WEDGE,      viskores::CELL_SHAPE_WEDGE,      viskores::CELL_SHAPE_WEDGE,
  viskores::CELL_SHAPE_WEDGE,      viskores::CELL_SHAPE_WEDGE,      viskores::CELL_SHAPE_WEDGE,
  viskores::CELL_SHAPE_WEDGE,      viskores::CELL_SHAPE_WEDGE,      viskores::CELL_SHAPE_WEDGE,
  viskores::CELL_SHAPE_WEDGE,      viskores::CELL_SHAPE_WEDGE,      viskores::CELL_SHAPE_WEDGE,
  viskores::CELL_SHAPE_WEDGE,      viskores::CELL_SHAPE_WEDGE,      viskores::CELL_SHAPE_WEDGE,
  viskores::CELL_SHAPE_WEDGE,      viskores::CELL_SHAPE_WEDGE,      viskores::CELL_SHAPE_WEDGE,
  viskores::CELL_SHAPE_WEDGE,      viskores::CELL_SHAPE_WEDGE,      viskores::CELL_SHAPE_WEDGE,
  viskores::CELL_SHAPE_WEDGE,      viskores::CELL_SHAPE_WEDGE,      viskores::CELL_SHAPE_WEDGE,
  viskores::CELL_SHAPE_WEDGE,      viskores::CELL_SHAPE_WEDGE,      viskores::CELL_SHAPE_WEDGE,
  viskores::CELL_SHAPE_WEDGE,      viskores::CELL_SHAPE_WEDGE,      viskores::CELL_SHAPE_WEDGE,
  viskores::CELL_SHAPE_WEDGE,      viskores::CELL_SHAPE_WEDGE,      viskores::CELL_SHAPE_WEDGE,
  viskores::CELL_SHAPE_WEDGE,      viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON,
  viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON,
  viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON,
  viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON,
  viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON,
  viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON,
  viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON,
  viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON,
  viskores::CELL_SHAPE_HEXAHEDRON, viskores::CELL_SHAPE_HEXAHEDRON,
};

viskores::Float32 pointData[numPoints] = {
  3.558f, 3.219f, 3.738f, 5.246f, 3.204f, 3.535f, 3.536f, 2.668f, 5.001f, 3.375f, 3.336f, 3.141f,
  3.824f, 4.556f, 4.097f, 4.969f, 3.164f, 3.306f, 2.031f, 3.613f, 3.084f, 3.958f, 3.583f, 3.748f,
  3.036f, 3.112f, 3.331f, 3.297f, 3.821f, 3.758f, 4.511f, 3.389f, 4.000f, 3.518f, 3.905f, 3.150f,
  2.684f, 3.079f, 3.081f, 3.707f, 4.622f, 4.555f, 3.523f, 4.312f, 2.990f, 3.867f, 3.187f, 2.803f,
  3.362f, 3.730f, 3.311f, 3.897f, 3.448f, 4.280f, 3.747f, 4.339f, 3.685f, 2.930f, 4.451f, 4.661f,
  2.849f, 3.299f, 2.776f, 4.053f, 4.511f, 4.356f, 3.117f, 4.159f, 3.348f, 2.650f, 3.015f, 3.547f,
  2.188f, 2.923f, 3.713f, 3.605f, 2.992f, 4.290f, 2.941f, 2.970f, 3.878f, 4.137f, 3.047f, 3.633f,
  3.226f, 4.029f, 2.443f, 3.395f, 3.037f, 4.274f, 4.132f, 4.211f, 3.076f, 3.174f, 3.925f, 3.504f,
  3.308f, 3.239f, 3.468f, 3.630f, 3.648f, 3.817f, 3.276f, 3.312f,
};

} //ExplicitData2

} //testing
} //cont
} //viskores
