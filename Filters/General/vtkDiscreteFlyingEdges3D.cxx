/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDiscreteFlyingEdges3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDiscreteFlyingEdges3D.h"

#include "vtkArrayListTemplate.h" // For processing attribute data
#include "vtkCellArray.h"
#include "vtkDataArrayRange.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkImageTransform.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkMarchingCubesTriangleCases.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <cmath>

vtkStandardNewMacro(vtkDiscreteFlyingEdges3D);

//----------------------------------------------------------------------------
namespace
{
// This templated class implements the heart of the algorithm.
// vtkDiscreteFlyingEdges3D populates the information in this class and
// then invokes Contour() to actually initiate execution.
template <class T>
class vtkDiscreteFlyingEdges3DAlgorithm
{
public:
  // Edge case table values.
  enum EdgeClass
  {
    BothOutside = 0,  // both vertices outside region
    RightOutside = 1, // right vertex is outside region, left is inside
    LeftOutside = 2,  // left vertex is outside region, right is inside
    BothInside = 3,   // both vertices inside region
  };

  // Dealing with boundary situations when processing volumes.
  enum CellClass
  {
    Interior = 0,
    MinBoundary = 1,
    MaxBoundary = 2
  };

  // Edge-based case table to generate output triangle primitives. It is
  // equivalent to the vertex-based Marching Cubes case table but provides
  // several computational advantages (parallel separability, more efficient
  // computation). This table is built from the MC case table when the class
  // is instantiated.
  unsigned char EdgeCases[256][16];

  // A table to map old edge ids (as defined from vtkMarchingCubesCases) into
  // the edge-based case table. This is so that the existing Marching Cubes
  // case tables can be reused.
  static const unsigned char EdgeMap[12];

  // A table that lists voxel point ids as a function of edge ids (edge ids
  // for edge-based case table).
  static const unsigned char VertMap[12][2];

  // A table describing vertex offsets (in index space) from the cube axes
  // origin for each of the eight vertices of a voxel.
  static const unsigned char VertOffsets[8][3];

  // This table is used to accelerate the generation of output triangles and
  // points. The EdgeUses array, a function of the voxel case number,
  // indicates which voxel edges intersect with the contour (i.e., require
  // interpolation). This array is filled in at instantiation during the case
  // table generation process.
  unsigned char EdgeUses[256][12];

  // Flags indicate whether a particular case requires voxel axes to be
  // processed. A cheap acceleration structure computed from the case
  // tables at the point of instantiation.
  unsigned char IncludesAxes[256];

  // Algorithm-derived data. XCases tracks the x-row edge cases. The
  // EdgeMetaData tracks information needed for parallel partitioning,
  // and to enable generation of the output primitives without using
  // a point locator.
  unsigned char* XCases;
  vtkIdType* EdgeMetaData;

  // Internal variables used by the various algorithm methods. Interfaces VTK
  // image data in a form more convenient to the algorithm.
  T* Scalars;
  vtkIdType Dims[3];
  vtkIdType NumberOfEdges;
  vtkIdType SliceOffset;
  int Min0;
  int Max0;
  int Inc0;
  int Min1;
  int Max1;
  int Inc1;
  int Min2;
  int Max2;
  int Inc2;

  // Output data. Threads write to partitioned memory.
  T* NewScalars;
  vtkCellArray* NewTris;
  float* NewPoints;
  float* NewGradients;
  float* NewNormals;
  bool NeedGradients;
  bool InterpolateAttributes;
  ArrayList Arrays;

  // Setup algorithm
  vtkDiscreteFlyingEdges3DAlgorithm();

  // The three main passes of the algorithm.
  void ProcessXEdge(double value, T const* const inPtr, vtkIdType row, vtkIdType slice); // PASS 1
  void ProcessYZEdges(vtkIdType row, vtkIdType slice);                                   // PASS 2
  void GenerateOutput(double value, T* inPtr, vtkIdType row, vtkIdType slice);           // PASS 4

  // Place holder for now in case fancy bit fiddling is needed later.
  void SetXEdge(unsigned char* ePtr, unsigned char edgeCase) { *ePtr = edgeCase; }

  // Given the four x-edge cases defining this voxel, return the voxel case
  // number.
  unsigned char GetEdgeCase(unsigned char* ePtr[4])
  {
    return (*(ePtr[0]) | ((*(ePtr[1])) << 2) | ((*(ePtr[2])) << 4) | ((*(ePtr[3])) << 6));
  }

  // Return the number of contouring primitives for a particular edge case number.
  unsigned char GetNumberOfPrimitives(unsigned char eCase) { return this->EdgeCases[eCase][0]; }

  // Return an array indicating which voxel edges intersect the contour.
  unsigned char* GetEdgeUses(unsigned char eCase) { return this->EdgeUses[eCase]; }

  // Indicate whether voxel axes need processing for this case.
  unsigned char CaseIncludesAxes(unsigned char eCase) { return this->IncludesAxes[eCase]; }

  // Count edge intersections near volume boundaries.
  void CountBoundaryYZInts(unsigned char loc, unsigned char* edgeCases, vtkIdType* eMD[4]);

  // Produce the output triangles for this voxel cell.
  struct GenerateTrisImpl
  {
    template <typename CellStateT>
    void operator()(
      CellStateT& state, const unsigned char* edges, int numTris, vtkIdType* eIds, vtkIdType& triId)
    {
      using ValueType = typename CellStateT::ValueType;
      auto* offsets = state.GetOffsets();
      auto* conn = state.GetConnectivity();

      auto offsetRange = vtk::DataArrayValueRange<1>(offsets);
      auto offsetIter = offsetRange.begin() + triId;
      auto connRange = vtk::DataArrayValueRange<1>(conn);
      auto connIter = connRange.begin() + (triId * 3);

      for (int i = 0; i < numTris; ++i)
      {
        *offsetIter++ = static_cast<ValueType>(3 * triId++);
        *connIter++ = eIds[*edges++];
        *connIter++ = eIds[*edges++];
        *connIter++ = eIds[*edges++];
      }

      // Write the last offset:
      *offsetIter = static_cast<ValueType>(3 * triId);
    }
  };
  void GenerateTris(unsigned char eCase, unsigned char numTris, vtkIdType* eIds, vtkIdType& triId)
  {
    const unsigned char* edges = this->EdgeCases[eCase] + 1;
    this->NewTris->Visit(GenerateTrisImpl{}, edges, numTris, eIds, triId);
  }

  // Compute gradient on interior point.
  void ComputeGradient(unsigned char loc, vtkIdType ijk[3], T const* const s0_start,
    T const* const s0_end, T const* const s1_start, T const* const s1_end, T const* const s2_start,
    T const* const s2_end, float g[3])
  {
    if (loc == Interior)
    {
      g[0] = 0.5 * (*s0_start - *s0_end);
      g[1] = 0.5 * (*s1_start - *s1_end);
      g[2] = 0.5 * (*s2_start - *s2_end);
    }
    else
    {
      this->ComputeBoundaryGradient(ijk, s0_start, s0_end, s1_start, s1_end, s2_start, s2_end, g);
    }
  }

