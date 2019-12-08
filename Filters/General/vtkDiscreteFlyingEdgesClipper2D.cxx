/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDiscreteFlyingEdgesClipper2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkDiscreteFlyingEdgesClipper2D.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkImageTransform.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkSMPTools.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <algorithm>
#include <cmath>
#include <limits>
#include <set>
#include <vector>

vtkStandardNewMacro(vtkDiscreteFlyingEdgesClipper2D);

//============================================================================
namespace
{ // anonymous

// Determine whether an image label/value is a specified contour
// value. Different data structures are used depending on the
// number of contour values. A cache is used for the common case
// of repeated queries for the same contour value.
template <typename T>
struct ContourMap
{
  T CachedValue;
  T CachedOutValue;
  bool CachedOutValueInitialized;

  ContourMap(double* values, int vtkNotUsed(numValues))
  {
    this->CachedValue = static_cast<T>(values[0]);
    this->CachedOutValue = static_cast<T>(values[0]);
    this->CachedOutValueInitialized = false;
  }
  virtual ~ContourMap() {}
  virtual bool IsContourValue(T label) = 0;
  bool IsContourValueInCache(T label, bool& inContourSet)
  {
    if (label == this->CachedValue)
    {
      inContourSet = true;
      return true;
    }
    else if (this->CachedOutValueInitialized && label == this->CachedOutValue)
    {
      inContourSet = false;
      return true;
    }
    else
    {
      return false;
    }
  }
};

// Cache a single contour value
template <typename T>
struct SingleContourValue : public ContourMap<T>
{
  SingleContourValue(double* values)
    : ContourMap<T>(values, 1)
  {
  }
  bool IsContourValue(T label) override { return (label == this->CachedValue ? true : false); }
};

// Represent a few contour values
template <typename T>
struct ContourVector : public ContourMap<T>
{
  std::vector<T> Map;

  ContourVector(double* values, int numValues)
    : ContourMap<T>(values, numValues)
  {
    for (int vidx = 0; vidx < numValues; vidx++)
    {
      Map.push_back(static_cast<T>(values[vidx]));
    }
  }
  bool IsContourValue(T label) override
  {
    bool inContourSet;
    if (this->IsContourValueInCache(label, inContourSet))
    {
      return inContourSet;
    }

    else
    {
      if (std::find(this->Map.begin(), this->Map.end(), label) != this->Map.end())
      {
        this->CachedValue = label;
        return true;
      }
      else
      {
        this->CachedOutValue = label;
        this->CachedOutValueInitialized = true;
        return false;
      }
    }
  }
};

// Represent many contour values
template <typename T>
struct ContourSet : public ContourMap<T>
{
  std::set<T> Map;

  ContourSet(double* values, int numValues)
    : ContourMap<T>(values, numValues)
  {
    for (int vidx = 0; vidx < numValues; vidx++)
    {
      Map.insert(static_cast<T>(values[vidx]));
    }
  }
  bool IsContourValue(T label) override
  {
    bool inContourSet;
    if (this->IsContourValueInCache(label, inContourSet))
    {
      return inContourSet;
    }

    else
    {
      if (this->Map.find(label) != this->Map.end())
      {
        this->CachedValue = label;
        return true;
      }
      else
      {
        this->CachedOutValue = label;
        this->CachedOutValueInitialized = true;
        return false;
      }
    }
  }
};

// This templated class is the heart of the algorithm. Templated across
// scalar type T. vtkDiscreteFlyingEdgesClipper2D populates the information
// in this class and then invokes ContourImage() to actually initiate
// executions.
template <class T>
class vtkDiscreteClipperAlgorithm
{
public:
  // Used to classify the flying edges 2D dyad
  enum DyadClass
  {
    Outside = 0,       // dyad origin outside
    Inside = 1,        // dyad origin inside
    XIntersection = 2, // dyad x-axis intersects
    YIntersection = 4, // dyad y-axis intersects
    InteriorPoint = 8, // dyad requires interior point
  };

  // Edges to generate output polygonal primitives (aka case table). The table is
  // generated from an external program. This is a special cases table based on
  // the potential nine vertices that are necessary to tessellate the cell.
  static const unsigned char VertCases[256][23];

  // This table is used to accelerate the generation of output primitives.
  // The VertUses array, a function of the case number, indicates which pixel
  // vertices, edges, and center point intersect with the contour (i.e.,
  // require interpolation).
  unsigned char VertUses[256][9];

  // Algorithm-derived data
  unsigned char* DyadCases;
  vtkIdType* EdgeMetaData;

  // Internal variables used by the various algorithm methods. Interfaces VTK
  // image data in a form more convenient to the algorithm.
  vtkIdType Dims[2];
  int K;
  int Axis0;
  int Min0;
  int Max0;
  int Inc0;
  int Axis1;
  int Min1;
  int Max1;
  int Inc1;
  int Axis2;
  ContourMap<T>* CMap;

  // Output data. Threads write to partitioned memory.
  T* Scalars;
  vtkCellArray* NewPolys;
  float* NewPoints;
  T* NewScalars; // cell scalars if requested

  // Instantiate and initialize key data members.
  vtkDiscreteClipperAlgorithm();

  // The three threaded passes of the algorithm.
  void ClassifyXEdges(T* inPtr, vtkIdType row); // PASS 1
  void ClassifyYEdges(T* inPtr, vtkIdType row); // PASS 2
  void GenerateOutput(T* inPtr, vtkIdType row); // PASS 4

  // Place holder for now in case fancy bit fiddling is needed later.
  void SetDyadClassification(unsigned char* dPtr, unsigned char vertCase) { *dPtr = vertCase; }

  // Given the four dyads required to define a case, return the case number.
  unsigned char GetDyadCase(unsigned char d0, unsigned char d1, unsigned char d2, unsigned char d3)
  {
    unsigned char dCase = d0 & 0x1;
    dCase |= (d1 & 0x1) << 1;
    dCase |= (d2 & 0x1) << 2;
    dCase |= (d3 & 0x1) << 3;
    dCase |= (d0 & 0x2) << 3;
    dCase |= (d2 & 0x2) << 4;
    dCase |= (d0 & 0x4) << 4;
    dCase |= (d1 & 0x4) << 5;

    return dCase;
  }

  // Return number of contouring primitives (polygons) for a particular case.
  unsigned char GetNumberOfPrimitives(unsigned char caseNum) { return this->VertCases[caseNum][0]; }

  // Return the polygon connectivity length for a particular case.
  unsigned char GetConnectivityLength(unsigned char caseNum) { return this->VertCases[caseNum][1]; }

  // Return whether an interior vertex is required for a particular case.
  unsigned char GetInteriorVertex(unsigned char caseNum) { return this->VertCases[caseNum][2]; }

  // Return an array indicating which pixel vert intersect the contour.
  unsigned char* GetVertUses(unsigned char dCase) { return this->VertUses[dCase]; }

