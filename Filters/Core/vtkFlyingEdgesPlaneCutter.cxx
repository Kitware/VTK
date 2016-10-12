/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFlyingEdgesPlaneCutter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkFlyingEdgesPlaneCutter.h"

#include "vtkArrayListTemplate.h" // For processing attribute data
#include "vtkMath.h"
#include "vtkImageData.h"
#include "vtkCellArray.h"
#include "vtkInformation.h"
#include "vtkInformationIntegerVectorKey.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkFloatArray.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkMarchingCubesTriangleCases.h"
#include "vtkPlane.h"
#include "vtkSMPTools.h"

#include <cmath>

vtkStandardNewMacro(vtkFlyingEdgesPlaneCutter);
vtkCxxSetObjectMacro(vtkFlyingEdgesPlaneCutter,Plane,vtkPlane);

//----------------------------------------------------------------------------
namespace {
// This templated class implements the heart of the algorithm.
// vtkFlyingEdgesPlaneCutter populates the information in this class and
// then invokes Contour() to actually initiate execution.
template <class T>
class vtkFlyingEdgesPlaneCutterAlgorithm
{
public:
  // Edge case table values.
  enum EdgeClass {
    Below = 0, //below isovalue (always isovalue=0.0 for plane)
    Above = 1, //above plane
    LeftAbove = 1, //left vertex is above plane
    RightAbove = 2, //right vertex is above pane
    BothAbove = 3 //entire edge is above plane
  };

  // Dealing with boundary situations when processing volumes.
  enum CellClass {
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
  // indicates which voxel edges intersect with the plane  (i.e., which
  // edges require interpolation). This array is filled in at instantiation
  // during the case table generation process.
  unsigned char EdgeUses[256][12];

  // Flags indicate whether a particular case requires voxel axes to be
  // processed. A cheap acceleration structure computed from the case
  // tables at the point of instantiation.
  unsigned char IncludesAxes[256];

  // Algorithm-derived data. XCases tracks the x-row edge cases. The
  // EdgeMetaData tracks information needed for parallel partitioning,
  // and to enable generation of the output primitives without using
  // a point locator.
  unsigned char *XCases;
  vtkIdType *EdgeMetaData;

  // Internal variables used by the various algorithm methods. Interfaces VTK
  // image data in a form more convenient to the algorithm.
  T        *Scalars;
  vtkIdType Dims[3];
  double    Origin[3];
  double    Spacing[3];
  double    XL, XR;
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
  double *Center; //define plane center
  double *Normal; //define plane normal

  // Output data. Threads write to partitioned memory.
  T         *NewScalars;
  vtkIdType *NewTris;
  float     *NewPoints;
  float     *NewNormals;
  bool       InterpolateAttributes;
  ArrayList  Arrays;

  // Setup algorithm
  vtkFlyingEdgesPlaneCutterAlgorithm();

  // Adjust the origin to the lower-left corner of the volume (if necessary)
  void AdjustOrigin()
  {
    this->Origin[0] = this->Origin[0] + this->Spacing[0]*this->Min0;
    this->Origin[1] = this->Origin[1] + this->Spacing[1]*this->Min1;
    this->Origin[2] = this->Origin[2] + this->Spacing[2]*this->Min2;;
  }

  // The three main passes of the algorithm.
  void ProcessXEdge(double xL[3], double xR[3], vtkIdType row, vtkIdType slice); //PASS 1
  void ProcessYZEdges(vtkIdType row, vtkIdType slice); //PASS 2
  void GenerateOutput(T* inPtr, vtkIdType row, vtkIdType slice);//PASS 4

  // Place holder for now in case fancy bit fiddling is needed later.
  void SetXEdge(unsigned char *ePtr, unsigned char edgeCase)
    {*ePtr = edgeCase;}

  // Given the four x-edge cases defining this voxel, return the voxel case
  // number.
  unsigned char GetEdgeCase(unsigned char *ePtr[4])
  {
    return (*(ePtr[0]) | ((*(ePtr[1]))<<2) | ((*(ePtr[2]))<<4) | ((*(ePtr[3]))<<6));
  }

  // Return the number of contouring primitives for a particular edge case number.
  unsigned char GetNumberOfPrimitives(unsigned char eCase)
    { return this->EdgeCases[eCase][0]; }

  // Return an array indicating which voxel edges intersect the contour.
  unsigned char *GetEdgeUses(unsigned char eCase)
    { return this->EdgeUses[eCase]; }

  // Indicate whether voxel axes need processing for this case.
  unsigned char CaseIncludesAxes(unsigned char eCase)
    { return this->IncludesAxes[eCase]; }

  // Count edge intersections near volume boundaries.
  void CountBoundaryYZInts(unsigned char loc, unsigned char *edgeCases,
                           vtkIdType *eMD[4]);

  // Produce the output triangles for this voxel cell.
  void GenerateTris(unsigned char eCase, unsigned char numTris, vtkIdType *eIds,
                    vtkIdType &triId)
  {
      vtkIdType *tri;
      const unsigned char *edges = this->EdgeCases[eCase] + 1;
      for (int i=0; i < numTris; ++i, edges+=3)
      {
        tri = this->NewTris + 4*triId++;
        tri[0] = 3;
        tri[1] = eIds[edges[0]];
        tri[2] = eIds[edges[1]];
        tri[3] = eIds[edges[2]];
      }
  }

  // Interpolate along a voxel axes edge.
  void InterpolateAxesEdge(double t, vtkIdType vId, const int incs[3],
                           double x0[3], double x1[3],
                           const double s0, const double s1,
                           vtkIdType ijk0[3], vtkIdType ijk1[3])
  {
      float *x = this->NewPoints + 3*vId;
      x[0] = static_cast<float>(x0[0] + t*(x1[0]-x0[0]));
      x[1] = static_cast<float>(x0[1] + t*(x1[1]-x0[1]));
      x[2] = static_cast<float>(x0[2] + t*(x1[2]-x0[2]));

      T *s = this->NewScalars + vId;
      *s = s0 + t * (s1 - s0);

      if ( this->NewNormals )
      {
        float *n = this->NewNormals + 3*vId;
        n[0] = -this->Normal[0];
        n[1] = -this->Normal[1];
        n[2] = -this->Normal[2];
      }//if normals

      if ( this->InterpolateAttributes )
      {
        vtkIdType v0=ijk0[0] + ijk0[1]*incs[1] + ijk0[2]*incs[2];
        vtkIdType v1=ijk1[0] + ijk1[1]*incs[1] + ijk1[2]*incs[2];;
        this->Arrays.InterpolateEdge(v0,v1,t,vId);
      }
  }