  // Interpolate along a voxel axes edge.
  void InterpolateAxesEdge(double t, unsigned char loc, T const* const s, const int incs[3],
    vtkIdType vId, vtkIdType ijk0[3], vtkIdType ijk1[3], float g0[3])
  {
    float* x = this->NewPoints + 3 * vId;
    x[0] = ijk0[0] + t * (ijk1[0] - ijk0[0]) + this->Min0;
    x[1] = ijk0[1] + t * (ijk1[1] - ijk0[1]) + this->Min1;
    x[2] = ijk0[2] + t * (ijk1[2] - ijk0[2]) + this->Min2;

    if (this->NeedGradients)
    {
      float g1[3];
      this->ComputeGradient(loc, ijk1, s + incs[0], s - incs[0], s + incs[1], s - incs[1],
        s + incs[2], s - incs[2], g1);

      float gTmp0 = g0[0] + t * (g1[0] - g0[0]);
      float gTmp1 = g0[1] + t * (g1[1] - g0[1]);
      float gTmp2 = g0[2] + t * (g1[2] - g0[2]);
      if (this->NewGradients)
      {
        float* g = this->NewGradients + 3 * vId;
        g[0] = gTmp0;
        g[1] = gTmp1;
        g[2] = gTmp2;
      }

      if (this->NewNormals)
      {
        float* n = this->NewNormals + 3 * vId;
        n[0] = -gTmp0;
        n[1] = -gTmp1;
        n[2] = -gTmp2;
        vtkMath::Normalize(n);
      }
    } // if normals or gradients required

    if (this->InterpolateAttributes)
    {
      vtkIdType v0 = ijk0[0] + ijk0[1] * incs[1] + ijk0[2] * incs[2];
      vtkIdType v1 = ijk1[0] + ijk1[1] * incs[1] + ijk1[2] * incs[2];
      this->Arrays.InterpolateEdge(v0, v1, t, vId);
    }
  }

  // Compute the gradient on a point which may be on the boundary of the volume.
  void ComputeBoundaryGradient(vtkIdType ijk[3], T const* const s0_start, T const* const s0_end,
    T const* const s1_start, T const* const s1_end, T const* const s2_start, T const* const s2_end,
    float g[3]);

  // Interpolate along an arbitrary edge, typically one that may be on the
  // volume boundary. This means careful computation of stuff requiring
  // neighborhood information (e.g., gradients).
  void InterpolateEdge(double value, vtkIdType ijk[3], T const* const s, const int incs[3],
    unsigned char edgeNum, unsigned char const* const edgeUses, vtkIdType* eIds);

  // Produce the output points on the voxel axes for this voxel cell.
  void GeneratePoints(double value, unsigned char loc, vtkIdType ijk[3], T const* const sPtr,
    const int incs[3], unsigned char const* const edgeUses, vtkIdType* eIds);

  // Helper function to set up the point ids on voxel edges.
  unsigned char InitVoxelIds(unsigned char* ePtr[4], vtkIdType* eMD[4], vtkIdType* eIds)
  {
    unsigned char eCase = GetEdgeCase(ePtr);
    eIds[0] = eMD[0][0]; // x-edges
    eIds[1] = eMD[1][0];
    eIds[2] = eMD[2][0];
    eIds[3] = eMD[3][0];
    eIds[4] = eMD[0][1]; // y-edges
    eIds[5] = eIds[4] + this->EdgeUses[eCase][4];
    eIds[6] = eMD[2][1];
    eIds[7] = eIds[6] + this->EdgeUses[eCase][6];
    eIds[8] = eMD[0][2]; // z-edges
    eIds[9] = eIds[8] + this->EdgeUses[eCase][8];
    eIds[10] = eMD[1][2];
    eIds[11] = eIds[10] + this->EdgeUses[eCase][10];
    return eCase;
  }

  // Helper function to advance the point ids along voxel rows.
  void AdvanceVoxelIds(unsigned char eCase, vtkIdType* eIds)
  {
    eIds[0] += this->EdgeUses[eCase][0]; // x-edges
    eIds[1] += this->EdgeUses[eCase][1];
    eIds[2] += this->EdgeUses[eCase][2];
    eIds[3] += this->EdgeUses[eCase][3];
    eIds[4] += this->EdgeUses[eCase][4]; // y-edges
    eIds[5] = eIds[4] + this->EdgeUses[eCase][5];
    eIds[6] += this->EdgeUses[eCase][6];
    eIds[7] = eIds[6] + this->EdgeUses[eCase][7];
    eIds[8] += this->EdgeUses[eCase][8]; // z-edges
    eIds[9] = eIds[8] + this->EdgeUses[eCase][9];
    eIds[10] += this->EdgeUses[eCase][10];
    eIds[11] = eIds[10] + this->EdgeUses[eCase][11];
  }

  // Threading integration via SMPTools
  template <class TT>
  class Pass1
  {
  public:
    vtkDiscreteFlyingEdges3DAlgorithm<TT>* Algo;
    double Value;
    Pass1(vtkDiscreteFlyingEdges3DAlgorithm<TT>* algo, double value)
    {
      this->Algo = algo;
      this->Value = value;
    }
    void operator()(vtkIdType slice, vtkIdType end)
    {
      vtkIdType row;
      TT *rowPtr, *slicePtr = this->Algo->Scalars + slice * this->Algo->Inc2;
      for (; slice < end; ++slice)
      {
        for (row = 0, rowPtr = slicePtr; row < this->Algo->Dims[1]; ++row)
        {
          this->Algo->ProcessXEdge(this->Value, rowPtr, row, slice);
          rowPtr += this->Algo->Inc1;
        } // for all rows in this slice
        slicePtr += this->Algo->Inc2;
      } // for all slices in this batch
    }
  };
  template <class TT>
  class Pass2
  {
  public:
    Pass2(vtkDiscreteFlyingEdges3DAlgorithm<TT>* algo) { this->Algo = algo; }
    vtkDiscreteFlyingEdges3DAlgorithm<TT>* Algo;
    void operator()(vtkIdType slice, vtkIdType end)
    {
      for (; slice < end; ++slice)
      {
        for (vtkIdType row = 0; row < (this->Algo->Dims[1] - 1); ++row)
        {
          this->Algo->ProcessYZEdges(row, slice);
        } // for all rows in this slice
      }   // for all slices in this batch
    }
  };
  template <class TT>
  class Pass4
  {
  public:
    Pass4(vtkDiscreteFlyingEdges3DAlgorithm<TT>* algo, double value)
    {
      this->Algo = algo;
      this->Value = value;
    }
    vtkDiscreteFlyingEdges3DAlgorithm<TT>* Algo;
    double Value;
    void operator()(vtkIdType slice, vtkIdType end)
    {
      vtkIdType row;
      vtkIdType* eMD0 = this->Algo->EdgeMetaData + slice * 6 * this->Algo->Dims[1];
      vtkIdType* eMD1 = eMD0 + 6 * this->Algo->Dims[1];
      TT *rowPtr, *slicePtr = this->Algo->Scalars + slice * this->Algo->Inc2;
      for (; slice < end; ++slice)
      {
        // It's possible to skip entire slices if there is nothing to generate
        if (eMD1[3] > eMD0[3]) // there are triangle primitives!
        {
          for (row = 0, rowPtr = slicePtr; row < this->Algo->Dims[1] - 1; ++row)
          {
            this->Algo->GenerateOutput(this->Value, rowPtr, row, slice);
            rowPtr += this->Algo->Inc1;
          } // for all rows in this slice
        }   // if there are triangles
        slicePtr += this->Algo->Inc2;
        eMD0 = eMD1;
        eMD1 = eMD0 + 6 * this->Algo->Dims[1];
      } // for all slices in this batch
    }
  };