  // Produce the primitives for this pixel cell.
  struct GeneratePolysImpl
  {
    template <typename CellStateT>
    void operator()(CellStateT& state, const unsigned char* verts, int numPolys,
      const vtkIdType ptIds[9], vtkIdType& cellOffsetBegin, vtkIdType& cellConnBegin)
    {
      using ValueType = typename CellStateT::ValueType;
      auto* offsets = state.GetOffsets();
      auto* conn = state.GetConnectivity();

      size_t vid{ 0 };
      while (numPolys-- > 0)
      {
        int nPts = static_cast<int>(*verts++);
        offsets->SetValue(cellOffsetBegin++, static_cast<ValueType>(cellConnBegin));
        while (nPts-- > 0)
        {
          // Can't just do vtkCellArray::AppendLegacyFormat bc of this funky
          // conversion:
          vid = static_cast<size_t>(*verts++);
          vid = (vid <= 3 ? vid : (vid <= 13 ? (vid - 6) : 8));
          conn->SetValue(cellConnBegin++, static_cast<ValueType>(ptIds[vid]));
        }
      }
      // Write the last offset:
      offsets->SetValue(cellOffsetBegin, cellConnBegin);
    }
  };
  void GeneratePolys(unsigned char dCase, unsigned char numPolys, vtkIdType ptIds[9],
    vtkIdType& cellOffsetBegin, vtkIdType& cellConnBegin)
  {
    const unsigned char* verts = this->VertCases[dCase] + 3;
    this->NewPolys->Visit(
      GeneratePolysImpl{}, verts, numPolys, ptIds, cellOffsetBegin, cellConnBegin);
  }

  // Produce the output points on the dyad. Special cases exist on the
  // boundary. The eids[9] array is the point ids on the pixel associated with
  // the dyad.
  void GenerateDyadPoints(int ijk[3], unsigned char vCase, vtkIdType* eIds);
  void GenerateXDyadPoints(int ijk[3], unsigned char vCase, vtkIdType* eIds);
  void GenerateYDyadPoints(int ijk[3], unsigned char vCase, vtkIdType* eIds);
  void GenerateOriginDyadPoint(int ijk[3], unsigned char vCase, vtkIdType* eIds);

  // Generate cell scalar vaues if requested
  void GenerateScalars(T* s, unsigned char dCase, vtkIdType& polyNum);

  // Helper function to set up the point ids on pixel vertices including the
  // mid-edge vertices and possible interior vertex.
  unsigned char InitPixelIds(unsigned char* dPtr0, unsigned char* dPtr0x, unsigned char* dPtr1,
    unsigned char* dPtr1x, vtkIdType* eMD0, vtkIdType* eMD1, vtkIdType* ids)
  {
    unsigned char dCase = GetDyadCase(*dPtr0, *dPtr0x, *dPtr1, *dPtr1x);
    ids[0] = eMD0[0];
    ids[1] = ids[0] + this->VertUses[dCase][0] + this->VertUses[dCase][4];
    ids[2] = eMD1[0];
    ids[3] = ids[2] + this->VertUses[dCase][2] + this->VertUses[dCase][5];
    ids[4] = ids[0] + this->VertUses[dCase][0];
    ids[5] = ids[2] + this->VertUses[dCase][2];
    ids[6] = eMD0[1];
    ids[7] = ids[6] + this->VertUses[dCase][6] + this->VertUses[dCase][8];
    ids[8] = ids[6] + this->VertUses[dCase][6];
    return dCase;
  }

  // Helper function to advance the point ids along pixel rows.
  void AdvancePixelIds(unsigned char dCase, vtkIdType* ids)
  {
    ids[0] = ids[1];
    ids[1] = ids[0] + this->VertUses[dCase][0] + this->VertUses[dCase][4];
    ids[2] = ids[3];
    ids[3] = ids[2] + this->VertUses[dCase][2] + this->VertUses[dCase][5];
    ids[4] = ids[0] + this->VertUses[dCase][0];
    ids[5] = ids[2] + this->VertUses[dCase][2];
    ids[6] = ids[7];
    ids[7] = ids[6] + this->VertUses[dCase][6] + this->VertUses[dCase][8];
    ids[8] = ids[6] + this->VertUses[dCase][6];
  }

  // Threading integration via SMPTools
  template <class TT>
  class Pass1
  {
  public:
    Pass1(vtkDiscreteClipperAlgorithm<TT>* algo) { this->Algo = algo; }
    vtkDiscreteClipperAlgorithm<TT>* Algo;
    void operator()(vtkIdType row, vtkIdType end)
    {
      TT* rowPtr = this->Algo->Scalars + row * this->Algo->Inc1;
      for (; row < end; ++row)
      {
        this->Algo->ClassifyXEdges(rowPtr, row);
        rowPtr += this->Algo->Inc1;
      } // for all rows in this batch
    }
  };
  template <class TT>
  class Pass2
  {
  public:
    Pass2(vtkDiscreteClipperAlgorithm<TT>* algo) { this->Algo = algo; }
    vtkDiscreteClipperAlgorithm<TT>* Algo;
    void operator()(vtkIdType row, vtkIdType end)
    {
      TT* rowPtr = this->Algo->Scalars + row * this->Algo->Inc1;
      for (; row < end; ++row)
      {
        this->Algo->ClassifyYEdges(rowPtr, row);
        rowPtr += this->Algo->Inc1;
      } // for all rows in this batch
    }
  };
  template <class TT>
  class Pass4
  {
  public:
    Pass4(vtkDiscreteClipperAlgorithm<TT>* algo) { this->Algo = algo; }
    vtkDiscreteClipperAlgorithm<TT>* Algo;
    void operator()(vtkIdType row, vtkIdType end)
    {
      T* rowPtr = this->Algo->Scalars + row * this->Algo->Inc1;
      for (; row < end; ++row)
      {
        this->Algo->GenerateOutput(rowPtr, row);
        rowPtr += this->Algo->Inc1;
      } // for all rows in this batch
    }
  };