  // Interpolate along an arbitrary edge, typically one that may be on the
  // volume boundary. This means careful computation of stuff requiring
  // neighborhood information.
  void InterpolateEdge(vtkIdType ijk[3], T const * const s,
                       const int incs[3],double x[3],
                       unsigned char edgeNum,
                       unsigned char const* const edgeUses,
                       vtkIdType *eIds);

  // Produce the output points on the voxel axes for this voxel cell.
  void GeneratePoints(unsigned char loc, vtkIdType ijk[3],
                      const T *sPtr, const int incs[3],
                      double x[3], const double sV,
                      unsigned char const * const edgeUses, vtkIdType *eIds);

  // Helper function to set up the point ids on voxel edges.
  unsigned char InitVoxelIds(unsigned char *ePtr[4], vtkIdType *eMD[4],
                             vtkIdType *eIds)
  {
      unsigned char eCase = GetEdgeCase(ePtr);
      eIds[0] = eMD[0][0]; //x-edges
      eIds[1] = eMD[1][0];
      eIds[2] = eMD[2][0];
      eIds[3] = eMD[3][0];
      eIds[4] = eMD[0][1]; //y-edges
      eIds[5] = eIds[4] + this->EdgeUses[eCase][4];
      eIds[6] = eMD[2][1];
      eIds[7] = eIds[6] + this->EdgeUses[eCase][6];
      eIds[8] = eMD[0][2]; //z-edges
      eIds[9] = eIds[8] + this->EdgeUses[eCase][8];
      eIds[10] = eMD[1][2];
      eIds[11] = eIds[10] + this->EdgeUses[eCase][10];
      return eCase;
  }

  // Helper function to advance the point ids along voxel rows.
  void AdvanceVoxelIds(unsigned char eCase, vtkIdType *eIds)
  {
      eIds[0] += this->EdgeUses[eCase][0]; //x-edges
      eIds[1] += this->EdgeUses[eCase][1];
      eIds[2] += this->EdgeUses[eCase][2];
      eIds[3] += this->EdgeUses[eCase][3];
      eIds[4] += this->EdgeUses[eCase][4]; //y-edges
      eIds[5] = eIds[4] + this->EdgeUses[eCase][5];
      eIds[6] += this->EdgeUses[eCase][6];
      eIds[7] = eIds[6] + this->EdgeUses[eCase][7];
      eIds[8] += this->EdgeUses[eCase][8]; //z-edges
      eIds[9] = eIds[8] + this->EdgeUses[eCase][9];
      eIds[10] += this->EdgeUses[eCase][10];
      eIds[11] = eIds[10] + this->EdgeUses[eCase][11];
  }

  // Threading integration via SMPTools
  template <class TT> class Pass1
  {
    public:
      vtkFlyingEdgesPlaneCutterAlgorithm<TT> *Algo;
      Pass1(vtkFlyingEdgesPlaneCutterAlgorithm<TT> *algo)
        {this->Algo = algo;}
      void  operator()(vtkIdType slice, vtkIdType end)
      {
        vtkIdType row;
        TT *rowPtr, *slicePtr = this->Algo->Scalars + slice*this->Algo->Inc2;
        double xL[3], xR[3];
        xL[0] = this->Algo->XL;
        xR[0] = this->Algo->XR;
        for ( ; slice < end; ++slice )
        {
          xL[2] = this->Algo->Origin[2] + slice*this->Algo->Spacing[2];
          xR[2] = xL[2];
          for (row=0, rowPtr=slicePtr; row < this->Algo->Dims[1]; ++row)
          {
            xL[1] = this->Algo->Origin[1] + row*this->Algo->Spacing[1];
            xR[1] = xL[1];
            this->Algo->ProcessXEdge(xL, xR, row, slice);
            rowPtr += this->Algo->Inc1;
          }//for all rows in this slice
          slicePtr += this->Algo->Inc2;
        }//for all slices in this batch
      }
  };
  template <class TT> class Pass2
  {
    public:
      Pass2(vtkFlyingEdgesPlaneCutterAlgorithm<TT> *algo)
        {this->Algo = algo;}
      vtkFlyingEdgesPlaneCutterAlgorithm<TT> *Algo;
      void  operator()(vtkIdType slice, vtkIdType end)
      {
        for ( ; slice < end; ++slice)
        {
          for ( vtkIdType row=0; row < (this->Algo->Dims[1]-1); ++row)
          {
            this->Algo->ProcessYZEdges(row, slice);
          }//for all rows in this slice
        }//for all slices in this batch
      }
  };
  template <class TT> class Pass4
  {
    public:
      Pass4(vtkFlyingEdgesPlaneCutterAlgorithm<TT> *algo)
        {this->Algo = algo;}
      vtkFlyingEdgesPlaneCutterAlgorithm<TT> *Algo;
      void  operator()(vtkIdType slice, vtkIdType end)
      {
        vtkIdType row;
        vtkIdType *eMD0 = this->Algo->EdgeMetaData + slice*6*this->Algo->Dims[1];
        vtkIdType *eMD1 = eMD0 + 6*this->Algo->Dims[1];
        TT *rowPtr, *slicePtr = this->Algo->Scalars + slice*this->Algo->Inc2;
        for ( ; slice < end; ++slice )
        {
          // It's possible to skip entire slices if there is nothing to generate
          if ( eMD1[3] > eMD0[3] ) //there are triangle primitives!
          {
            for (row=0, rowPtr=slicePtr; row < this->Algo->Dims[1]-1; ++row)
            {
              this->Algo->GenerateOutput(rowPtr, row, slice);
              rowPtr += this->Algo->Inc1;
            }//for all rows in this slice
          }//if there are triangles
          slicePtr += this->Algo->Inc2;
          eMD0 = eMD1;
          eMD1 = eMD0 + 6*this->Algo->Dims[1];
        }//for all slices in this batch
      }
  };