  // Interface between VTK and templated functions
  static void Contour(vtkDiscreteFlyingEdges3D* self, vtkImageData* input, vtkDataArray* inScalars,
    int extent[6], vtkIdType* incs, T* scalars, vtkPolyData* output, vtkPoints* newPts,
    vtkCellArray* newTris, vtkDataArray* newScalars, vtkFloatArray* newNormals,
    vtkFloatArray* newGradients);
};

//----------------------------------------------------------------------------
// Map MC edges numbering to use the saner FlyingEdges edge numbering scheme.
template <class T>
const unsigned char vtkDiscreteFlyingEdges3DAlgorithm<T>::EdgeMap[12] = { 0, 5, 1, 4, 2, 7, 3, 6, 8,
  9, 10, 11 };

//----------------------------------------------------------------------------
// Map MC edges numbering to use the saner FlyingEdges edge numbering scheme.
template <class T>
const unsigned char vtkDiscreteFlyingEdges3DAlgorithm<T>::VertMap[12][2] = {
  { 0, 1 },
  { 2, 3 },
  { 4, 5 },
  { 6, 7 },
  { 0, 2 },
  { 1, 3 },
  { 4, 6 },
  { 5, 7 },
  { 0, 4 },
  { 1, 5 },
  { 2, 6 },
  { 3, 7 },
};

//----------------------------------------------------------------------------
// The offsets of each vertex (in index space) from the voxel axes origin.
template <class T>
const unsigned char vtkDiscreteFlyingEdges3DAlgorithm<T>::VertOffsets[8][3] = {
  { 0, 0, 0 },
  { 1, 0, 0 },
  { 0, 1, 0 },
  { 1, 1, 0 },
  { 0, 0, 1 },
  { 1, 0, 1 },
  { 0, 1, 1 },
  { 1, 1, 1 },
};

//----------------------------------------------------------------------------
// Instantiate and initialize key data members. Mostly we build the
// edge-based case table, and associated acceleration structures, from the
// marching cubes case table. Some of this code is borrowed shamelessly from
// vtkVoxel::Contour() method.
template <class T>
vtkDiscreteFlyingEdges3DAlgorithm<T>::vtkDiscreteFlyingEdges3DAlgorithm()
  : XCases(nullptr)
  , EdgeMetaData(nullptr)
  , NewScalars(nullptr)
  , NewTris(nullptr)
  , NewPoints(nullptr)
  , NewGradients(nullptr)
  , NewNormals(nullptr)
{
  int i, j, k, l, ii, eCase, index, numTris;
  static const int vertMap[8] = { 0, 1, 3, 2, 4, 5, 7, 6 };
  static const int CASE_MASK[8] = { 1, 2, 4, 8, 16, 32, 64, 128 };
  EDGE_LIST* edge;
  vtkMarchingCubesTriangleCases* triCase;
  unsigned char* edgeCase;

  // Initialize cases, increments, and edge intersection flags
  for (eCase = 0; eCase < 256; ++eCase)
  {
    for (j = 0; j < 16; ++j)
    {
      this->EdgeCases[eCase][j] = 0;
    }
    for (j = 0; j < 12; ++j)
    {
      this->EdgeUses[eCase][j] = 0;
    }
    this->IncludesAxes[eCase] = 0;
  }

  // The voxel, edge-based case table is a function of the four x-edge cases
  // that define the voxel. Here we convert the existing MC vertex-based case
  // table into a x-edge case table. Note that the four x-edges are ordered
  // (0->3): x, x+y, x+z, x+y+z; the four y-edges are ordered (4->7): y, y+x,
  // y+z, y+x+z; and the four z-edges are ordered (8->11): z, z+x, z+y,
  // z+x+y.
  for (l = 0; l < 4; ++l)
  {
    for (k = 0; k < 4; ++k)
    {
      for (j = 0; j < 4; ++j)
      {
        for (i = 0; i < 4; ++i)
        {
          // yes we could just count to (0->255) but where's the fun in that?
          eCase = i | (j << 2) | (k << 4) | (l << 6);
          for (ii = 0, index = 0; ii < 8; ++ii)
          {
            if (eCase & (1 << vertMap[ii])) // map into ancient MC table
            {
              index |= CASE_MASK[ii];
            }
          }
          // Now build case table
          triCase = vtkMarchingCubesTriangleCases::GetCases() + index;
          edge = triCase->edges;
          for (numTris = 0, edge = triCase->edges; edge[0] > -1; edge += 3)
          { // count the number of triangles
            numTris++;
          }
          if (numTris > 0)
          {
            edgeCase = this->EdgeCases[eCase];
            *edgeCase++ = numTris;
            for (edge = triCase->edges; edge[0] > -1; edge += 3, edgeCase += 3)
            {
              // Build new case table.
              edgeCase[0] = this->EdgeMap[edge[0]];
              edgeCase[1] = this->EdgeMap[edge[1]];
              edgeCase[2] = this->EdgeMap[edge[2]];
            }
          }
        } // x-edges
      }   // x+y-edges
    }     // x+z-edges
  }       // x+y+z-edges

  // Okay now build the acceleration structure. This is used to generate
  // output points and triangles when processing a voxel x-row as well as to
  // perform other topological reasoning. This structure is a function of the
  // particular case number.
  for (eCase = 0; eCase < 256; ++eCase)
  {
    edgeCase = this->EdgeCases[eCase];
    numTris = *edgeCase++;

    // Mark edges that are used by this case.
    for (i = 0; i < numTris * 3; ++i) // just loop over all edges
    {
      this->EdgeUses[eCase][edgeCase[i]] = 1;
    }

    this->IncludesAxes[eCase] =
      this->EdgeUses[eCase][0] | this->EdgeUses[eCase][4] | this->EdgeUses[eCase][8];

  } // for all cases
}

//----------------------------------------------------------------------------
// Count intersections along voxel axes. When traversing the volume across
// x-edges, the voxel axes on the boundary may be undefined near boundaries
// (because there are no fully-formed cells). Thus the voxel axes on the
// boundary are treated specially.
template <class T>
void vtkDiscreteFlyingEdges3DAlgorithm<T>::CountBoundaryYZInts(
  unsigned char loc, unsigned char* edgeUses, vtkIdType* eMD[4])
{
  switch (loc)
  {
    case 2: //+x boundary
      eMD[0][1] += edgeUses[5];
      eMD[0][2] += edgeUses[9];
      break;
    case 8: //+y
      eMD[1][2] += edgeUses[10];
      break;
    case 10: //+x +y
      eMD[0][1] += edgeUses[5];
      eMD[0][2] += edgeUses[9];
      eMD[1][2] += edgeUses[10];
      eMD[1][2] += edgeUses[11];
      break;
    case 32: //+z
      eMD[2][1] += edgeUses[6];
      break;
    case 34: //+x +z
      eMD[0][1] += edgeUses[5];
      eMD[0][2] += edgeUses[9];
      eMD[2][1] += edgeUses[6];
      eMD[2][1] += edgeUses[7];
      break;
    case 40: //+y +z
      eMD[2][1] += edgeUses[6];
      eMD[1][2] += edgeUses[10];
      break;
    case 42: //+x +y +z happens no more than once per volume
      eMD[0][1] += edgeUses[5];
      eMD[0][2] += edgeUses[9];
      eMD[1][2] += edgeUses[10];
      eMD[1][2] += edgeUses[11];
      eMD[2][1] += edgeUses[6];
      eMD[2][1] += edgeUses[7];
      break;
    default: // uh-oh shouldn't happen
      break;
  }
}

//----------------------------------------------------------------------------
// Compute the gradient when the point may be near the boundary of the
// volume.
template <class T>
void vtkDiscreteFlyingEdges3DAlgorithm<T>::ComputeBoundaryGradient(vtkIdType ijk[3],
  T const* const s0_start, T const* const s0_end, T const* const s1_start, T const* const s1_end,
  T const* const s2_start, T const* const s2_end, float g[3])
{
  const T* s = s0_start - this->Inc0;

  if (ijk[0] == 0)
  {
    g[0] = *s0_start - *s;
  }
  else if (ijk[0] >= (this->Dims[0] - 1))
  {
    g[0] = *s - *s0_end;
  }
  else
  {
    g[0] = 0.5 * (*s0_start - *s0_end);
  }

  if (ijk[1] == 0)
  {
    g[1] = *s1_start - *s;
  }
  else if (ijk[1] >= (this->Dims[1] - 1))
  {
    g[1] = *s - *s1_end;
  }
  else
  {
    g[1] = 0.5 * (*s1_start - *s1_end);
  }

  if (ijk[2] == 0)
  {
    g[2] = *s2_start - *s;
  }
  else if (ijk[2] >= (this->Dims[2] - 1))
  {
    g[2] = *s - *s2_end;
  }
  else
  {
    g[2] = 0.5 * (*s2_start - *s2_end);
  }
}

//----------------------------------------------------------------------------
// Interpolate a new point along a boundary edge. Make sure to consider
// proximity to the boundary when computing gradients, etc.
template <class T>
void vtkDiscreteFlyingEdges3DAlgorithm<T>::InterpolateEdge(double vtkNotUsed(value),
  vtkIdType ijk[3], T const* const s, const int incs[3], unsigned char edgeNum,
  unsigned char const* const edgeUses, vtkIdType* eIds)
{
  // if this edge is not used then get out
  if (!edgeUses[edgeNum])
  {
    return;
  }

  // build the edge information
  const unsigned char* vertMap = this->VertMap[edgeNum];

  vtkIdType ijk0[3], ijk1[3], vId = eIds[edgeNum];

  const unsigned char* offsets = this->VertOffsets[vertMap[0]];
  T const* const s0 = s + offsets[0] * incs[0] + offsets[1] * incs[1] + offsets[2] * incs[2];
  ijk0[0] = ijk[0] + offsets[0];
  ijk0[1] = ijk[1] + offsets[1];
  ijk0[2] = ijk[2] + offsets[2];

  offsets = this->VertOffsets[vertMap[1]];
  T const* const s1 = s + offsets[0] * incs[0] + offsets[1] * incs[1] + offsets[2] * incs[2];
  ijk1[0] = ijk[0] + offsets[0];
  ijk1[1] = ijk[1] + offsets[1];
  ijk1[2] = ijk[2] + offsets[2];

  // Okay interpolate
  double t = 0.5;
  float* xPtr = this->NewPoints + 3 * vId;
  xPtr[0] = ijk0[0] + t * (ijk1[0] - ijk0[0]) + this->Min0;
  xPtr[1] = ijk0[1] + t * (ijk1[1] - ijk0[1]) + this->Min1;
  xPtr[2] = ijk0[2] + t * (ijk1[2] - ijk0[2]) + this->Min2;

  if (this->NeedGradients)
  {
    float g0[3], g1[3];
    this->ComputeBoundaryGradient(
      ijk0, s0 + incs[0], s0 - incs[0], s0 + incs[1], s0 - incs[1], s0 + incs[2], s0 - incs[2], g0);
    this->ComputeBoundaryGradient(
      ijk1, s1 + incs[0], s1 - incs[0], s1 + incs[1], s1 - incs[1], s1 + incs[2], s1 - incs[2], g1);

    float gTmp0 = g0[0] + t * (g1[0] - g0[0]);
    float gTmp1 = g0[1] + t * (g1[1] - g0[1]);
    float gTmp2 = g0[2] + t * (g1[2] - g0[2]);

    if (this->NewGradients)
    {
      float* g = this->NewGradients + 3 * vId;
      g[0] = gTmp0;
      g[1] = gTmp1;
      g[2] = gTmp2;
    }

    if (this->NewNormals)
    {
      float* n = this->NewNormals + 3 * vId;
      n[0] = -gTmp0;
      n[1] = -gTmp1;
      n[2] = -gTmp2;
      vtkMath::Normalize(n);
    }
  } // if normals or gradients required

  if (this->InterpolateAttributes)
  {
    vtkIdType v0 = ijk0[0] + ijk0[1] * incs[1] + ijk0[2] * incs[2];
    vtkIdType v1 = ijk1[0] + ijk1[1] * incs[1] + ijk1[2] * incs[2];
    this->Arrays.InterpolateEdge(v0, v1, t, vId);
  }
}

//----------------------------------------------------------------------------
// Generate the output points and optionally normals, gradients and
// interpolate attributes.
template <class T>
void vtkDiscreteFlyingEdges3DAlgorithm<T>::GeneratePoints(double value, unsigned char loc,
  vtkIdType ijk[3], T const* const sPtr, const int incs[3], unsigned char const* const edgeUses,
  vtkIdType* eIds)
{
  // Create a slightly faster path for voxel axes interior to the volume.
  float g0[3];
  if (this->NeedGradients)
  {
    this->ComputeGradient(loc, ijk, sPtr + incs[0], sPtr - incs[0], sPtr + incs[1], sPtr - incs[1],
      sPtr + incs[2], sPtr - incs[2], g0);
  }

  // Interpolate the cell axes edges
  for (int i = 0; i < 3; ++i)
  {
    if (edgeUses[i * 4])
    {
      // edgesUses[0] == i axes edge
      // edgesUses[4] == j axes edge
      // edgesUses[8] == k axes edge
      vtkIdType ijk1[3] = { ijk[0], ijk[1], ijk[2] };
      ++ijk1[i];

      T const* const sPtr2 = (sPtr + incs[i]);
      double t = 0.5;
      this->InterpolateAxesEdge(t, loc, sPtr2, incs, eIds[i * 4], ijk, ijk1, g0);
    }
  }

  // On the boundary cells special work has to be done to cover the partial
  // cell axes. These are boundary situations where the voxel axes is not
  // fully formed. These situations occur on the +x,+y,+z volume
  // boundaries. (The other cases fall through the default: case which is
  // expected.)
  //
  // Note that loc is one of 27 regions in the volume, with (0,1,2)
  // indicating (interior, min, max) along coordinate axes.
  switch (loc)
  {
    case 2:
    case 6:
    case 18:
    case 22: //+x
      this->InterpolateEdge(value, ijk, sPtr, incs, 5, edgeUses, eIds);
      this->InterpolateEdge(value, ijk, sPtr, incs, 9, edgeUses, eIds);
      break;
    case 8:
    case 9:
    case 24:
    case 25: //+y
      this->InterpolateEdge(value, ijk, sPtr, incs, 1, edgeUses, eIds);
      this->InterpolateEdge(value, ijk, sPtr, incs, 10, edgeUses, eIds);
      break;
    case 32:
    case 33:
    case 36:
    case 37: //+z
      this->InterpolateEdge(value, ijk, sPtr, incs, 2, edgeUses, eIds);
      this->InterpolateEdge(value, ijk, sPtr, incs, 6, edgeUses, eIds);
      break;
    case 10:
    case 26: //+x +y
      this->InterpolateEdge(value, ijk, sPtr, incs, 1, edgeUses, eIds);
      this->InterpolateEdge(value, ijk, sPtr, incs, 5, edgeUses, eIds);
      this->InterpolateEdge(value, ijk, sPtr, incs, 9, edgeUses, eIds);
      this->InterpolateEdge(value, ijk, sPtr, incs, 10, edgeUses, eIds);
      this->InterpolateEdge(value, ijk, sPtr, incs, 11, edgeUses, eIds);
      break;
    case 34:
    case 38: //+x +z
      this->InterpolateEdge(value, ijk, sPtr, incs, 2, edgeUses, eIds);
      this->InterpolateEdge(value, ijk, sPtr, incs, 5, edgeUses, eIds);
      this->InterpolateEdge(value, ijk, sPtr, incs, 9, edgeUses, eIds);
      this->InterpolateEdge(value, ijk, sPtr, incs, 6, edgeUses, eIds);
      this->InterpolateEdge(value, ijk, sPtr, incs, 7, edgeUses, eIds);
      break;
    case 40:
    case 41: //+y +z
      this->InterpolateEdge(value, ijk, sPtr, incs, 1, edgeUses, eIds);
      this->InterpolateEdge(value, ijk, sPtr, incs, 2, edgeUses, eIds);
      this->InterpolateEdge(value, ijk, sPtr, incs, 3, edgeUses, eIds);
      this->InterpolateEdge(value, ijk, sPtr, incs, 6, edgeUses, eIds);
      this->InterpolateEdge(value, ijk, sPtr, incs, 10, edgeUses, eIds);
      break;
    case 42: //+x +y +z happens no more than once per volume
      this->InterpolateEdge(value, ijk, sPtr, incs, 1, edgeUses, eIds);
      this->InterpolateEdge(value, ijk, sPtr, incs, 2, edgeUses, eIds);
      this->InterpolateEdge(value, ijk, sPtr, incs, 3, edgeUses, eIds);
      this->InterpolateEdge(value, ijk, sPtr, incs, 5, edgeUses, eIds);
      this->InterpolateEdge(value, ijk, sPtr, incs, 9, edgeUses, eIds);
      this->InterpolateEdge(value, ijk, sPtr, incs, 10, edgeUses, eIds);
      this->InterpolateEdge(value, ijk, sPtr, incs, 11, edgeUses, eIds);
      this->InterpolateEdge(value, ijk, sPtr, incs, 6, edgeUses, eIds);
      this->InterpolateEdge(value, ijk, sPtr, incs, 7, edgeUses, eIds);
      break;
    default: // interior, or -x,-y,-z boundaries
      return;
  }
}

//----------------------------------------------------------------------------
// PASS 1: Process a single volume x-row (and all of the voxel edges that
// compose the row). Determine the x-edges case classification, count the
// number of x-edge intersections, and figure out where intersections along
// the x-row begins and ends (i.e., gather information for computational
// trimming).
template <class T>
void vtkDiscreteFlyingEdges3DAlgorithm<T>::ProcessXEdge(
  double value, T const* const inPtr, vtkIdType row, vtkIdType slice)
{
  vtkIdType nxcells = this->Dims[0] - 1;
  vtkIdType minInt = nxcells, maxInt = 0;
  vtkIdType* edgeMetaData;
  unsigned char edgeCase, *ePtr = this->XCases + slice * this->SliceOffset + row * nxcells;
  T s0, s1 = (*inPtr);
  T labelValue = static_cast<T>(value);
  vtkIdType sum = 0;

  // run along the entire x-edge computing edge cases
  edgeMetaData = this->EdgeMetaData + (slice * this->Dims[1] + row) * 6;
  std::fill_n(edgeMetaData, 6, 0);

  // pull this out help reduce false sharing
  vtkIdType inc0 = this->Inc0;

  for (vtkIdType i = 0; i < nxcells; ++i, ++ePtr)
  {
    s0 = s1;
    s1 = static_cast<double>(*(inPtr + (i + 1) * inc0));

    if (s0 != labelValue)
    {
      edgeCase = (s1 != value ? vtkDiscreteFlyingEdges3DAlgorithm::BothOutside
                              : vtkDiscreteFlyingEdges3DAlgorithm::LeftOutside);
    }
    else // s0 == labelValue
    {
      edgeCase = (s1 != value ? vtkDiscreteFlyingEdges3DAlgorithm::RightOutside
                              : vtkDiscreteFlyingEdges3DAlgorithm::BothInside);
    }

    this->SetXEdge(ePtr, edgeCase);

    // if edge intersects contour
    if (edgeCase == vtkDiscreteFlyingEdges3DAlgorithm::LeftOutside ||
      edgeCase == vtkDiscreteFlyingEdges3DAlgorithm::RightOutside)
    {
      ++sum; // increment number of intersections along x-edge
      if (i < minInt)
      {
        minInt = i;
      }
      maxInt = i + 1;
    } // if contour interacts with this x-edge
  }   // for all x-cell edges along this x-edge

  edgeMetaData[0] += sum; // write back the number of intersections along x-edge

  // The beginning and ending of intersections along the edge is used for
  // computational trimming.
  edgeMetaData[4] = minInt; // where intersections start along x edge
  edgeMetaData[5] = maxInt; // where intersections end along x edge
}

//----------------------------------------------------------------------------
// PASS 2: Process a single x-row of voxels. Count the number of y- and
// z-intersections by topological reasoning from x-edge cases. Determine the
// number of primitives (i.e., triangles) generated from this row. Use
// computational trimming to reduce work. Note *ePtr[4] is four pointers to
// four x-edge rows that bound the voxel x-row and which contain edge case
// information.
template <class T>
void vtkDiscreteFlyingEdges3DAlgorithm<T>::ProcessYZEdges(vtkIdType row, vtkIdType slice)
{
  // Grab the four edge cases bounding this voxel x-row.
  unsigned char *ePtr[4], ec0, ec1, ec2, ec3, xInts = 1;
  ePtr[0] = this->XCases + slice * this->SliceOffset + row * (this->Dims[0] - 1);
  ePtr[1] = ePtr[0] + this->Dims[0] - 1;
  ePtr[2] = ePtr[0] + this->SliceOffset;
  ePtr[3] = ePtr[2] + this->Dims[0] - 1;

  // Grab the edge meta data surrounding the voxel row.
  vtkIdType* eMD[4];
  eMD[0] = this->EdgeMetaData + (slice * this->Dims[1] + row) * 6; // this x-edge
  eMD[1] = eMD[0] + 6;                                             // x-edge in +y direction
  eMD[2] = eMD[0] + this->Dims[1] * 6;                             // x-edge in +z direction
  eMD[3] = eMD[2] + 6;                                             // x-edge in +y+z direction

  // Determine whether this row of x-cells needs processing. If there are no
  // x-edge intersections, and the state of the four bounding x-edges is the
  // same, then there is no need for processing.
  if ((eMD[0][0] | eMD[1][0] | eMD[2][0] | eMD[3][0]) == 0) // any x-ints?
  {
    if (*(ePtr[0]) == *(ePtr[1]) && *(ePtr[1]) == *(ePtr[2]) && *(ePtr[2]) == *(ePtr[3]))
    {
      return; // there are no y- or z-ints, thus no contour, skip voxel row
    }
    else
    {
      xInts = 0; // there are y- or z- edge ints however
    }
  }

  // Determine proximity to the boundary of volume. This information is used
  // to count edge intersections in boundary situations.
  unsigned char loc, yLoc, zLoc, yzLoc;
  yLoc = (row >= (this->Dims[1] - 2) ? MaxBoundary : Interior);
  zLoc = (slice >= (this->Dims[2] - 2) ? MaxBoundary : Interior);
  yzLoc = (yLoc << 2) | (zLoc << 4);

  // The trim edges may need adjustment if the contour travels between rows
  // of x-edges (without intersecting these x-edges). This means checking
  // whether the trim faces at (xL,xR) made up of the y-z edges intersect the
  // contour. Basically just an intersection operation. Determine the voxel
  // row trim edges, need to check all four x-edges.
  vtkIdType xL = eMD[0][4], xR = eMD[0][5];
  vtkIdType i;
  if (xInts)
  {
    for (i = 1; i < 4; ++i)
    {
      xL = (eMD[i][4] < xL ? eMD[i][4] : xL);
      xR = (eMD[i][5] > xR ? eMD[i][5] : xR);
    }

    if (xL > 0) // if trimmed in the -x direction
    {
      ec0 = *(ePtr[0] + xL);
      ec1 = *(ePtr[1] + xL);
      ec2 = *(ePtr[2] + xL);
      ec3 = *(ePtr[3] + xL);
      if ((ec0 & 0x1) != (ec1 & 0x1) || (ec1 & 0x1) != (ec2 & 0x1) || (ec2 & 0x1) != (ec3 & 0x1))
      {
        xL = eMD[0][4] = 0; // reset left trim
      }
    }

    if (xR < (this->Dims[0] - 1)) // if trimmed in the +x direction
    {
      ec0 = *(ePtr[0] + xR);
      ec1 = *(ePtr[1] + xR);
      ec2 = *(ePtr[2] + xR);
      ec3 = *(ePtr[3] + xR);
      if ((ec0 & 0x2) != (ec1 & 0x2) || (ec1 & 0x2) != (ec2 & 0x2) || (ec2 & 0x2) != (ec3 & 0x2))
      {
        xR = eMD[0][5] = this->Dims[0] - 1; // reset right trim
      }
    }
  }
  else // contour cuts through without intersecting x-edges, reset trim edges
  {
    xL = eMD[0][4] = 0;
    xR = eMD[0][5] = this->Dims[0] - 1;
  }

  // Okay run along the x-voxels and count the number of y- and
  // z-intersections. Here we are just checking y,z edges that make up the
  // voxel axes. Also check the number of primitives generated.
  unsigned char *edgeUses, eCase, numTris;
  ePtr[0] += xL;
  ePtr[1] += xL;
  ePtr[2] += xL;
  ePtr[3] += xL;
  const vtkIdType dim0Wall = this->Dims[0] - 2;
  for (i = xL; i < xR; ++i) // run along the trimmed x-voxels
  {
    eCase = this->GetEdgeCase(ePtr);
    if ((numTris = this->GetNumberOfPrimitives(eCase)) > 0)
    {
      // Okay let's increment the triangle count.
      eMD[0][3] += numTris;

      // Count the number of y- and z-points to be generated. Pass# 1 counted
      // the number of x-intersections along the x-edges. Now we count all
      // intersections on the y- and z-voxel axes.
      edgeUses = this->GetEdgeUses(eCase);
      eMD[0][1] += edgeUses[4]; // y-voxel axes edge always counted
      eMD[0][2] += edgeUses[8]; // z-voxel axes edge always counted
      loc = yzLoc | (i >= dim0Wall ? MaxBoundary : Interior);
      if (loc != 0)
      {
        this->CountBoundaryYZInts(loc, edgeUses, eMD);
      }
    } // if cell contains contour

    // advance the four pointers along voxel row
    ePtr[0]++;
    ePtr[1]++;
    ePtr[2]++;
    ePtr[3]++;
  } // for all voxels along this x-edge
}

//----------------------------------------------------------------------------
// PASS 4: Process the x-row cells to generate output primitives, including
// point coordinates and triangles. This is the fourth and final pass of the
// algorithm.
template <class T>
void vtkDiscreteFlyingEdges3DAlgorithm<T>::GenerateOutput(
  double value, T* rowPtr, vtkIdType row, vtkIdType slice)
{
  // Grab the edge meta data surrounding the voxel row.
  vtkIdType* eMD[4];
  eMD[0] = this->EdgeMetaData + (slice * this->Dims[1] + row) * 6; // this x-edge
  eMD[1] = eMD[0] + 6;                                             // x-edge in +y direction
  eMD[2] = eMD[0] + this->Dims[1] * 6;                             // x-edge in +z direction
  eMD[3] = eMD[2] + 6;                                             // x-edge in +y+z direction

  // Return if there is nothing to do (i.e., no triangles to generate)
  if (eMD[0][3] == eMD[1][3])
  {
    return;
  }

  // Get the voxel row trim edges and prepare to generate. Find the voxel row
  // trim edges, need to check all four x-edges to compute row trim edge.
  vtkIdType xL = eMD[0][4], xR = eMD[0][5];
  vtkIdType i;
  for (i = 1; i < 4; ++i)
  {
    xL = (eMD[i][4] < xL ? eMD[i][4] : xL);
    xR = (eMD[i][5] > xR ? eMD[i][5] : xR);
  }

  // Grab the four edge cases bounding this voxel x-row. Begin at left trim edge.
  unsigned char* ePtr[4];
  ePtr[0] = this->XCases + slice * this->SliceOffset + row * (this->Dims[0] - 1) + xL;
  ePtr[1] = ePtr[0] + this->Dims[0] - 1;
  ePtr[2] = ePtr[0] + this->SliceOffset;
  ePtr[3] = ePtr[2] + this->Dims[0] - 1;

  // Traverse all voxels in this row, those containing the contour are
  // further identified for processing, meaning generating points and
  // triangles. Begin by setting up point ids on voxel edges.
  vtkIdType triId = eMD[0][3];
  vtkIdType eIds[12]; // the ids of generated points

  unsigned char eCase = this->InitVoxelIds(ePtr, eMD, eIds);

  // Determine the proximity to the boundary of volume. This information is
  // used to generate edge intersections.
  unsigned char loc, yLoc, zLoc, yzLoc;
  yLoc = (row < 1 ? MinBoundary : (row >= (this->Dims[1] - 2) ? MaxBoundary : Interior));
  zLoc = (slice < 1 ? MinBoundary : (slice >= (this->Dims[2] - 2) ? MaxBoundary : Interior));
  yzLoc = (yLoc << 2) | (zLoc << 4);

  // compute the ijk for this section
  vtkIdType ijk[3] = { xL, row, slice };

  // load the inc0/inc1/inc2 into local memory
  const int incs[3] = { this->Inc0, this->Inc1, this->Inc2 };
  const T* sPtr = rowPtr + xL * incs[0];
  const vtkIdType dim0Wall = this->Dims[0] - 2;
  const vtkIdType endVoxel = xR - 1;

  for (i = xL; i < xR; ++i)
  {
    const unsigned char numTris = this->GetNumberOfPrimitives(eCase);
    if (numTris > 0)
    {
      // Start by generating triangles for this case
      this->GenerateTris(eCase, numTris, eIds, triId);

      // Now generate point(s) along voxel axes if needed. Remember to take
      // boundary into account.
      loc = yzLoc | (i < 1 ? MinBoundary : (i >= dim0Wall ? MaxBoundary : Interior));
      if (this->CaseIncludesAxes(eCase) || loc != Interior)
      {
        unsigned char const* const edgeUses = this->GetEdgeUses(eCase);
        this->GeneratePoints(value, loc, ijk, sPtr, incs, edgeUses, eIds);
      }
      this->AdvanceVoxelIds(eCase, eIds);
    }

    // Advance along voxel row if not at the end. Saves a little work.
    if (i < endVoxel)
    {
      ePtr[0]++;
      ePtr[1]++;
      ePtr[2]++;
      ePtr[3]++;
      eCase = this->GetEdgeCase(ePtr);

      ++ijk[0];
      sPtr += incs[0];
    } // if not at end of voxel row
  }   // for all non-trimmed cells along this x-edge
}

//----------------------------------------------------------------------------
// Contouring filter specialized for 3D volumes. This templated function
// interfaces the vtkDiscreteFlyingEdges3D class with the templated algorithm
// class. It also invokes the three passes of the Flying Edges algorithm.
template <class T>
void vtkDiscreteFlyingEdges3DAlgorithm<T>::Contour(vtkDiscreteFlyingEdges3D* self,
  vtkImageData* input, vtkDataArray* inScalars, int extent[6], vtkIdType* incs, T* scalars,
  vtkPolyData* output, vtkPoints* newPts, vtkCellArray* newTris, vtkDataArray* newScalars,
  vtkFloatArray* newNormals, vtkFloatArray* newGradients)
{
  double value, *values = self->GetValues();
  vtkIdType numContours = self->GetNumberOfContours();
  vtkIdType vidx, row, slice, *eMD, zInc;
  vtkIdType numOutXPts, numOutYPts, numOutZPts, numOutTris;
  vtkIdType numXPts = 0, numYPts = 0, numZPts = 0, numTris = 0;
  vtkIdType startXPts, startYPts, startZPts, startTris;
  startXPts = startYPts = startZPts = startTris = 0;

  // This may be subvolume of the total 3D image. Capture information for
  // subsequent processing.
  vtkDiscreteFlyingEdges3DAlgorithm<T> algo;
  algo.Scalars = scalars;
  algo.Min0 = extent[0];
  algo.Max0 = extent[1];
  algo.Inc0 = incs[0];
  algo.Min1 = extent[2];
  algo.Max1 = extent[3];
  algo.Inc1 = incs[1];
  algo.Min2 = extent[4];
  algo.Max2 = extent[5];
  algo.Inc2 = incs[2];

  // Now allocate working arrays. The XCases array tracks x-edge cases.
  algo.Dims[0] = algo.Max0 - algo.Min0 + 1;
  algo.Dims[1] = algo.Max1 - algo.Min1 + 1;
  algo.Dims[2] = algo.Max2 - algo.Min2 + 1;
  algo.NumberOfEdges = algo.Dims[1] * algo.Dims[2];
  algo.SliceOffset = (algo.Dims[0] - 1) * algo.Dims[1];
  algo.XCases = new unsigned char[(algo.Dims[0] - 1) * algo.NumberOfEdges];

  // Also allocate the characterization (metadata) array for the x edges.
  // This array tracks the number of x-, y- and z- intersections on the voxel
  // axes along an x-edge; as well as the number of the output triangles, and
  // the xMin_i and xMax_i (minimum index of first intersection, maximum
  // index of intersection for the ith x-row, the so-called trim edges used
  // for computational trimming).
  algo.EdgeMetaData = new vtkIdType[algo.NumberOfEdges * 6];

  // Interpolating attributes and other stuff. Interpolate extra attributes only if they
  // exist and the user requests it.
  algo.NeedGradients = (newGradients || newNormals);
  algo.InterpolateAttributes =
    (self->GetInterpolateAttributes() && input->GetPointData()->GetNumberOfArrays() > 1) ? true
                                                                                         : false;

  // Loop across each contour value. This encompasses all three passes.
  for (vidx = 0; vidx < numContours; vidx++)
  {
    value = values[vidx];

    // PASS 1: Traverse all x-rows building edge cases and counting number of
    // intersections (i.e., accumulate information necessary for later output
    // memory allocation, e.g., the number of output points along the x-rows
    // are counted).
    Pass1<T> pass1(&algo, value);
    vtkSMPTools::For(0, algo.Dims[2], pass1);

    // PASS 2: Traverse all voxel x-rows and process voxel y&z edges.  The
    // result is a count of the number of y- and z-intersections, as well as
    // the number of triangles generated along these voxel rows.
    Pass2<T> pass2(&algo);
    vtkSMPTools::For(0, algo.Dims[2] - 1, pass2);

    // PASS 3: Now allocate and generate output. First we have to update the
    // edge meta data to partition the output into separate pieces so
    // independent threads can write without collisions. Once allocation is
    // complete, the volume is processed on a voxel row by row basis to
    // produce output points and triangles, and interpolate point attribute
    // data (as necessary). NOTE: This implementation is serial. It is
    // possible to use a threaded prefix sum to make it even faster. Since
    // this pass usually takes a small amount of time, we choose simplicity
    // over performance.
    numOutXPts = startXPts;
    numOutYPts = startYPts;
    numOutZPts = startZPts;
    numOutTris = startTris;

    // Count number of points and tris generate along each cell row
    for (slice = 0; slice < algo.Dims[2]; ++slice)
    {
      zInc = slice * algo.Dims[1];
      for (row = 0; row < algo.Dims[1]; ++row)
      {
        eMD = algo.EdgeMetaData + (zInc + row) * 6;
        numXPts = eMD[0];
        numYPts = eMD[1];
        numZPts = eMD[2];
        numTris = eMD[3];
        eMD[0] = numOutXPts + numOutYPts + numOutZPts;
        eMD[1] = eMD[0] + numXPts;
        eMD[2] = eMD[1] + numYPts;
        eMD[3] = numOutTris;
        numOutXPts += numXPts;
        numOutYPts += numYPts;
        numOutZPts += numZPts;
        numOutTris += numTris;
      }
    }

    // Output can now be allocated.
    vtkIdType totalPts = numOutXPts + numOutYPts + numOutZPts;
    if (totalPts > 0)
    {
      newPts->GetData()->WriteVoidPointer(0, 3 * totalPts);
      algo.NewPoints = static_cast<float*>(newPts->GetVoidPointer(0));
      newTris->ResizeExact(numOutTris, 3 * numOutTris);
      algo.NewTris = newTris;
      if (newScalars)
      {
        vtkIdType numPrevPts = newScalars->GetNumberOfTuples();
        vtkIdType numNewPts = totalPts - numPrevPts;
        newScalars->WriteVoidPointer(0, totalPts);
        algo.NewScalars = static_cast<T*>(newScalars->GetVoidPointer(0));
        T TValue = static_cast<T>(value);
        std::fill_n(algo.NewScalars + numPrevPts, numNewPts, TValue);
      }
      if (newGradients)
      {
        newGradients->WriteVoidPointer(0, 3 * totalPts);
        algo.NewGradients = static_cast<float*>(newGradients->GetVoidPointer(0));
      }
      if (newNormals)
      {
        newNormals->WriteVoidPointer(0, 3 * totalPts);
        algo.NewNormals = static_cast<float*>(newNormals->GetVoidPointer(0));
      }
      if (algo.InterpolateAttributes)
      {
        if (vidx == 0) // first contour
        {
          // Make sure we don't interpolate the input scalars twice; or generate scalars
          // when ComputeScalars is off.
          output->GetPointData()->InterpolateAllocate(input->GetPointData(), totalPts);
          output->GetPointData()->RemoveArray(inScalars->GetName());
          algo.Arrays.ExcludeArray(inScalars);
          algo.Arrays.AddArrays(totalPts, input->GetPointData(), output->GetPointData());
        }
        else
        {
          algo.Arrays.Realloc(totalPts);
        }
      }

      // PASS 4: Fourth and final pass: Process voxel rows and generate output.
      // Note that we are simultaneously generating triangles and interpolating
      // points. These could be split into separate, parallel operations for
      // maximum performance.
      Pass4<T> pass4(&algo, value);
      vtkSMPTools::For(0, algo.Dims[2] - 1, pass4);
    } // if anything generated

    // Handle multiple contours
    startXPts = numOutXPts;
    startYPts = numOutYPts;
    startZPts = numOutZPts;
    startTris = numOutTris;
  } // for all contour values

  // Clean up and return
  delete[] algo.XCases;
  delete[] algo.EdgeMetaData;
}

} // anonymous namespace