  // Interface between VTK and templated functions
  static void ContourImage(vtkDiscreteFlyingEdgesClipper2D* self, T* scalars, vtkPoints* newPts,
    vtkDataArray* newScalars, vtkCellArray* newPolys, vtkImageData* input, int* updateExt);
};

// The case table is formatted: (numPolys, connectityLen, centerPoint,
// vi,vj,vk, vi,vj,vk, ...) referring to the pixel corner points [0,3];
// points generated on pixel edges [10,13]; and possibly the pixel center
// point (100) to define polygons. The case number is determined by combining
// the DyadClass classifications of the dyads forming a pixel. Note because
// we are generating manifold cases, the non-manifold cases are empty. THIS
// IS NOT A MARCHING CUBES-TYPE CASE TABLE. Note also that for every polygon,
// the first point is always an image corner point (which is useful for obtaining
// the cell data for the polygon).
template <class T>
const unsigned char vtkDiscreteClipperAlgorithm<T>::VertCases[256][23] = {
  // case 0 (0,0,0,0) (0,0,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 1 (1,0,0,0) (0,0,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 2 (0,1,0,0) (0,0,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 3 (1,1,0,0) (0,0,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 4 (0,0,1,0) (0,0,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 5 (1,0,1,0) (0,0,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 6 (0,1,1,0) (0,0,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 7 (1,1,1,0) (0,0,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 8 (0,0,0,1) (0,0,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 9 (1,0,0,1) (0,0,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 10 (0,1,0,1) (0,0,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 11 (1,1,0,1) (0,0,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 12 (0,0,1,1) (0,0,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 13 (1,0,1,1) (0,0,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 14 (0,1,1,1) (0,0,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 15 (1,1,1,1) (0,0,0,0)
  { 1, 5, 0, 4, 0, 1, 3, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 16 (0,0,0,0) (1,0,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 17 (1,0,0,0) (1,0,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 18 (0,1,0,0) (1,0,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 19 (1,1,0,0) (1,0,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 20 (0,0,1,0) (1,0,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 21 (1,0,1,0) (1,0,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 22 (0,1,1,0) (1,0,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 23 (1,1,1,0) (1,0,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 24 (0,0,0,1) (1,0,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 25 (1,0,0,1) (1,0,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 26 (0,1,0,1) (1,0,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 27 (1,1,0,1) (1,0,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 28 (0,0,1,1) (1,0,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 29 (1,0,1,1) (1,0,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 30 (0,1,1,1) (1,0,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 31 (1,1,1,1) (1,0,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 32 (0,0,0,0) (0,1,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 33 (1,0,0,0) (0,1,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 34 (0,1,0,0) (0,1,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 35 (1,1,0,0) (0,1,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 36 (0,0,1,0) (0,1,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 37 (1,0,1,0) (0,1,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 38 (0,1,1,0) (0,1,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 39 (1,1,1,0) (0,1,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 40 (0,0,0,1) (0,1,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 41 (1,0,0,1) (0,1,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 42 (0,1,0,1) (0,1,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 43 (1,1,0,1) (0,1,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 44 (0,0,1,1) (0,1,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 45 (1,0,1,1) (0,1,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 46 (0,1,1,1) (0,1,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 47 (1,1,1,1) (0,1,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 48 (0,0,0,0) (1,1,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 49 (1,0,0,0) (1,1,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 50 (0,1,0,0) (1,1,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 51 (1,1,0,0) (1,1,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 52 (0,0,1,0) (1,1,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 53 (1,0,1,0) (1,1,0,0)
  { 1, 5, 0, 4, 0, 10, 11, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 54 (0,1,1,0) (1,1,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 55 (1,1,1,0) (1,1,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 56 (0,0,0,1) (1,1,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 57 (1,0,0,1) (1,1,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 58 (0,1,0,1) (1,1,0,0)
  { 1, 5, 0, 4, 1, 3, 11, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 59 (1,1,0,1) (1,1,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 60 (0,0,1,1) (1,1,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 61 (1,0,1,1) (1,1,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 62 (0,1,1,1) (1,1,0,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 63 (1,1,1,1) (1,1,0,0)
  { 2, 10, 0, 4, 0, 10, 11, 2, 4, 1, 3, 11, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 64 (0,0,0,0) (0,0,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 65 (1,0,0,0) (0,0,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 66 (0,1,0,0) (0,0,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 67 (1,1,0,0) (0,0,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 68 (0,0,1,0) (0,0,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 69 (1,0,1,0) (0,0,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 70 (0,1,1,0) (0,0,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 71 (1,1,1,0) (0,0,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 72 (0,0,0,1) (0,0,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 73 (1,0,0,1) (0,0,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 74 (0,1,0,1) (0,0,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 75 (1,1,0,1) (0,0,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 76 (0,0,1,1) (0,0,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 77 (1,0,1,1) (0,0,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 78 (0,1,1,1) (0,0,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 79 (1,1,1,1) (0,0,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 80 (0,0,0,0) (1,0,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 81 (1,0,0,0) (1,0,1,0)
  { 1, 4, 0, 3, 0, 10, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 82 (0,1,0,0) (1,0,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 83 (1,1,0,0) (1,0,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 84 (0,0,1,0) (1,0,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 85 (1,0,1,0) (1,0,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 86 (0,1,1,0) (1,0,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 87 (1,1,1,0) (1,0,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 88 (0,0,0,1) (1,0,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 89 (1,0,0,1) (1,0,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 90 (0,1,0,1) (1,0,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 91 (1,1,0,1) (1,0,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 92 (0,0,1,1) (1,0,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 93 (1,0,1,1) (1,0,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 94 (0,1,1,1) (1,0,1,0)
  { 1, 6, 0, 5, 3, 2, 12, 10, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 95 (1,1,1,1) (1,0,1,0)
  { 2, 10, 0, 5, 3, 2, 12, 10, 1, 3, 0, 10, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 96 (0,0,0,0) (0,1,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 97 (1,0,0,0) (0,1,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 98 (0,1,0,0) (0,1,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 99 (1,1,0,0) (0,1,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 100 (0,0,1,0) (0,1,1,0)
  { 1, 4, 0, 3, 2, 12, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 101 (1,0,1,0) (0,1,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 102 (0,1,1,0) (0,1,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 103 (1,1,1,0) (0,1,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 104 (0,0,0,1) (0,1,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 105 (1,0,0,1) (0,1,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 106 (0,1,0,1) (0,1,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 107 (1,1,0,1) (0,1,1,0)
  { 1, 6, 0, 5, 1, 3, 11, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 108 (0,0,1,1) (0,1,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 109 (1,0,1,1) (0,1,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 110 (0,1,1,1) (0,1,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 111 (1,1,1,1) (0,1,1,0)
  { 2, 10, 0, 5, 1, 3, 11, 12, 0, 3, 2, 12, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 112 (0,0,0,0) (1,1,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 113 (1,0,0,0) (1,1,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 114 (0,1,0,0) (1,1,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 115 (1,1,0,0) (1,1,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 116 (0,0,1,0) (1,1,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 117 (1,0,1,0) (1,1,1,0)
  { 2, 10, 1, 4, 2, 12, 100, 13, 4, 0, 10, 100, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 118 (0,1,1,0) (1,1,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 119 (1,1,1,0) (1,1,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 120 (0,0,0,1) (1,1,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 121 (1,0,0,1) (1,1,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 122 (0,1,0,1) (1,1,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 123 (1,1,0,1) (1,1,1,0)
  { 2, 11, 1, 5, 1, 3, 11, 100, 10, 4, 0, 10, 100, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 124 (0,0,1,1) (1,1,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 125 (1,0,1,1) (1,1,1,0)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 126 (0,1,1,1) (1,1,1,0)
  { 2, 11, 1, 5, 1, 3, 11, 100, 10, 4, 2, 12, 100, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 127 (1,1,1,1) (1,1,1,0)
  { 3, 16, 1, 5, 1, 3, 11, 100, 10, 4, 0, 10, 100, 12, 4, 2, 12, 100, 11, 0, 0, 0, 0 },
  // case 128 (0,0,0,0) (0,0,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 129 (1,0,0,0) (0,0,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 130 (0,1,0,0) (0,0,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 131 (1,1,0,0) (0,0,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 132 (0,0,1,0) (0,0,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 133 (1,0,1,0) (0,0,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 134 (0,1,1,0) (0,0,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 135 (1,1,1,0) (0,0,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 136 (0,0,0,1) (0,0,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 137 (1,0,0,1) (0,0,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 138 (0,1,0,1) (0,0,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 139 (1,1,0,1) (0,0,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 140 (0,0,1,1) (0,0,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 141 (1,0,1,1) (0,0,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 142 (0,1,1,1) (0,0,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 143 (1,1,1,1) (0,0,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 144 (0,0,0,0) (1,0,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 145 (1,0,0,0) (1,0,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 146 (0,1,0,0) (1,0,0,1)
  { 1, 4, 0, 3, 1, 13, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 147 (1,1,0,0) (1,0,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 148 (0,0,1,0) (1,0,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 149 (1,0,1,0) (1,0,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 150 (0,1,1,0) (1,0,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 151 (1,1,1,0) (1,0,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 152 (0,0,0,1) (1,0,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 153 (1,0,0,1) (1,0,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 154 (0,1,0,1) (1,0,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 155 (1,1,0,1) (1,0,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 156 (0,0,1,1) (1,0,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 157 (1,0,1,1) (1,0,0,1)
  { 1, 6, 0, 5, 2, 0, 10, 13, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 158 (0,1,1,1) (1,0,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 159 (1,1,1,1) (1,0,0,1)
  { 2, 10, 0, 5, 2, 0, 10, 13, 3, 3, 1, 13, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 160 (0,0,0,0) (0,1,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 161 (1,0,0,0) (0,1,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 162 (0,1,0,0) (0,1,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 163 (1,1,0,0) (0,1,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 164 (0,0,1,0) (0,1,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 165 (1,0,1,0) (0,1,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 166 (0,1,1,0) (0,1,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 167 (1,1,1,0) (0,1,0,1)
  { 1, 6, 0, 5, 0, 1, 13, 11, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 168 (0,0,0,1) (0,1,0,1)
  { 1, 4, 0, 3, 3, 11, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 169 (1,0,0,1) (0,1,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 170 (0,1,0,1) (0,1,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 171 (1,1,0,1) (0,1,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 172 (0,0,1,1) (0,1,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 173 (1,0,1,1) (0,1,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 174 (0,1,1,1) (0,1,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 175 (1,1,1,1) (0,1,0,1)
  { 2, 10, 0, 5, 0, 1, 13, 11, 2, 3, 3, 11, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 176 (0,0,0,0) (1,1,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 177 (1,0,0,0) (1,1,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 178 (0,1,0,0) (1,1,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 179 (1,1,0,0) (1,1,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 180 (0,0,1,0) (1,1,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 181 (1,0,1,0) (1,1,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 182 (0,1,1,0) (1,1,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 183 (1,1,1,0) (1,1,0,1)
  { 2, 11, 1, 5, 2, 0, 10, 100, 11, 4, 1, 13, 100, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 184 (0,0,0,1) (1,1,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 185 (1,0,0,1) (1,1,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 186 (0,1,0,1) (1,1,0,1)
  { 2, 10, 1, 4, 1, 13, 100, 10, 4, 3, 11, 100, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 187 (1,1,0,1) (1,1,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 188 (0,0,1,1) (1,1,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 189 (1,0,1,1) (1,1,0,1)
  { 2, 11, 1, 5, 2, 0, 10, 100, 11, 4, 3, 11, 100, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 190 (0,1,1,1) (1,1,0,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 191 (1,1,1,1) (1,1,0,1)
  { 2, 10, 1, 4, 1, 13, 100, 10, 4, 3, 11, 100, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 192 (0,0,0,0) (0,0,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 193 (1,0,0,0) (0,0,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 194 (0,1,0,0) (0,0,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 195 (1,1,0,0) (0,0,1,1)
  { 1, 5, 0, 4, 0, 1, 13, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 196 (0,0,1,0) (0,0,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 197 (1,0,1,0) (0,0,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 198 (0,1,1,0) (0,0,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 199 (1,1,1,0) (0,0,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 200 (0,0,0,1) (0,0,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 201 (1,0,0,1) (0,0,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 202 (0,1,0,1) (0,0,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 203 (1,1,0,1) (0,0,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 204 (0,0,1,1) (0,0,1,1)
  { 1, 5, 0, 4, 2, 12, 13, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 205 (1,0,1,1) (0,0,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 206 (0,1,1,1) (0,0,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 207 (1,1,1,1) (0,0,1,1)
  { 2, 10, 0, 4, 0, 1, 13, 12, 4, 2, 12, 13, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 208 (0,0,0,0) (1,0,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 209 (1,0,0,0) (1,0,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 210 (0,1,0,0) (1,0,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 211 (1,1,0,0) (1,0,1,1)
  { 2, 10, 1, 4, 0, 10, 100, 12, 4, 1, 13, 100, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 212 (0,0,1,0) (1,0,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 213 (1,0,1,0) (1,0,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 214 (0,1,1,0) (1,0,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 215 (1,1,1,0) (1,0,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 216 (0,0,0,1) (1,0,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 217 (1,0,0,1) (1,0,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 218 (0,1,0,1) (1,0,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 219 (1,1,0,1) (1,0,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 220 (0,0,1,1) (1,0,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 221 (1,0,1,1) (1,0,1,1)
  { 2, 11, 1, 5, 3, 2, 12, 100, 13, 4, 0, 10, 100, 12, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 222 (0,1,1,1) (1,0,1,1)
  { 2, 11, 1, 5, 3, 2, 12, 100, 13, 4, 1, 13, 100, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 223 (1,1,1,1) (1,0,1,1)
  { 2, 10, 1, 4, 0, 10, 100, 12, 4, 1, 13, 100, 10, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 224 (0,0,0,0) (0,1,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 225 (1,0,0,0) (0,1,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 226 (0,1,0,0) (0,1,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 227 (1,1,0,0) (0,1,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 228 (0,0,1,0) (0,1,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 229 (1,0,1,0) (0,1,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 230 (0,1,1,0) (0,1,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 231 (1,1,1,0) (0,1,1,1)
  { 2, 11, 1, 5, 0, 1, 13, 100, 12, 4, 2, 12, 100, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 232 (0,0,0,1) (0,1,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 233 (1,0,0,1) (0,1,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 234 (0,1,0,1) (0,1,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 235 (1,1,0,1) (0,1,1,1)
  { 2, 11, 1, 5, 0, 1, 13, 100, 12, 4, 3, 11, 100, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 236 (0,0,1,1) (0,1,1,1)
  { 2, 10, 1, 4, 2, 12, 100, 11, 4, 3, 11, 100, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 237 (1,0,1,1) (0,1,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 238 (0,1,1,1) (0,1,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 239 (1,1,1,1) (0,1,1,1)
  { 3, 16, 1, 5, 0, 1, 13, 100, 12, 4, 2, 12, 100, 11, 4, 3, 11, 100, 13, 0, 0, 0, 0 },
  // case 240 (0,0,0,0) (1,1,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 241 (1,0,0,0) (1,1,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 242 (0,1,0,0) (1,1,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 243 (1,1,0,0) (1,1,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 244 (0,0,1,0) (1,1,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 245 (1,0,1,0) (1,1,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 246 (0,1,1,0) (1,1,1,1)
  { 2, 10, 1, 4, 1, 13, 100, 10, 4, 2, 12, 100, 11, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 247 (1,1,1,0) (1,1,1,1)
  { 3, 15, 1, 4, 0, 10, 100, 12, 4, 1, 13, 100, 10, 4, 2, 12, 100, 11, 0, 0, 0, 0, 0 },
  // case 248 (0,0,0,1) (1,1,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 249 (1,0,0,1) (1,1,1,1)
  { 2, 10, 1, 4, 0, 10, 100, 12, 4, 3, 11, 100, 13, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 250 (0,1,0,1) (1,1,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 251 (1,1,0,1) (1,1,1,1)
  { 3, 15, 1, 4, 0, 10, 100, 12, 4, 1, 13, 100, 10, 4, 3, 11, 100, 13, 0, 0, 0, 0, 0 },
  // case 252 (0,0,1,1) (1,1,1,1)
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  // case 253 (1,0,1,1) (1,1,1,1)
  { 3, 15, 1, 4, 0, 10, 100, 12, 4, 3, 11, 100, 13, 4, 2, 12, 100, 11, 0, 0, 0, 0, 0 },
  // case 254 (0,1,1,1) (1,1,1,1)
  { 3, 15, 1, 4, 1, 13, 100, 10, 4, 3, 11, 100, 13, 4, 2, 12, 100, 11, 0, 0, 0, 0, 0 },
  // case 255 (1,1,1,1) (1,1,1,1)
  { 4, 20, 1, 4, 0, 10, 100, 12, 4, 1, 13, 100, 10, 4, 3, 11, 100, 13, 4, 2, 12, 100, 11 },
};

//----------------------------------------------------------------------------
// Instantiate and initialize key data members. Mostly we build some
// acceleration structures from the case table.
template <class T>
vtkDiscreteClipperAlgorithm<T>::vtkDiscreteClipperAlgorithm()
  : DyadCases(nullptr)
  , EdgeMetaData(nullptr)
  , Scalars(nullptr)
  , NewPolys(nullptr)
  , NewPoints(nullptr)
  , NewScalars(nullptr)
{
  int j, v, dCase, numPolys, centerPoint, numVerts;
  const unsigned char* vertCase;

  // Initialize over all the cases
  for (dCase = 0; dCase < 256; ++dCase)
  {
    for (j = 0; j < 9; ++j)
    {
      this->VertUses[dCase][j] = 0;
    }
  }

  // Populate the derived case table information.
  for (dCase = 0; dCase < 256; ++dCase)
  {
    vertCase = this->VertCases[dCase];
    numPolys = *vertCase++;
    vertCase++; // skip connectivity length
    centerPoint = *vertCase++;

    // Mark verts and edges and maybe interior point that are used by this
    // case. Thus the case table refers to a mix of different types of
    // generated vertices.
    for (j = 0; j < numPolys; ++j) // just loop over all edges
    {
      numVerts = *vertCase++;
      for (v = 0; v < numVerts; ++v) // just loop over all vertices
      {
        if (vertCase[v] <= 3) // pixel points
        {
          this->VertUses[dCase][vertCase[v]] = 1;
        }
        else if (vertCase[v] >= 10 && vertCase[v] <= 13) // edges
        {
          this->VertUses[dCase][4 + (vertCase[v] - 10)] = 1;
        }
      }
      vertCase += numVerts;
    } // for all primitives in this case

    if (centerPoint) // possible center point
    {
      this->VertUses[dCase][8] = 1;
    }
  }
}

//----------------------------------------------------------------------------
// Generate the output points
template <class T>
void vtkDiscreteClipperAlgorithm<T>::GenerateDyadPoints(
  int ijk[3], unsigned char vertCase, vtkIdType* ids)
{
  if (vertCase == 0)
  {
    return;
  }

  // Generate points from the dyad. This may include the origin point;
  // intersection points along the x & y edges; and possibly an interior center
  // pixel point.
  float* xo;
  if ((vertCase & vtkDiscreteClipperAlgorithm::Inside)) // dyad origin
  {
    xo = this->NewPoints + 3 * ids[0];
    xo[0] = ijk[0];
    xo[1] = ijk[1];
    xo[2] = ijk[2];
  }

  if ((vertCase & vtkDiscreteClipperAlgorithm::XIntersection)) // x axes edge
  {
    xo = this->NewPoints + 3 * ids[4];
    xo[0] = ijk[0] + 0.5;
    xo[1] = ijk[1];
    xo[2] = ijk[2];
  }

  if ((vertCase & vtkDiscreteClipperAlgorithm::YIntersection)) // y axes edge
  {
    xo = this->NewPoints + 3 * ids[6];
    xo[0] = ijk[0];
    xo[1] = ijk[1] + 0.5;
    xo[2] = ijk[2];
  }

  if ((vertCase & vtkDiscreteClipperAlgorithm::InteriorPoint)) // pixel center point
  {
    xo = this->NewPoints + 3 * ids[8];
    xo[0] = ijk[0] + 0.5;
    xo[1] = ijk[1] + 0.5;
    xo[2] = ijk[2];
  }
}

//----------------------------------------------------------------------------
// Generate the output points along the upper edge of the image boundary.
template <class T>
void vtkDiscreteClipperAlgorithm<T>::GenerateXDyadPoints(
  int ijk[3], unsigned char vertCase, vtkIdType* ids)
{
  if (vertCase == 0)
  {
    return;
  }

  // Generate points from the dyad along the boundary of the image. This may
  // include the origin point; and an intersection points along the x dyad
  // edge. Note that the ids[9] array comes from the pixel "below" this dyad
  // x edge.
  float* xo;
  if ((vertCase & vtkDiscreteClipperAlgorithm::Inside)) // dyad origin
  {
    xo = this->NewPoints + 3 * ids[2];
    xo[0] = ijk[0];
    xo[1] = ijk[1] + 1;
    xo[2] = ijk[2];
  }

  if ((vertCase & vtkDiscreteClipperAlgorithm::XIntersection)) // x axes edge
  {
    xo = this->NewPoints + 3 * ids[5];
    xo[0] = ijk[0] + 0.5;
    xo[1] = ijk[1] + 1;
    xo[2] = ijk[2];
  }
}

//----------------------------------------------------------------------------
// Generate the output points along the right edge of the image boundary.
template <class T>
void vtkDiscreteClipperAlgorithm<T>::GenerateYDyadPoints(
  int ijk[3], unsigned char vertCase, vtkIdType* ids)
{
  if (vertCase == 0)
  {
    return;
  }

  // Generate points from the dyad along the right boundary of the
  // image. This may include the origin point; and an intersection points
  // along the y dyad edge. Note that the ids[9] array comes from the pixel
  // "to the left of" this dyad y edge.
  float* xo;
  if ((vertCase & vtkDiscreteClipperAlgorithm::Inside)) // dyad origin
  {
    xo = this->NewPoints + 3 * ids[1];
    xo[0] = ijk[0] + 1;
    xo[1] = ijk[1];
    xo[2] = ijk[2];
  }

  if ((vertCase & vtkDiscreteClipperAlgorithm::YIntersection)) // y axes edge
  {
    xo = this->NewPoints + 3 * ids[7];
    xo[0] = ijk[0] + 1;
    xo[1] = ijk[1] + 0.5;
    xo[2] = ijk[2];
  }
}

//----------------------------------------------------------------------------
// Generate the output point at the origin of the dyad. This method may be
// invoked once per execution, and it is invoked by a pixel below and to the
// left.
template <class T>
void vtkDiscreteClipperAlgorithm<T>::GenerateOriginDyadPoint(
  int ijk[3], unsigned char vertCase, vtkIdType* ids)
{
  // Generate points from the dyad at the upper right corner of the image.
  if ((vertCase & vtkDiscreteClipperAlgorithm::Inside)) // dyad origin
  {
    float* xo = this->NewPoints + 3 * ids[3];
    xo[0] = ijk[0] + 1;
    xo[1] = ijk[1] + 1;
    xo[2] = ijk[2];
  }
}

//----------------------------------------------------------------------------
// Cell scalars are produced. The pointer to the scalar field s is positioned
// at the lower left corner of a pixel. The number of scalars to produce is
// indicated by numPolys; the cellScalars
template <class T>
void vtkDiscreteClipperAlgorithm<T>::GenerateScalars(
  T* sPtr, unsigned char dCase, vtkIdType& polyNum)
{
  unsigned char numPolys = this->VertCases[dCase][0];
  const unsigned char* verts = this->VertCases[dCase] + 3;
  unsigned char nVerts, id;
  T* s;

  // Remember the first id is guaranteed to be on an image pixel
  for (int i = 0; i < numPolys; ++i)
  {
    nVerts = verts[0];
    id = verts[1];

    if (id == 0)
    {
      s = sPtr; // use s
    }
    else if (id == 1)
    {
      s = sPtr + 1;
    }
    else if (id == 2)
    {
      s = sPtr + this->Dims[0];
    }
    else //( id == 3 )
    {
      s = sPtr + 1 + this->Dims[0];
    }

    this->NewScalars[polyNum++] = *s;
    verts += (nVerts + 1);
  }
}

//----------------------------------------------------------------------------
// PASS 1: Process a single x-row and associated dyads for each pixel. Start
// building cell contour case table, determine the number of intersections,
// and trim intersections along the row. Note that dyads at the +x,y
// boundaries are partial and must be treated appropriately.
template <class T>
void vtkDiscreteClipperAlgorithm<T>::ClassifyXEdges(T* inPtr, vtkIdType row)
{
  vtkIdType nxcells = this->Dims[0];
  vtkIdType minInt = nxcells, maxInt = 0;
  unsigned char vertCase = vtkDiscreteClipperAlgorithm::Outside;
  vtkIdType* eMD = this->EdgeMetaData + row * 6;
  unsigned char* dPtr = this->DyadCases + row * nxcells;
  T s0, sx = (*inPtr);
  bool isCV0, isCVx = this->CMap->IsContourValue(sx);

  // run along the entire x-edge classifying dyad x and y axes
  std::fill_n(eMD, 6, 0);
  for (vtkIdType i = 0; i < nxcells; ++i, ++dPtr)
  {
    // At volume boundaries we have to be careful since the dyads are partial
    s0 = sx;
    isCV0 = isCVx;
    if (i == (nxcells - 1))
    { // dummy x-edge on image boundary
      sx = s0;
      isCVx = isCV0;
    }
    else
    {
      sx = static_cast<T>(*(inPtr + (i + 1) * this->Inc0));
      isCVx = this->CMap->IsContourValue(sx);
    }

    // Is the current vertex a contour value?
    if (isCV0)
    {
      vertCase = vtkDiscreteClipperAlgorithm::Inside;
      eMD[0]++; // increment number of points along x-edges
    }
    else
    {
      vertCase = vtkDiscreteClipperAlgorithm::Outside;
    }

    // Does the current x-edge need dividing?
    if ((isCV0 || isCVx) && s0 != sx)
    {
      vertCase |= vtkDiscreteClipperAlgorithm::XIntersection;
      eMD[0]++; // increment number of points along x-edges
    }

    this->SetDyadClassification(dPtr, vertCase);

    // If either x- or y-dyad edge intersects contour, or the pixel is inside
    // the region, then the pixels will have to be processed.
    if (vertCase > vtkDiscreteClipperAlgorithm::Outside)
    {
      minInt = (i < minInt ? i : minInt);
      maxInt = i + 1;
    } // if contour interacts with this dyad
  }   // for all dyad-x-edges along this image x-edge

  // The beginning and ending of intersections along the edge is used for
  // computational trimming.
  eMD[4] = minInt;
  eMD[5] = (maxInt < nxcells ? maxInt : nxcells - 1);
}

//----------------------------------------------------------------------------
// PASS 2: Classify the y-axis portion of the dyads along a single x-row.
// Determine whether an interior point is needed for the tessellation.
template <class T>
void vtkDiscreteClipperAlgorithm<T>::ClassifyYEdges(T* inPtr0, vtkIdType row)
{
  // Edge metadata
  vtkIdType* eMD0 = this->EdgeMetaData + row * 6;
  vtkIdType* eMD1 = this->EdgeMetaData + (row + 1) * 6;

  // Determine whether this row of pixels needs processing. If there are no
  // x-axis dyad intersections or corner points (dyad origin), then no y-axis
  // pixel classification is required.
  if ((eMD0[0] == 0) && // any x-axis dyad ints from bottom row?
    (eMD1[0] == 0))     // any x-axis dyad ints from top row?
  {
    return; // there are no y-dyad-ints or interior points, thus no
            // classification required
  }

  // Okay need to do more work.
  // Current row pointer from the row above

  // The trim edges may need adjustment; check both top and bottom x-edges
  // to process the maximal extent.
  vtkIdType xL = ((eMD0[4] < eMD1[4]) ? eMD0[4] : eMD1[4]);
  vtkIdType xR = ((eMD0[5] > eMD1[5]) ? eMD0[5] : eMD1[5]);

  // Grab the dyad cases bounding this pixel. Remember this is trimmed.
  inPtr0 += xL;
  T* inPtr1 = inPtr0 + this->Inc1;
  ;
  T* inPtr0x = inPtr0 + 1;
  T* inPtr1x = inPtr0x + this->Inc1;

  // The dyad cases surrounding this pixel
  unsigned char* dPtr0 = this->DyadCases + row * this->Dims[0] + xL;
  unsigned char* dPtr1 = dPtr0 + this->Dims[0];
  unsigned char* dPtr0x = dPtr0 + 1;
  unsigned char* dPtr1x = dPtr1 + 1;

  // Check if dyad y-axis should be split or not. If the two points are
  // different values, or both inside but different values, then splitting is
  // required. This is initially done to the left side of the pixel, in the
  // loop that follows we classify the right side of the pixel.
  if (((*dPtr0 & 0x1) != (*dPtr1 & 0x1)) || (*inPtr0 != *inPtr1))
  {
    *dPtr0 |= vtkDiscreteClipperAlgorithm::YIntersection;
    eMD0[1]++; // an dyad y-axis intersection
  }

  // Okay run along the trimmed dyads to classify y-axis edges,
  // as well as whether interior pixel points are needed.
  unsigned char dCase, numPolys;
  vtkIdType i;
  for (i = xL; i < xR; ++i) // run along the trimmed x-pixels
  {
    if (((*dPtr0x & 0x1) != (*dPtr1x & 0x1)) || (*inPtr0x != *inPtr1x))
    {
      *dPtr0x |= vtkDiscreteClipperAlgorithm::YIntersection;
      eMD0[1]++; // an dyad y-axis intersection
    }

    dCase = this->GetDyadCase(*dPtr0, *dPtr0x, *dPtr1, *dPtr1x);

    if ((numPolys = this->GetNumberOfPrimitives(dCase)) > 0)
    {
      // Now determine if interior point is required. Add this to the
      // number of y-points required.
      if (this->GetInteriorVertex(dCase))
      {
        *dPtr0 |= vtkDiscreteClipperAlgorithm::InteriorPoint;
      }
      eMD0[1] += this->GetInteriorVertex(dCase);

      // Okay let's increment the primitive count.
      eMD0[2] += numPolys;

      // Okay let's increment the connectivity list.
      eMD0[3] += this->GetConnectivityLength(dCase);
    } // if cell contains contour

    // advance the pointers along pixel rows
    dPtr0++;
    dPtr0x++;
    dPtr1++;
    dPtr1x++;
    inPtr0x++;
    inPtr1x++;
  } // for all pixels along this x-edge
}

//----------------------------------------------------------------------------
// PASS 4: Process the x-row dyads to generate output primitives, including
// point coordinates and polygon primitives. This is the fourth pass of the
// algorithm.
template <class T>
void vtkDiscreteClipperAlgorithm<T>::GenerateOutput(T* rowPtr, vtkIdType row)
{
  vtkIdType* eMD0 = this->EdgeMetaData + row * 6;
  vtkIdType* eMD1 = this->EdgeMetaData + (row + 1) * 6;
  // Return if there is nothing to do (i.e., no points to generate)
  if (eMD0[0] == eMD1[0])
  {
    return;
  }

  // Get the trim edges and prepare to generate
  vtkIdType i;
  vtkIdType rightPixels = (this->Dims[0] - 2), topPixels = (this->Dims[1] - 2);
  vtkIdType xL = ((eMD0[4] < eMD1[4]) ? eMD0[4] : eMD1[4]);
  vtkIdType xR = ((eMD0[5] > eMD1[5]) ? eMD0[5] : eMD1[5]);

  // Grab the dyads bounding the pixels along this x-row. Begin at left trim edge.
  unsigned char* dPtr0 = this->DyadCases + row * this->Dims[0] + xL;
  unsigned char* dPtr1 = dPtr0 + this->Dims[0];
  unsigned char* dPtr0x = dPtr0 + 1;
  unsigned char* dPtr1x = dPtr1 + 1;

  // Traverse all pixels in this row, those containing output primitives
  // further identified for processing, meaning generating points and
  // polygons. Begin by setting up point ids on pixel edges.
  vtkIdType polyNum = eMD0[2];
  vtkIdType cellOffsetBegin = eMD0[2];
  vtkIdType cellConnBegin = eMD0[3] - eMD0[2];
  vtkIdType ids[9]; // the ids of generated points
  unsigned char numPolys;

  // Process the dyads as necessary to generate point intersections.
  dPtr0 = this->DyadCases + row * this->Dims[0] + xL;
  int ijk[3];
  ijk[1] = row + this->Min1;
  ijk[2] = this->K;
  unsigned char dCase = this->InitPixelIds(dPtr0, dPtr0x, dPtr1, dPtr1x, eMD0, eMD1, ids);

  // Run along pixels in x-row direction and generate output primitives and points.
  // The left and right boundaries of the image confuse everything as the dyads are
  // not "complete" on the boundaries.
  for (i = xL; i < xR; ++i)
  {
    // Advance point ids as necessary (if not first pixel). The reason this
    // increment is not just done at the end of this loop is that it would
    // advance into undefined memory etc.
    if (i != xL)
    {
      // Advance dyads along pixel row and get the case of the next pixel
      dPtr0++;
      dPtr0x++;
      dPtr1++;
      dPtr1x++;
      dCase = GetDyadCase(*dPtr0, *dPtr0x, *dPtr1, *dPtr1x);
      this->AdvancePixelIds(dCase, ids);
    }

    // See whether any polygonal primitives need to be generated
    if ((numPolys = this->GetNumberOfPrimitives(dCase)) > 0)
    {
      // Now produce point(s) along the dyad if needed. Watch the boundaries.
      ijk[0] = i + this->Min0;
      this->GenerateDyadPoints(ijk, *dPtr0, ids);

      // If end of row, generate partial dyad from right side of pixel
      if (i == rightPixels)
      {
        this->GenerateYDyadPoints(ijk, *dPtr0x, ids);
      }

      // If top of image, generate partial dyad from top of pixel
      if (row == topPixels)
      {
        this->GenerateXDyadPoints(ijk, *dPtr1, ids);
        // If top right pixel, then the origin of the dyad may contribute a point
        if (i == rightPixels)
        {
          this->GenerateOriginDyadPoint(ijk, *dPtr1x, ids);
        }
      }

      // Generate polygons for this case
      this->GeneratePolys(dCase, numPolys, ids, cellOffsetBegin, cellConnBegin);

      // If requested, generate cell scalars for the polygons in this case
      if (this->NewScalars)
      {
        this->GenerateScalars(rowPtr + i, dCase, polyNum);
      }
    } // if anything to be generated

  } // for all non-trimmed pixels along this x-edge
}

//----------------------------------------------------------------------------
// Contouring filter specialized for images. This templated function interfaces the
// vtkDiscreteFlyingEdgesClipper2D class with the templated algorithm class. It also invokes
// the three passes of the Flying Edges algorithm.
//
template <class T>
void vtkDiscreteClipperAlgorithm<T>::ContourImage(vtkDiscreteFlyingEdgesClipper2D* self, T* scalars,
  vtkPoints* newPts, vtkDataArray* newScalars, vtkCellArray* newPolys, vtkImageData* input,
  int* updateExt)
{
  double* values = self->GetValues();
  vtkIdType numContours = self->GetNumberOfContours();
  vtkIdType row, *eMD;

  // The update extent may be different than the extent of the image.
  // The only problem with using the update extent is that one or two
  // sources enlarge the update extent.  This behavior is slated to be
  // eliminated.
  vtkIdType incs[3];
  input->GetIncrements(incs);
  int* ext = input->GetExtent();

  // Figure out which 2D plane the image lies in. Capture information for
  // subsequent processing.
  vtkDiscreteClipperAlgorithm<T> algo;
  if (updateExt[4] == updateExt[5])
  { // z collapsed
    algo.Axis0 = 0;
    algo.Min0 = updateExt[0];
    algo.Max0 = updateExt[1];
    algo.Inc0 = incs[0];
    algo.Axis1 = 1;
    algo.Min1 = updateExt[2];
    algo.Max1 = updateExt[3];
    algo.Inc1 = incs[1];
    algo.K = updateExt[4];
    algo.Axis2 = 2;
  }
  else if (updateExt[2] == updateExt[3])
  { // y collapsed
    algo.Axis0 = 0;
    algo.Min0 = updateExt[0];
    algo.Max0 = updateExt[1];
    algo.Inc0 = incs[0];
    algo.Axis1 = 2;
    algo.Min1 = updateExt[4];
    algo.Max1 = updateExt[5];
    algo.Inc1 = incs[2];
    algo.K = updateExt[2];
    algo.Axis2 = 1;
  }
  else if (updateExt[0] == updateExt[1])
  { // x collapsed
    algo.Axis0 = 1;
    algo.Min0 = updateExt[2];
    algo.Max0 = updateExt[3];
    algo.Inc0 = incs[1];
    algo.Axis1 = 2;
    algo.Min1 = updateExt[4];
    algo.Max1 = updateExt[5];
    algo.Inc1 = incs[2];
    algo.K = updateExt[0];
    algo.Axis2 = 0;
  }
  else
  {
    vtkGenericWarningMacro("Expecting 2D data.");
    return;
  }

  // Now allocate working arrays. The DyadCases array tracks case# for each pixel dyad.
  algo.Dims[0] = algo.Max0 - algo.Min0 + 1;
  algo.Dims[1] = algo.Max1 - algo.Min1 + 1;
  algo.DyadCases = new unsigned char[algo.Dims[0] * algo.Dims[1]];

  // Also allocate the characterization (metadata) array for the x edges.
  // This array tracks the number of intersections along each x-row, y-row;
  // as well as the number of polygon primitives and connectivity length, and
  // the xMin_i and xMax_i (minimum index of first intersection, maximum
  // index of intersection for row i, so-called trim edges used for
  // computational trimming).
  algo.EdgeMetaData = new vtkIdType[algo.Dims[1] * 6];

  // Compute the starting location for scalar data.  We may be operating
  // on a part of the image.
  algo.Scalars = scalars + incs[0] * (updateExt[0] - ext[0]) + incs[1] * (updateExt[2] - ext[2]) +
    incs[2] * (updateExt[4] - ext[4]) + self->GetArrayComponent();

  // This algorithm executes just once no matter how many contour values,
  // requiring a fast lookup as to whether a data value is a contour value.
  // Depending on the number of contours, different lookup strategies are
  // used.
  if (numContours == 1)
  {
    algo.CMap = new SingleContourValue<T>(values);
  }
  else if (numContours < 10)
  {
    algo.CMap = new ContourVector<T>(values, numContours);
  }
  else
  {
    algo.CMap = new ContourSet<T>(values, numContours);
  }

  // The algorithm is separated into multiple passes. The first pass detects
  // intersections on row edges, counting the number of intersected edges as
  // it progresses. It also keeps track of the generated edge cases and other
  // incidental information about intersections along rows. The second pass
  // classifies y-edges and identifies intersection information.  A prefix
  // sum is used in the third pass to allocate memory for output. In the
  // fourth and final pass, intersection points (including interior points)
  // and polygonal primitives are generated.

  // PASS 1: Traverse all rows identifying intersections and classifying the
  // dyads. Also accumulate information necessary for later allocation.  For
  // example the number of output points is computed.
  Pass1<T> pass1(&algo);
  vtkSMPTools::For(0, algo.Dims[1], pass1);

  // PASS 2: Traverse all rows and process interior information. Continue building
  // dyad case table from this information.
  Pass2<T> pass2(&algo);
  vtkSMPTools::For(0, algo.Dims[1] - 1, pass2);

  // PASS 3: Now allocate output. First we have to update the x-Edge meta
  // data to partition the output into separate pieces so independent threads
  // can write into separate memory partititions. Once allocation is
  // complete, process on a row by row basis and produce output points and
  // polygon primitives, (if necessary).
  vtkIdType numXPts, numYPts, numPolys, connLen;
  vtkIdType numOutXPts, numOutYPts, numOutPolys, outConnLen;

  numOutXPts = 0;
  numOutYPts = 0;
  numOutPolys = 0;
  outConnLen = 0;
  for (row = 0; row < algo.Dims[1]; ++row)
  {
    eMD = algo.EdgeMetaData + row * 6;
    numXPts = eMD[0];
    numYPts = eMD[1];
    numPolys = eMD[2];
    connLen = eMD[3];

    eMD[0] = numOutXPts + numOutYPts;
    eMD[1] = eMD[0] + numXPts;
    eMD[2] = numOutPolys;
    eMD[3] = outConnLen;

    numOutXPts += numXPts;
    numOutYPts += numYPts;
    numOutPolys += numPolys;
    outConnLen += connLen;
  }

  // Output can now be allocated.
  vtkIdType totalPts = numOutXPts + numOutYPts;
  if (totalPts > 0)
  {
    newPts->GetData()->WriteVoidPointer(0, 3 * totalPts);
    algo.NewPoints = static_cast<float*>(newPts->GetVoidPointer(0));
    newPolys->ResizeExact(numOutPolys, outConnLen - numOutPolys);
    algo.NewPolys = newPolys;
    if (newScalars)
    {
      newScalars->WriteVoidPointer(0, numOutPolys);
      algo.NewScalars = static_cast<T*>(newScalars->GetVoidPointer(0));
    }

    // PASS 4: Now process each x-pixel-row and produce the output
    // primitives.
    Pass4<T> pass4(&algo);
    vtkSMPTools::For(0, algo.Dims[1] - 1, pass4);
  } // if output generated

  // Clean up and return
  delete[] algo.DyadCases;
  delete[] algo.EdgeMetaData;
  delete algo.CMap;
}

} // anonymous namespace

//============================================================================
//----------------------------------------------------------------------------
// Here is the VTK class proper.
vtkDiscreteFlyingEdgesClipper2D::vtkDiscreteFlyingEdgesClipper2D()
{
  this->ContourValues = vtkContourValues::New();

  this->ComputeScalars = 1;
  this->ArrayComponent = 0;

  // by default process active point scalars
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
vtkDiscreteFlyingEdgesClipper2D::~vtkDiscreteFlyingEdgesClipper2D()
{
  this->ContourValues->Delete();
}

//----------------------------------------------------------------------------
// Description:
// Overload standard modified time function. If contour values are modified,
// then this object is modified as well.
vtkMTimeType vtkDiscreteFlyingEdgesClipper2D::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  vtkMTimeType mTime2 = this->ContourValues->GetMTime();

  return (mTime2 > mTime ? mTime2 : mTime);
}

//----------------------------------------------------------------------------
// Contouring filter specialized for images (or slices from images)
//
int vtkDiscreteFlyingEdgesClipper2D::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkImageData* input = vtkImageData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkDebugMacro(<< "Executing discrete 2D clipping");

  if (this->GetNumberOfContours() < 1)
  {
    return 1;
  }

  int* ext = inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
  vtkDataArray* inScalars = this->GetInputArrayToProcess(0, inputVector);
  if (inScalars == nullptr)
  {
    vtkErrorMacro(<< "Scalars must be defined for contouring");
    return 1;
  }

  int numComps = inScalars->GetNumberOfComponents();
  if (this->ArrayComponent >= numComps)
  {
    vtkErrorMacro("Scalars have " << numComps
                                  << " components. "
                                     "ArrayComponent must be smaller than "
                                  << numComps);
    return 1;
  }

  // Create necessary objects to hold output. We will defer the
  // actual allocation to a later point.
  vtkCellArray* newPolys = vtkCellArray::New();
  vtkPoints* newPts = vtkPoints::New();
  newPts->SetDataTypeToFloat();
  vtkDataArray* newScalars = nullptr;

  if (this->ComputeScalars)
  {
    newScalars = inScalars->NewInstance();
    newScalars->SetNumberOfComponents(1);
    newScalars->SetName(inScalars->GetName());
  }

  // Check data type and execute appropriate function
  void* scalars = inScalars->GetVoidPointer(0);
  switch (inScalars->GetDataType())
  {
    vtkTemplateMacro(vtkDiscreteClipperAlgorithm<VTK_TT>::ContourImage(
      this, (VTK_TT*)scalars, newPts, newScalars, newPolys, input, ext));
  } // switch

  vtkDebugMacro(<< "Created: " << newPts->GetNumberOfPoints() << " points, "
                << newPolys->GetNumberOfCells() << " polygons");

  // Update ourselves.
  output->SetPoints(newPts);
  newPts->Delete();

  output->SetPolys(newPolys);
  newPolys->Delete();

  if (newScalars)
  {
    int idx = output->GetCellData()->AddArray(newScalars);
    output->GetCellData()->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    newScalars->Delete();
  }

  vtkImageTransform::TransformPointSet(input, output);

  return 1;
}

//----------------------------------------------------------------------------
int vtkDiscreteFlyingEdgesClipper2D::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

//----------------------------------------------------------------------------
void vtkDiscreteFlyingEdgesClipper2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  this->ContourValues->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Compute Scalars: " << (this->ComputeScalars ? "On\n" : "Off\n");
  os << indent << "ArrayComponent: " << this->ArrayComponent << endl;
}