  // Interface between VTK and templated functions
  static void Contour(vtkFlyingEdgesPlaneCutter *self, vtkImageData *input,
                      vtkDataArray *inScalars, int extent[6], vtkIdType *incs,
                      T *scalars, vtkPolyData *output, vtkPoints *newPts,
                      vtkCellArray *newTris, vtkDataArray *newScalars,
                      vtkDataArray *newNormals);
};

//----------------------------------------------------------------------------
// Map MC edges numbering to use the saner FlyingEdges edge numbering scheme.
template <class T> const unsigned char vtkFlyingEdgesPlaneCutterAlgorithm<T>::
EdgeMap[12] = {0,5,1,4,2,7,3,6,8,9,10,11};

//----------------------------------------------------------------------------
// Map MC edges numbering to use the saner FlyingEdges edge numbering scheme.
template <class T> const unsigned char vtkFlyingEdgesPlaneCutterAlgorithm<T>::
VertMap[12][2] = {{0,1}, {2,3}, {4,5}, {6,7}, {0,2}, {1,3}, {4,6}, {5,7},
                  {0,4}, {1,5}, {2,6}, {3,7}};

//----------------------------------------------------------------------------
// The offsets of each vertex (in index space) from the voxel axes origin.
template <class T> const unsigned char vtkFlyingEdgesPlaneCutterAlgorithm<T>::
VertOffsets[8][3] = {{0,0,0}, {1,0,0}, {0,1,0}, {1,1,0},
                     {0,0,1}, {1,0,1}, {0,1,1}, {1,1,1}};

//----------------------------------------------------------------------------
// Instantiate and initialize key data members. Mostly we build the
// edge-based case table, and associated acceleration structures, from the
// marching cubes case table. Some of this code is borrowed shamelessly from
// vtkVoxel::Contour() method.
template <class T> vtkFlyingEdgesPlaneCutterAlgorithm<T>::
vtkFlyingEdgesPlaneCutterAlgorithm():XCases(NULL),EdgeMetaData(NULL),NewScalars(NULL),
                                     NewTris(NULL),NewPoints(NULL),NewNormals(NULL)
{
  int i, j, k, l, ii, eCase, index, numTris;
  static int vertMap[8] = {0,1,3,2,4,5,7,6};
  static int CASE_MASK[8] = {1,2,4,8,16,32,64,128};
  EDGE_LIST *edge;
  vtkMarchingCubesTriangleCases *triCase;
  unsigned char *edgeCase;

  // Initialize cases, increments, and edge intersection flags
  for (eCase=0; eCase<256; ++eCase)
  {
    for (j=0; j<16; ++j)
    {
      this->EdgeCases[eCase][j] = 0;
    }
    for (j=0; j<12; ++j)
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
  for (l=0; l<4; ++l)
  {
    for (k=0; k<4; ++k)
    {
      for (j=0; j<4; ++j)
      {
        for (i=0; i<4; ++i)
        {
          //yes we could just count to (0->255) but where's the fun in that?
          eCase = i | (j<<2) | (k<<4) | (l<<6);
          for ( ii=0, index = 0; ii < 8; ++ii)
          {
            if ( eCase & (1<<vertMap[ii]) ) //map into ancient MC table
            {
              index |= CASE_MASK[ii];
            }
          }
          //Now build case table
          triCase = vtkMarchingCubesTriangleCases::GetCases() + index;
          edge = triCase->edges;
          for ( numTris=0, edge=triCase->edges; edge[0] > -1; edge += 3 )
          {//count the number of triangles
            numTris++;
          }
          if ( numTris > 0 )
          {
            edgeCase = this->EdgeCases[eCase];
            *edgeCase++ = numTris;
            for ( edge = triCase->edges; edge[0] > -1; edge += 3, edgeCase+=3 )
            {
              // Build new case table.
              edgeCase[0] = this->EdgeMap[edge[0]];
              edgeCase[1] = this->EdgeMap[edge[1]];
              edgeCase[2] = this->EdgeMap[edge[2]];
            }
          }
        }//x-edges
      }//x+y-edges
    }//x+z-edges
  }//x+y+z-edges

  // Okay now build the acceleration structure. This is used to generate
  // output points and triangles when processing a voxel x-row as well as to
  // perform other topological reasoning. This structure is a function of the
  // particular case number.
  for (eCase=0; eCase < 256; ++eCase)
  {
    edgeCase = this->EdgeCases[eCase];
    numTris = *edgeCase++;

    // Mark edges that are used by this case.
    for (i=0; i < numTris*3; ++i) //just loop over all edges
    {
      this->EdgeUses[eCase][edgeCase[i]] = 1;
    }

    this->IncludesAxes[eCase] = this->EdgeUses[eCase][0] |
      this->EdgeUses[eCase][4] | this->EdgeUses[eCase][8];

  }//for all cases
}

//----------------------------------------------------------------------------
// Count intersections along voxel axes. When traversing the volume across
// x-edges, the voxel axes on the boundary may be undefined near boundaries
// (because there are no fully-formed cells). Thus the voxel axes on the
// boundary are treated specially.
template <class T> void vtkFlyingEdgesPlaneCutterAlgorithm<T>::
CountBoundaryYZInts(unsigned char loc, unsigned char *edgeUses,
                    vtkIdType *eMD[4])
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
    case 10://+x +y
      eMD[0][1] += edgeUses[5];
      eMD[0][2] += edgeUses[9];
      eMD[1][2] += edgeUses[10];
      eMD[1][2] += edgeUses[11];
      break;
    case 32://+z
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
    default: //uh-oh shouldn't happen
      break;
  }
}

//----------------------------------------------------------------------------
// Interpolate a new point along a boundary edge. Make sure to consider
// proximity to the boundary.
template <class T> void vtkFlyingEdgesPlaneCutterAlgorithm<T>::
InterpolateEdge(vtkIdType ijk[3], T const * const s, const int incs[3],
                double x[3], unsigned char edgeNum,
                unsigned char const * const edgeUses, vtkIdType *eIds)
{
  // if this edge is not used then get out
  if ( ! edgeUses[edgeNum] )
  {
    return;
  }

  // Build the edge information. Note that the information we have is the
  // position of the lower-left corner of the voxel; the offsets tell us how
  // to generate the interpolating edge.
  const unsigned char *vertMap = this->VertMap[edgeNum];

  double x0[3], x1[3];
  vtkIdType ijk0[3], ijk1[3], vId=eIds[edgeNum];

  const unsigned char *offsets0 = this->VertOffsets[vertMap[0]];
  const T *s0 = s + offsets0[0]*incs[0] +
                    offsets0[1]*incs[1] +
                    offsets0[2]*incs[2];
  x0[0] = x[0] + offsets0[0]*this->Spacing[0];
  x0[1] = x[1] + offsets0[1]*this->Spacing[1];
  x0[2] = x[2] + offsets0[2]*this->Spacing[2];

  const unsigned char *offsets1 = this->VertOffsets[vertMap[1]];
  const T *s1 = s + offsets1[0]*incs[0] +
                    offsets1[1]*incs[1] +
                    offsets1[2]*incs[2];
  x1[0] = x[0] + offsets1[0]*this->Spacing[0];
  x1[1] = x[1] + offsets1[1]*this->Spacing[1];
  x1[2] = x[2] + offsets1[2]*this->Spacing[2];

  double sV0 = vtkPlane::Evaluate(this->Normal,this->Center,x0);
  double sV1 = vtkPlane::Evaluate(this->Normal,this->Center,x1);

  // Okay interpolate. Remember the plane value = 0.0.
  double t = -(sV0) / (sV1 - sV0);
  float *xPtr = this->NewPoints + 3*vId;
  xPtr[0] = static_cast<float>(x0[0] + t*(x1[0]-x0[0]));
  xPtr[1] = static_cast<float>(x0[1] + t*(x1[1]-x0[1]));
  xPtr[2] = static_cast<float>(x0[2] + t*(x1[2]-x0[2]));

  T *sInt = this->NewScalars + vId;
  *sInt = *s0 + t*(*s1-*s0);

  if ( this->NewNormals )
  {
    float *n = this->NewNormals + 3*vId;
    n[0] = -this->Normal[0];
    n[1] = -this->Normal[1];
    n[2] = -this->Normal[2];
  }//if normals

  if ( this->InterpolateAttributes )
  {
    ijk0[0] = ijk[0] + offsets0[0];
    ijk0[1] = ijk[1] + offsets0[1];
    ijk0[2] = ijk[2] + offsets0[2];

    ijk1[0] = ijk[0] + offsets1[0];
    ijk1[1] = ijk[1] + offsets1[1];
    ijk1[2] = ijk[2] + offsets1[2];

    vtkIdType v0=ijk0[0] + ijk0[1]*incs[1] + ijk0[2]*incs[2];
    vtkIdType v1=ijk1[0] + ijk1[1]*incs[1] + ijk1[2]*incs[2];
    this->Arrays.InterpolateEdge(v0,v1,t,vId);
  }
}

//----------------------------------------------------------------------------
// Generate the output points and optionally normals and attributes.
template <class T> void vtkFlyingEdgesPlaneCutterAlgorithm<T>::
GeneratePoints(unsigned char loc, vtkIdType ijk[3],
               T const * const sPtr, const int incs[3],
               double x[3], const double sV0,
               unsigned char const * const edgeUses, vtkIdType *eIds)
{
  // Interpolate the three x-y-z cell axes edges.
  double sV1;
  for(int i=0; i < 3; ++i)
  {
    if(edgeUses[i*4])
    {
      //edgesUses[0] == x axes edge
      //edgesUses[4] == y axes edge
      //edgesUses[8] == z axes edge
      double x1[3] = {x[0], x[1], x[2] }; x1[i] += this->Spacing[i];
      vtkIdType ijk1[3] = { ijk[0], ijk[1], ijk[2] }; ++ijk1[i];

      T const * const sPtr1 = (sPtr+incs[i]);
      sV1 = vtkPlane::Evaluate(this->Normal,this->Center,x1);
      double t = -sV0 / (sV1 - sV0);
      this->InterpolateAxesEdge(t, eIds[i*4], incs, x, x1, *sPtr, *sPtr1, ijk, ijk1);
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
    case 2: case 6: case 18: case 22: //+x
      this->InterpolateEdge(ijk, sPtr, incs, x, 5, edgeUses, eIds);
      this->InterpolateEdge(ijk, sPtr, incs, x, 9, edgeUses, eIds);
      break;
    case 8: case 9: case 24: case 25: //+y
      this->InterpolateEdge(ijk, sPtr, incs, x, 1, edgeUses, eIds);
      this->InterpolateEdge(ijk, sPtr, incs, x, 10, edgeUses, eIds);
      break;
    case 32: case 33: case 36: case 37: //+z
      this->InterpolateEdge(ijk, sPtr, incs, x, 2, edgeUses, eIds);
      this->InterpolateEdge(ijk, sPtr, incs, x, 6, edgeUses, eIds);
      break;
    case 10: case 26: //+x +y
      this->InterpolateEdge(ijk, sPtr, incs, x, 1, edgeUses, eIds);
      this->InterpolateEdge(ijk, sPtr, incs, x, 5, edgeUses, eIds);
      this->InterpolateEdge(ijk, sPtr, incs, x, 9, edgeUses, eIds);
      this->InterpolateEdge(ijk, sPtr, incs, x, 10, edgeUses, eIds);
      this->InterpolateEdge(ijk, sPtr, incs, x, 11, edgeUses, eIds);
      break;
    case 34: case 38: //+x +z
      this->InterpolateEdge(ijk, sPtr, incs, x, 2, edgeUses, eIds);
      this->InterpolateEdge(ijk, sPtr, incs, x, 5, edgeUses, eIds);
      this->InterpolateEdge(ijk, sPtr, incs, x, 9, edgeUses, eIds);
      this->InterpolateEdge(ijk, sPtr, incs, x, 6, edgeUses, eIds);
      this->InterpolateEdge(ijk, sPtr, incs, x, 7, edgeUses, eIds);
      break;
    case 40: case 41: //+y +z
      this->InterpolateEdge(ijk, sPtr, incs, x, 1, edgeUses, eIds);
      this->InterpolateEdge(ijk, sPtr, incs, x, 2, edgeUses, eIds);
      this->InterpolateEdge(ijk, sPtr, incs, x, 3, edgeUses, eIds);
      this->InterpolateEdge(ijk, sPtr, incs, x, 6, edgeUses, eIds);
      this->InterpolateEdge(ijk, sPtr, incs, x, 10, edgeUses, eIds);
      break;
    case 42: //+x +y +z happens no more than once per volume
      this->InterpolateEdge(ijk, sPtr, incs, x, 1, edgeUses, eIds);
      this->InterpolateEdge(ijk, sPtr, incs, x, 2, edgeUses, eIds);
      this->InterpolateEdge(ijk, sPtr, incs, x, 3, edgeUses, eIds);
      this->InterpolateEdge(ijk, sPtr, incs, x, 5, edgeUses, eIds);
      this->InterpolateEdge(ijk, sPtr, incs, x, 9, edgeUses, eIds);
      this->InterpolateEdge(ijk, sPtr, incs, x, 10, edgeUses, eIds);
      this->InterpolateEdge(ijk, sPtr, incs, x, 11, edgeUses, eIds);
      this->InterpolateEdge(ijk, sPtr, incs, x, 6, edgeUses, eIds);
      this->InterpolateEdge(ijk, sPtr, incs, x, 7, edgeUses, eIds);
      break;
    default: //interior, or -x,-y,-z boundaries
      return;
  }
}

//----------------------------------------------------------------------------
// PASS 1: Process a single volume x-row (and all of the voxel edges that
// compose the row). Determine the x-edges case classification, count the
// number of x-edge intersections, and figure out where intersections along
// the x-row begins and ends (i.e., gather information for computational
// trimming).
//
// This method varies significantly from the first pass of vtkFlyingEdges3D
// because the evaluation of the x-edges is performed by intersection against
// the cutting plane.
template <class T> void vtkFlyingEdgesPlaneCutterAlgorithm<T>::
ProcessXEdge(double xL[3], double xR[3], vtkIdType row, vtkIdType slice)
{
  vtkIdType nxcells=this->Dims[0]-1;
  vtkIdType minInt=nxcells, maxInt = 0;
  vtkIdType *edgeMetaData;
  unsigned char *ePtr = this->XCases + slice*this->SliceOffset + row*nxcells;

  // Grab this volume edge's metadata
  edgeMetaData = this->EdgeMetaData + (slice*this->Dims[1] + row)*6;

  // Evaluate the end points of this volume edge
  vtkIdType numInts;
  double a, b;
  double vL = vtkPlane::Evaluate(this->Normal,this->Center,xL);
  double vR = vtkPlane::Evaluate(this->Normal,this->Center,xR);
  if ( vL >= 0.0 && vR >= 0.0)
  {//also catches the case where vL=vR==0.0
    numInts = 0;
    std::fill_n(ePtr, nxcells, vtkFlyingEdgesPlaneCutterAlgorithm::BothAbove);
    minInt = nxcells;
    maxInt = 0;
  }
  else if ( vL < 0.0 && vR < 0.0)
  {
    numInts = 0;
    std::fill_n(ePtr, nxcells, vtkFlyingEdgesPlaneCutterAlgorithm::Below);
    minInt = nxcells;
    maxInt = 0;
  }
  else
  {//volume edge intersects plane
    numInts = 1;
    a = fabs(vL);
    b = fabs(vR);
    minInt = static_cast<vtkIdType>((a*nxcells)/(a+b));
    minInt = minInt >= nxcells ? nxcells-1 : minInt;
    maxInt = minInt + 1;
    if ( vL < 0.0 ) //&& vR >= 0.0
    {
      std::fill_n(ePtr,minInt,vtkFlyingEdgesPlaneCutterAlgorithm::Below);
      this->SetXEdge(ePtr+minInt,vtkFlyingEdgesPlaneCutterAlgorithm::RightAbove);
      std::fill_n(ePtr+maxInt,nxcells-maxInt,vtkFlyingEdgesPlaneCutterAlgorithm::BothAbove);
    }
    else //vL >= 0.0 && vR < 0.0
    {
      std::fill_n(ePtr,minInt,vtkFlyingEdgesPlaneCutterAlgorithm::BothAbove);
      this->SetXEdge(ePtr+minInt,vtkFlyingEdgesPlaneCutterAlgorithm::LeftAbove);
      std::fill_n(ePtr+maxInt,nxcells-maxInt,vtkFlyingEdgesPlaneCutterAlgorithm::Below);
    }
  }//volume edge intersects plane

  // The beginning and ending of intersections along the edge is used for
  // computational trimming.
  edgeMetaData[0] = numInts; //number x-cell axes intersections
  edgeMetaData[1] = 0; //number y-cell axes intersections
  edgeMetaData[2] = 0; //number z-cell axes intersections
  edgeMetaData[3] = 0; //number triangles to be generated
  edgeMetaData[4] = minInt; //where intersections start along x edge
  edgeMetaData[5] = maxInt; //where intersections end along x edge
}

//----------------------------------------------------------------------------
// PASS 2: Process a single x-row of voxels. Count the number of y- and
// z-intersections by topological reasoning from x-edge cases. Determine the
// number of primitives (i.e., triangles) generated from this row. Use
// computational trimming to reduce work. Note *ePtr[4] is four pointers to
// four x-edge rows that bound the voxel x-row and which contain edge case
// information.
template <class T> void vtkFlyingEdgesPlaneCutterAlgorithm<T>::
ProcessYZEdges(vtkIdType row, vtkIdType slice)
{
  // Grab the four edge cases bounding this voxel x-row.
  unsigned char *ePtr[4], ec0, ec1, ec2, ec3, xInts=1;
  ePtr[0] = this->XCases + slice*this->SliceOffset + row*(this->Dims[0]-1);
  ePtr[1] = ePtr[0] + this->Dims[0]-1;
  ePtr[2] = ePtr[0] + this->SliceOffset;
  ePtr[3] = ePtr[2] + this->Dims[0]-1;

  // Grab the edge meta data surrounding the voxel row.
  vtkIdType *eMD[4];
  eMD[0] = this->EdgeMetaData + (slice*this->Dims[1] + row)*6; //this x-edge
  eMD[1] = eMD[0] + 6; //x-edge in +y direction
  eMD[2] = eMD[0] + this->Dims[1]*6; //x-edge in +z direction
  eMD[3] = eMD[2] + 6; //x-edge in +y+z direction

  // Determine whether this row of x-cells needs processing. If there are no
  // x-edge intersections, and the classification of the four bounding
  // x-edges is the same, then there is no need for processing.
  if ( (eMD[0][0] | eMD[1][0] | eMD[2][0] | eMD[3][0]) == 0 ) //any x-ints?
  {
    if ( *(ePtr[0]) == *(ePtr[1]) &&  *(ePtr[1]) == *(ePtr[2]) &&
         *(ePtr[2]) == *(ePtr[3]) )
    {
      return; //there are no x-, y- or z-ints, thus no contour, skip voxel row
    }
    else
    {
      xInts = 0; //there are y- or z- edge ints however
    }
  }

  // Determine proximity to the boundary of volume. This information is used
  // to count edge intersections in boundary situations.
  unsigned char loc, yLoc, zLoc, yzLoc;
  yLoc = (row >= (this->Dims[1]-2) ? MaxBoundary : Interior);
  zLoc = (slice >= (this->Dims[2]-2) ? MaxBoundary : Interior);
  yzLoc = (yLoc << 2) | (zLoc << 4);

  // The trim edges may need adjustment if the plane travels between rows
  // of x-edges (without intersecting these x-edges). This means checking
  // whether the trim faces at (xL,xR) made up of the y-z edges intersect the
  // contour. Basically just an intersection operation. Determine the voxel
  // row trim edges, need to check all four x-edges.
  vtkIdType xL=eMD[0][4], xR=eMD[0][5];
  vtkIdType i;
  if ( xInts )
  {
    for (i=1; i < 4; ++i)
    {
      xL = ( eMD[i][4] < xL ? eMD[i][4] : xL);
      xR = ( eMD[i][5] > xR ? eMD[i][5] : xR);
    }

    if ( xL > 0 ) //if trimmed in the -x direction
    {
      ec0 = *(ePtr[0]+xL); ec1 = *(ePtr[1]+xL);
      ec2 = *(ePtr[2]+xL); ec3 = *(ePtr[3]+xL);
      if ( (ec0 & 0x1) != (ec1 & 0x1) || (ec1 & 0x1) != (ec2 & 0x1) ||
           (ec2 & 0x1) != (ec3 & 0x1) )
      {
        xL = eMD[0][4] = 0; //reset left trim
      }
    }

    if ( xR < (this->Dims[0]-1) ) //if trimmed in the +x direction
    {
      ec0 = *(ePtr[0]+xR); ec1 = *(ePtr[1]+xR);
      ec2 = *(ePtr[2]+xR); ec3 = *(ePtr[3]+xR);
      if ( (ec0 & 0x2) != (ec1 & 0x2) || (ec1 & 0x2) != (ec2 & 0x2) ||
           (ec2 & 0x2) != (ec3 & 0x2) )
      {
        xR = eMD[0][5] = this->Dims[0]-1; //reset right trim
      }
    }
  }
  else //plane cuts through without intersecting x-edges, reset trim edges
  {
    xL = eMD[0][4] = 0;
    xR = eMD[0][5] = this->Dims[0]-1;
  }

  // Okay run along the x-voxels and count the number of y- and
  // z-intersections. Here we are just checking y,z edges that make up the
  // voxel axes. Also check the number of primitives generated.
  unsigned char *edgeUses, eCase, numTris;
  ePtr[0] += xL; ePtr[1] += xL; ePtr[2] += xL; ePtr[3] += xL;
  const vtkIdType dim0Wall = this->Dims[0]-2;
  for (i=xL; i < xR; ++i) //run along the trimmed x-voxels
  {
    eCase = this->GetEdgeCase(ePtr);
    if ( (numTris=this->GetNumberOfPrimitives(eCase)) > 0 )
    {
      // Okay let's increment the triangle count.
      eMD[0][3] += numTris;

      // Count the number of y- and z-points to be generated. Pass# 1 counted
      // the number of x-intersections along the x-edges. Now we count all
      // intersections on the y- and z-voxel axes.
      edgeUses = this->GetEdgeUses(eCase);
      eMD[0][1] += edgeUses[4]; //y-voxel axes edge always counted
      eMD[0][2] += edgeUses[8]; //z-voxel axes edge always counted
      loc = yzLoc | (i >= dim0Wall ? MaxBoundary : Interior);
      if ( loc != 0 )
      {
        this->CountBoundaryYZInts(loc,edgeUses,eMD);
      }
    }//if cell contains contour

    // advance the four pointers along voxel row
    ePtr[0]++; ePtr[1]++; ePtr[2]++; ePtr[3]++;
  }//for all voxels along this x-edge
}

//----------------------------------------------------------------------------
// PASS 4: Process the x-row cells to generate output primitives, including
// point coordinates and triangles. This is the fourth and final pass of the
// algorithm.
template <class T> void vtkFlyingEdgesPlaneCutterAlgorithm<T>::
GenerateOutput(T* rowPtr, vtkIdType row, vtkIdType slice)
{
  // Grab the edge meta data surrounding the voxel row.
  vtkIdType *eMD[4];
  eMD[0] = this->EdgeMetaData + (slice*this->Dims[1] + row)*6; //this x-edge
  eMD[1] = eMD[0] + 6; //x-edge in +y direction
  eMD[2] = eMD[0] + this->Dims[1]*6; //x-edge in +z direction
  eMD[3] = eMD[2] + 6; //x-edge in +y+z direction

  // Return if there is nothing to do (i.e., no triangles to generate)
  if ( eMD[0][3] == eMD[1][3] )
  {
    return;
  }

  // Get the voxel row trim edges and prepare to generate. Find the voxel row
  // trim edges, need to check all four x-edges to compute row trim edge.
  vtkIdType xL=eMD[0][4], xR=eMD[0][5];
  vtkIdType i;
  for (i=1; i < 4; ++i)
  {
    xL = ( eMD[i][4] < xL ? eMD[i][4] : xL);
    xR = ( eMD[i][5] > xR ? eMD[i][5] : xR);
  }

  // Grab the four edge cases bounding this voxel x-row. Begin at left trim edge.
  unsigned char *ePtr[4];
  ePtr[0] = this->XCases + slice*this->SliceOffset + row*(this->Dims[0]-1) + xL;
  ePtr[1] = ePtr[0] + this->Dims[0]-1;
  ePtr[2] = ePtr[0] + this->SliceOffset;
  ePtr[3] = ePtr[2] + this->Dims[0]-1;

  // Traverse all voxels in this row, those containing the contour are
  // further identified for processing, meaning generating points and
  // triangles. Begin by setting up point ids on voxel edges.
  vtkIdType triId = eMD[0][3];
  vtkIdType eIds[12]; //the ids of generated points

  unsigned char eCase = this->InitVoxelIds(ePtr,eMD,eIds);

  // Determine the proximity to the boundary of volume. This information is
  // used to generate edge intersections.
  unsigned char loc, yLoc, zLoc, yzLoc;
  yLoc = (row < 1 ? MinBoundary :
          (row >= (this->Dims[1]-2) ? MaxBoundary : Interior));
  zLoc = (slice < 1 ? MinBoundary :
          (slice >= (this->Dims[2]-2) ? MaxBoundary : Interior));
  yzLoc = (yLoc << 2) | (zLoc << 4);

  // Run along voxels in x-row direction and generate output primitives. Note
  // that active voxel axes edges are interpolated to produce points and
  // possibly interpolate attribute data.
  double x[3], sV;
  x[0] = this->Origin[0] + xL*this->Spacing[0];
  x[1] = this->Origin[1] + row*this->Spacing[1];
  x[2] = this->Origin[2] + slice*this->Spacing[2];

  //compute the ijk for this section
  vtkIdType ijk[3] = { xL, row, slice};

  //load the inc0/inc1/inc2 into local memory
  const int incs[3] = { this->Inc0, this->Inc1, this->Inc2 };
  const T* sPtr = rowPtr + xL*incs[0];
  const double xSpace = this->Spacing[0];
  const vtkIdType dim0Wall = this->Dims[0]-2;

  for (i=xL; i < xR; ++i)
  {
    const unsigned char numTris = this->GetNumberOfPrimitives(eCase);
    if ( numTris > 0 )
    {
      // Start by generating triangles for this case
      this->GenerateTris(eCase,numTris,eIds,triId);

      // Now generate point(s) along voxel axes if needed. Remember to take
      // boundary into account.
      loc = yzLoc | (i < 1 ? MinBoundary :
                     (i >= dim0Wall ? MaxBoundary : Interior));
      if ( this->CaseIncludesAxes(eCase) || loc != Interior )
      {
        unsigned char const * const edgeUses = this->GetEdgeUses(eCase);
        sV = vtkPlane::Evaluate(this->Normal,this->Center,x);
        this->GeneratePoints(loc, ijk, sPtr, incs, x, sV, edgeUses, eIds);
      }
      this->AdvanceVoxelIds(eCase,eIds);
    }

    // advance along voxel row
    ePtr[0]++; ePtr[1]++; ePtr[2]++; ePtr[3]++;
    eCase = this->GetEdgeCase(ePtr);

    ++ijk[0];
    sPtr += incs[0];
    x[0] += xSpace;
  } //for all non-trimmed cells along this x-edge
}

//----------------------------------------------------------------------------
// Contouring filter specialized for 3D volumes. This templated function
// interfaces the vtkFlyingEdgesPlaneCutter class with the templated algorithm
// class. It also invokes the three passes of the Flying Edges algorithm.
template <class T> void vtkFlyingEdgesPlaneCutterAlgorithm<T>::
Contour(vtkFlyingEdgesPlaneCutter *self, vtkImageData *input, vtkDataArray *inScalars,
        int extent[6], vtkIdType *incs, T *scalars, vtkPolyData *output, vtkPoints *newPts,
        vtkCellArray *newTris, vtkDataArray *newScalars, vtkDataArray *newNormals)
{
  vtkIdType row, slice, *eMD, zInc;
  vtkIdType numOutXPts, numOutYPts, numOutZPts, numOutTris;
  vtkIdType numXPts=0, numYPts=0, numZPts=0, numTris=0;
  vtkIdType startXPts, startYPts, startZPts, startTris;
  startXPts = startYPts = startZPts = startTris = 0;

  // This may be subvolume of the total 3D image. Capture information for
  // subsequent processing.
  vtkFlyingEdgesPlaneCutterAlgorithm<T> algo;
  algo.Scalars = scalars;
  input->GetOrigin(algo.Origin);
  input->GetSpacing(algo.Spacing);
  algo.Min0 = extent[0];
  algo.Max0 = extent[1];
  algo.Inc0 = incs[0];
  algo.Min1 = extent[2];
  algo.Max1 = extent[3];
  algo.Inc1 = incs[1];
  algo.Min2 = extent[4];
  algo.Max2 = extent[5];
  algo.Inc2 = incs[2];
  algo.AdjustOrigin();

  // The left and right bounds of the x-volume edges
  algo.XL = algo.Origin[0];
  algo.XR = algo.Origin[0] + (algo.Max0 - algo.Min0)*algo.Spacing[0];

  // Copy down the plane definition
  algo.Center = self->GetPlane()->GetOrigin();
  algo.Normal = self->GetPlane()->GetNormal();

  // Now allocate working arrays. The XCases array tracks x-edge cases.
  algo.Dims[0] = algo.Max0 - algo.Min0 + 1;
  algo.Dims[1] = algo.Max1 - algo.Min1 + 1;
  algo.Dims[2] = algo.Max2 - algo.Min2 + 1;
  algo.NumberOfEdges = algo.Dims[1]*algo.Dims[2];
  algo.SliceOffset = (algo.Dims[0]-1) * algo.Dims[1];
  algo.XCases = new unsigned char [(algo.Dims[0]-1)*algo.NumberOfEdges];

  // Also allocate the characterization (metadata) array for the x edges.
  // This array tracks the number of x-, y- and z- intersections on the voxel
  // axes along an x-edge; as well as the number of the output triangles, and
  // the xMin_i and xMax_i (minimum index of first intersection, maximum
  // index of intersection for the ith x-row, the so-called trim edges used
  // for computational trimming).
  algo.EdgeMetaData = new vtkIdType [algo.NumberOfEdges*6];

  // Configure the interpolation of attribute data. Scalars are required and
  // always interpolated, other attributes are optional.
  algo.InterpolateAttributes = (self->GetInterpolateAttributes() &&
                                input->GetPointData()->GetNumberOfArrays() > 1) ? true : false;

  // PASS 1: Traverse all x-rows building edge cases and counting number of
  // intersections (i.e., accumulate information necessary for later output
  // memory allocation, e.g., the number of output points along the x-rows
  // are counted).
  Pass1<T> pass1(&algo);
  vtkSMPTools::For(0,algo.Dims[2], pass1);

  // PASS 2: Traverse all voxel x-rows and process voxel y&z edges.  The
  // result is a count of the number of y- and z-intersections, as well as
  // the number of triangles generated along these voxel rows.
  Pass2<T> pass2(&algo);
  vtkSMPTools::For(0,algo.Dims[2]-1, pass2);

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
  for (slice=0; slice < algo.Dims[2]; ++slice)
  {
    zInc = slice * algo.Dims[1];
    for (row=0; row < algo.Dims[1]; ++row)
    {
      eMD = algo.EdgeMetaData + (zInc+row)*6;
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
  if ( totalPts > 0 )
  {
    newPts->GetData()->WriteVoidPointer(0,3*totalPts);
    algo.NewPoints = static_cast<float*>(newPts->GetVoidPointer(0));
    newTris->WritePointer(numOutTris,4*numOutTris);
    algo.NewTris = static_cast<vtkIdType*>(newTris->GetPointer());

    if (newScalars)
    {
      newScalars->WriteVoidPointer(0,totalPts);
      algo.NewScalars = static_cast<T*>(newScalars->GetVoidPointer(0));
    }

    if (newNormals)
    {
      newNormals->WriteVoidPointer(0,3*totalPts);
      algo.NewNormals = static_cast<float*>(newNormals->GetVoidPointer(0));
    }

    if ( algo.InterpolateAttributes )
    {
      // Make sure we don't interpolate the input scalars twice
      output->GetPointData()->InterpolateAllocate(input->GetPointData(),totalPts);
      output->GetPointData()->RemoveArray(inScalars->GetName());
      algo.Arrays.ExcludeArray(inScalars);
      algo.Arrays.AddArrays(totalPts,input->GetPointData(),output->GetPointData());
    }

    // PASS 4: Fourth and final pass: Process voxel rows and generate output.
    // Note that we are simultaneously generating triangles and interpolating
    // points. These could be split into separate, parallel operations for
    // maximum performance.
    Pass4<T> pass4(&algo);
    vtkSMPTools::For(0,algo.Dims[2]-1, pass4);
  }

  // Clean up and return
  delete [] algo.XCases;
  delete [] algo.EdgeMetaData;
}

}//anonymous namespace

//----------------------------------------------------------------------------
// Here is the VTK class proper.
// Construct object with a single contour value of 0.0.
vtkFlyingEdgesPlaneCutter::vtkFlyingEdgesPlaneCutter()
{
  this->Plane = vtkPlane::New();
  this->ComputeNormals = 0;
  this->InterpolateAttributes = 0;
  this->ArrayComponent = 0;

  // by default process active point scalars
  this->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
vtkFlyingEdgesPlaneCutter::~vtkFlyingEdgesPlaneCutter()
{
  this->SetPlane(NULL);
}

//----------------------------------------------------------------------------
// Overload standard modified time function. If the plane definition is modified,
// then this object is modified as well.
vtkMTimeType vtkFlyingEdgesPlaneCutter::GetMTime()
{
  vtkMTimeType mTime=this->Superclass::GetMTime();
  if ( this->Plane != NULL )
  {
    vtkMTimeType mTime2=this->Plane->GetMTime();
    return ( mTime2 > mTime ? mTime2 : mTime );
  }
  else
  {
    return mTime;
  }
}

//----------------------------------------------------------------------------
int vtkFlyingEdgesPlaneCutter::RequestUpdateExtent(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **vtkNotUsed(inputVector),
  vtkInformationVector *vtkNotUsed(outputVector))
{
  return 1;
}

//----------------------------------------------------------------------------
int vtkFlyingEdgesPlaneCutter::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkDebugMacro(<< "Executing plane cutter");

  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkImageData *input = vtkImageData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData *output = vtkPolyData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // to be safe recompute the update extent
  this->RequestUpdateExtent(request,inputVector,outputVector);
  vtkDataArray *inScalars = this->GetInputArrayToProcess(0,inputVector);

  // Determine extent
  int* inExt = input->GetExtent();
  int exExt[6];
  inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), exExt);
  for (int i=0; i<3; i++)
  {
    if (inExt[2*i] > exExt[2*i])
    {
      exExt[2*i] = inExt[2*i];
    }
    if (inExt[2*i+1] < exExt[2*i+1])
    {
      exExt[2*i+1] = inExt[2*i+1];
    }
  }
  if ( exExt[0] >= exExt[1] || exExt[2] >= exExt[3] || exExt[4] >= exExt[5] )
  {
    vtkDebugMacro(<<"Cutting requires 3D data");
    return 0;
  }

  if ( this->Plane == NULL )
  {
    vtkDebugMacro(<<"Cutting requires vtkPlane");
    return 0;
  }

  // Check data type and execute appropriate function
  //
  if (inScalars == NULL)
  {
    vtkDebugMacro("No scalars for cutting.");
    return 0;
  }
  int numComps = inScalars->GetNumberOfComponents();

  if (this->ArrayComponent >= numComps)
  {
    vtkErrorMacro("Scalars have " << numComps << " components. "
                  "ArrayComponent must be smaller than " << numComps);
    return 0;
  }

  // Create necessary objects to hold output. We will defer the
  // actual allocation to a later point.
  vtkCellArray *newTris = vtkCellArray::New();
  vtkPoints *newPts = vtkPoints::New();
  newPts->SetDataTypeToFloat();
  vtkDataArray *newScalars = NULL;
  vtkFloatArray *newNormals = NULL;

  // We are interpolating scalars across the plane
  newScalars = inScalars->NewInstance();
  newScalars->SetNumberOfComponents(1);
  newScalars->SetName(inScalars->GetName());

  if (this->ComputeNormals)
  {
    newNormals = vtkFloatArray::New();
    newNormals->SetNumberOfComponents(3);
    newNormals->SetName("Normals");
  }

  void *ptr = input->GetArrayPointerForExtent(inScalars, exExt);
  vtkIdType *incs = input->GetIncrements(inScalars);
  switch (inScalars->GetDataType())
  {
    vtkTemplateMacro(vtkFlyingEdgesPlaneCutterAlgorithm<VTK_TT>::
                     Contour(this, input, inScalars, exExt, incs, (VTK_TT *)ptr,
                             output, newPts, newTris, newScalars, newNormals));
  }

  vtkDebugMacro(<<"Created: "
                << newPts->GetNumberOfPoints() << " points, "
                << newTris->GetNumberOfCells() << " triangles");

  // Update ourselves.
  output->SetPoints(newPts);
  newPts->Delete();

  output->SetPolys(newTris);
  newTris->Delete();

  int idx = output->GetPointData()->AddArray(newScalars);
  output->GetPointData()->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
  newScalars->Delete();

  if (newNormals)
  {
    idx = output->GetPointData()->AddArray(newNormals);
    output->GetPointData()->SetActiveAttribute(idx, vtkDataSetAttributes::NORMALS);
    newNormals->Delete();
  }

  return 1;
}

//----------------------------------------------------------------------------
int vtkFlyingEdgesPlaneCutter::FillInputPortInformation(int, vtkInformation *info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
void vtkFlyingEdgesPlaneCutter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Plane: " << this->Plane << "\n";
  os << indent << "Compute Normals: " << (this->ComputeNormals ? "On\n" : "Off\n");
  os << indent << "Interpolate Attributes: " << (this->InterpolateAttributes ? "On\n" : "Off\n");
  os << indent << "ArrayComponent: " << this->ArrayComponent << endl;
}