//----------------------------------------------------------------------------
// Here is the VTK class proper.
// Construct object with a single contour value of 0.0.
vtkDiscreteFlyingEdges3D::vtkDiscreteFlyingEdges3D()
{
  this->ContourValues = vtkContourValues::New();
  this->ComputeNormals = 1;
  this->ComputeGradients = 0;
  this->ComputeScalars = 1;
  this->InterpolateAttributes = 0;
  this->ArrayComponent = 0;

  // by default process active point scalars
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
vtkDiscreteFlyingEdges3D::~vtkDiscreteFlyingEdges3D()
{
  this->ContourValues->Delete();
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If contour values are modified,
// then this object is modified as well.
vtkMTimeType vtkDiscreteFlyingEdges3D::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  vtkMTimeType mTime2 = this->ContourValues->GetMTime();
  return (mTime2 > mTime ? mTime2 : mTime);
}

//----------------------------------------------------------------------------
int vtkDiscreteFlyingEdges3D::RequestUpdateExtent(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // These require extra ghost levels
  if (this->ComputeGradients || this->ComputeNormals)
  {
    vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
    vtkInformation* outInfo = outputVector->GetInformationObject(0);

    int ghostLevels;
    ghostLevels = outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
    inInfo->Set(vtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevels + 1);
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkDiscreteFlyingEdges3D::RequestData(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkDebugMacro(<< "Executing 3D flying edges");

  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkImageData* input = vtkImageData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // to be safe recompute the update extent
  this->RequestUpdateExtent(request, inputVector, outputVector);
  vtkDataArray* inScalars = this->GetInputArrayToProcess(0, inputVector);

  // Determine extent
  int* inExt = input->GetExtent();
  int exExt[6];
  inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), exExt);
  for (int i = 0; i < 3; i++)
  {
    if (inExt[2 * i] > exExt[2 * i])
    {
      exExt[2 * i] = inExt[2 * i];
    }
    if (inExt[2 * i + 1] < exExt[2 * i + 1])
    {
      exExt[2 * i + 1] = inExt[2 * i + 1];
    }
  }
  if (exExt[0] >= exExt[1] || exExt[2] >= exExt[3] || exExt[4] >= exExt[5])
  {
    vtkDebugMacro(<< "3D structured contours requires 3D data");
    return 0;
  }

  // Check data type and execute appropriate function
  //
  if (inScalars == nullptr)
  {
    vtkDebugMacro("No scalars for contouring.");
    return 0;
  }
  int numComps = inScalars->GetNumberOfComponents();

  if (this->ArrayComponent >= numComps)
  {
    vtkErrorMacro("Scalars have " << numComps
                                  << " components. "
                                     "ArrayComponent must be smaller than "
                                  << numComps);
    return 0;
  }

  // Create necessary objects to hold output. We will defer the
  // actual allocation to a later point.
  vtkCellArray* newTris = vtkCellArray::New();
  vtkPoints* newPts = vtkPoints::New();
  newPts->SetDataTypeToFloat();
  vtkDataArray* newScalars = nullptr;
  vtkFloatArray* newNormals = nullptr;
  vtkFloatArray* newGradients = nullptr;

  if (this->ComputeScalars)
  {
    newScalars = inScalars->NewInstance();
    newScalars->SetNumberOfComponents(1);
    newScalars->SetName(inScalars->GetName());
  }
  if (this->ComputeNormals)
  {
    newNormals = vtkFloatArray::New();
    newNormals->SetNumberOfComponents(3);
    newNormals->SetName("Normals");
  }
  if (this->ComputeGradients)
  {
    newGradients = vtkFloatArray::New();
    newGradients->SetNumberOfComponents(3);
    newGradients->SetName("Gradients");
  }

  void* ptr = input->GetArrayPointerForExtent(inScalars, exExt);
  vtkIdType incs[3];
  input->GetIncrements(inScalars, incs);
  switch (inScalars->GetDataType())
  {
    vtkTemplateMacro(vtkDiscreteFlyingEdges3DAlgorithm<VTK_TT>::Contour(this, input, inScalars,
      exExt, incs, (VTK_TT*)ptr, output, newPts, newTris, newScalars, newNormals, newGradients));
  }

  vtkDebugMacro(<< "Created: " << newPts->GetNumberOfPoints() << " points, "
                << newTris->GetNumberOfCells() << " triangles");

  // Update ourselves.  Because we don't know up front how many lines
  // we've created, take care to reclaim memory.
  output->SetPoints(newPts);
  newPts->Delete();

  output->SetPolys(newTris);
  newTris->Delete();

  if (newScalars)
  {
    int idx = output->GetPointData()->AddArray(newScalars);
    output->GetPointData()->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    newScalars->Delete();
  }

  if (newNormals)
  {
    int idx = output->GetPointData()->AddArray(newNormals);
    output->GetPointData()->SetActiveAttribute(idx, vtkDataSetAttributes::NORMALS);
    newNormals->Delete();
  }

  if (newGradients)
  {
    int idx = output->GetPointData()->AddArray(newGradients);
    output->GetPointData()->SetActiveAttribute(idx, vtkDataSetAttributes::VECTORS);
    newGradients->Delete();
  }

  vtkImageTransform::TransformPointSet(input, output);

  return 1;
}

//----------------------------------------------------------------------------
int vtkDiscreteFlyingEdges3D::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
void vtkDiscreteFlyingEdges3D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  this->ContourValues->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Compute Normals: " << (this->ComputeNormals ? "On\n" : "Off\n");
  os << indent << "Compute Gradients: " << (this->ComputeGradients ? "On\n" : "Off\n");
  os << indent << "Compute Scalars: " << (this->ComputeScalars ? "On\n" : "Off\n");
  os << indent << "Interpolate Attributes: " << (this->InterpolateAttributes ? "On\n" : "Off\n");
  os << indent << "ArrayComponent: " << this->ArrayComponent << endl;
}
