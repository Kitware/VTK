// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkSurfaceNets2D.h"

#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkFloatArray.h"
#include "vtkImageData.h"
#include "vtkImageTransform.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLabelMapLookup.h"
#include "vtkLogger.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkSMPTools.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <memory>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkSurfaceNets2D);

//============================================================================
// The generation of surface nets consists of two major steps: 1) Extract a
// boundary surface from the labeled data, and 2) smooth the surface to
// improve its quality. (In the case of 2D, the "surface" is a linked set of
// line segments.) Note that the smoothing of the surface requires smoothing
// stencils, which connect points in the center of squares to potential
// points in edge neighbors, and is used in an iterative smoothing
// process. In this implementation of surface nets, a
// vtkConstrainedSmoothingFilter performs the smoothing.
//
// A templated surface nets extraction algorithm implementation follows. It
// uses a edge-by-edge parallel algorithm (aka flying edges) for
// performance. There are four passes to the algorithm: 1) classify
// x-edges. 2) classify y-edges, 3) perform a prefix sum to determine where
// to write / allocate output data, and 4) a output generation pass (i.e.,
// generate points, line segments, smoothing stencils, and optional scalar
// data).
//
// Terminology: four pixels (which in VTK is point-associated data) are
// combined to create squares (which in VTK are cells). A dyad (x-y axis) is
// positioned in the lower-left corner pixel of each square, and carries
// information about the classification of the pixel and associated
// square. This information is combined to configure the filter output, and
// control the generation of the output boundary lines (and smoothing
// stencils). An edge case is determined by combining the four edges of a
// square cell, and setting bits corresponding to the "active" edges of the
// cell.  (An active edge is one that is split by a passing boundary, and/or
// is used to generate a smoothing stencil connection.) Edges are numbered as
// follows for a dyad located at x-y: 0) x-edge, 1) (x+1)-edge, 2) y-edge, 3)
// (y+1)-edge.
//
// Implementation detail: a dyad is associated with each pixel, except on the
// boundaries. On the boundaries, a layer of extra dyads "pads" the image.
// This is done to simplify the generation of the surface net, and to enable
// the resulting boundary edges to extend 1/2 square beyond the edges of the
// image, since we are stretching VTK's definition of a pixel (value at a
// point) to be a region of constant value.

namespace
{ // anonymous

// A core concept is the 2D dyad. A dyad at any pixel are the two edges
// connected to the pixel in the +x, +y direction, and which when combined
// with neighboring dyads form a "square" in which points and line segments
// are generated. In Pass#1 and Pass#2, the dyads are classified and used to
// gather information about the square. In particular, the information
// gathered is whether the x-edge and/or y-edge requires "intersection",
// whether a point needs to be inserted into the square, and whether the
// origin point of the dyad is inside of any labeled region, or outside.
//
// The reason for dyads is that the can be independently computed in parallel
// (without race conditions), and then later combined to provide information
// about the "square" that they bound.
//
// The SurfaceNets struct implements the core of the surface nets
// algorithm. It uses a flying edges approach to parallel process data
// edge-by-edge, which provides edge-based parallel tasking, reduces the
// number of pixel lookups and eliminates costly coincident point merging.
template <typename T>
struct SurfaceNets
{
  // The dyad classification carries information on four different bits.
  // The first bit indicates whether the origin of the dyad is inside or
  // outside of *any* labeled region. Bit 2 indicates whether the x-edge
  // needs intersection (i.e., a surface net edges passes through it); and
  // Bit 3 whether the y-edge needs intersection. (Dyad edges require
  // intersection when the two end point values are not equal to one another,
  // and at least one of the end point values is "Inside" a labeled region.)
  // Finally, the fourth bit is used to indicate whether a point will be
  // generated in the square associated with a dyad. This fourth bit
  // simplifies and speeds up code.
  enum DyadClassification
  {
    Outside = 0,       // dyad origin point is outside of any labeled region
    Inside = 1,        // dyad origin inside of some labeled region
    XIntersection = 2, // dyad x-axis requires intersection
    YIntersection = 4, // dyad y-axis requires intersection
    ProducePoint = 8   // the square associated with this point will produce a point
  };

  // Given the three dyads (from the pixels on the square: (x,y); ([x+1],y);
  // (x,[y+1]), return the case number for this square. The case number
  // ranges from [0,16), considering the XIntersection and YIntersection
  // bits of all contributing dyads.
  unsigned char GetSquareCase(unsigned char d0, unsigned char d1, unsigned char d2)
  {
    unsigned char dCase = (d0 & 0x2) >> 1;
    dCase |= (d2 & 0x2);
    dCase |= (d0 & 0x4);
    dCase |= ((d1 & 0x4) << 1);

    return dCase;
  }

  // This boundary line-generation table is indexed by the edge case for a
  // square cell. (The edge case is determined by combining the three dyads
  // whose edges compose the edges of the cell.) For each square cell, up to
  // two line segments may be generated. One is an x-line that runs from the
  // cell center in the positive x direction to the edge-neighboring cell,
  // and another is a y-line that runs from the cell center in the positive y
  // direction to the edge-neighboring cell. The table consists of: 1) the
  // number of lines to be produced (two at most), and 2) 0/1 values
  // indicating whether a x-line and y-line are to be generated.
  static const unsigned char EdgeCases[16][3];
  static unsigned char GetNumberOfLines(unsigned char caseNum)
  {
    return SurfaceNets::EdgeCases[caseNum][0];
  }
  static unsigned char GenerateXLine(unsigned char caseNum)
  {
    return SurfaceNets::EdgeCases[caseNum][1];
  }
  static unsigned char GenerateYLine(unsigned char caseNum)
  {
    return SurfaceNets::EdgeCases[caseNum][2];
  }

  // This smoothing stencil table is indexed by the edge case for a square
  // cell.  For each square cell, up to four stencil edges may be generated
  // corresponding to connections to each of the cell's four edge
  // neighbors. The table consists of: 1) the number of edge connections, and
  // 2) 0/1 values indicating whether an edge is to be generated. Note: the
  // stencils are designed to perform better on boundary edges, so the stencils
  // may not be fully connected as might be expected.
  static const unsigned char StencilCases[16][5];
  static unsigned char GetNumberOfStencilEdges(unsigned char caseNum)
  {
    return SurfaceNets::StencilCases[caseNum][0];
  }
  static const unsigned char* GetStencilEdges(unsigned char caseNum)
  {
    return SurfaceNets::StencilCases[caseNum];
  }

  // Return whether a dyad, and its associated square, requires the generation
  // of a point.
  unsigned char ProducesPoint(unsigned char dCase)
  {
    return ((dCase & SurfaceNets::ProducePoint) >> 3);
  }

  // Input and output data.
  T* Scalars;                // input image scalars
  float* NewPts;             // output points
  vtkCellArray* NewLines;    // output lines
  T* NewScalars;             // output 2-component cell scalars if requested
  vtkCellArray* NewStencils; // output smoothing stencils

  // Internal variable to handle label processing.
  vtkIdType NumLabels;
  const double* LabelValues;
  T BackgroundLabel; // the label of any outside region

  // Internal variables used by the various algorithm methods. Interfaces VTK
  // image data in an efficient form more convenient to the algorithm.
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

  // Algorithm-derived data for bookkeeping data locations
  // when parallel computing.
  static const int EdgeMetaDataSize = 5;
  unsigned char* DyadCases;
  vtkIdType DyadDims[2];
  vtkIdType* EdgeMetaData;

  // Instantiate key data members.
  SurfaceNets()
    : Scalars(nullptr)
    , NewPts(nullptr)
    , NewLines(nullptr)
    , NewScalars(nullptr)
    , NewStencils(nullptr)
    , NumLabels(0)
    , LabelValues(nullptr)
    , BackgroundLabel(0)
    , DyadCases(nullptr)
    , DyadDims{ 0, 0 }
    , EdgeMetaData(nullptr)
  {
  }

  // Place holder for now in case fancy bit fiddling is needed later.
  void SetDyadClassification(unsigned char* dPtr, unsigned char vertCase) { *dPtr = vertCase; }

  // Classify a dyad y-edge. Use the dyad cases at both ends of the y-edge
  // first; if necessary, access the pixel values.
  unsigned char ClassifyYEdge(T* inPtr, vtkIdType i, unsigned char case0, unsigned char case1)
  {
    unsigned char inout0 = (case0 & 0x1);
    unsigned char inout1 = (case1 & 0x1);
    if (inout0 == inout1)
    {
      if (inout0 == Outside)
      { // both dyad origins are outside
        return 0;
      }
      else
      { // both dyad origins are inside, need to check regions
        T s0 = *(inPtr + i);
        T s1 = *(inPtr + i + this->Inc1);
        return (s0 == s1 ? 0 : YIntersection);
      }
    }
    else
    { // one dyad origin point is inside, one outside
      return YIntersection;
    }
  } // ClassifyYEdge

  // These two methods are used to help generate output points, lines,
  // stencils, and scalar data. They manage the numbering of points for each
  // row of squares.
  void InitRowIterator(vtkIdType row, vtkIdType pIds[3])
  {
    vtkIdType* eMD = this->EdgeMetaData + row * EdgeMetaDataSize;
    vtkIdType* eMDBelow = (row > 0 ? eMD - EdgeMetaDataSize : nullptr);
    vtkIdType* eMDAbove = eMD + EdgeMetaDataSize;

    // The row below starting point id
    pIds[0] = (eMDBelow ? eMDBelow[0] : -1); // if row 0 is undefined

    // The current row starting point id
    pIds[1] = eMD[0];

    // The row above starting point id
    pIds[2] = eMDAbove[0];
  }

  // Increment the point ids which are used to generate line segments and
  // stencils. The point ids are incremented if the current square, or the
  // square above or below have points generated inside of them.  Note that
  // the point ids refer to the squares below the current square pIds[0]; the
  // current square pIds[1]; and the square above the current square pIds[2].
  void AdvanceRowIterator(unsigned char dyads[3], vtkIdType pIds[3])
  {
    pIds[0] += this->ProducesPoint(dyads[0]);
    pIds[1] += this->ProducesPoint(dyads[1]);
    pIds[2] += this->ProducesPoint(dyads[2]);
  }

  // Given an i,j dyad index, create a new point in the center of the
  // dyad. It is possible for some points to be generated outside of the
  // actual image (i.e., in the padded boundary dyads).  The point is
  // generated in image space, later it will be transformed into world space
  // via vtkImageTransform.
  void GeneratePoint(vtkIdType ptId, vtkIdType i, vtkIdType j)
  {
    float* x = this->NewPts + 3 * ptId;
    x[0] = this->Min0 + static_cast<float>(i) - 0.5;
    x[1] = this->Min1 + static_cast<float>(j) - 0.5;
    x[2] = this->K;
  }

  // Produce the output lines for this square.
  struct GenerateLinesImpl
  {
    template <typename CellStateT>
    void operator()(CellStateT& state, unsigned char sqCase, vtkIdType* pIds, vtkIdType& lineId)
    {
      using ValueType = typename CellStateT::ValueType;
      auto* offsets = state.GetOffsets();
      auto* conn = state.GetConnectivity();

      auto offsetRange = vtk::DataArrayValueRange<1>(offsets);
      auto offsetIter = offsetRange.begin() + lineId;
      auto connRange = vtk::DataArrayValueRange<1>(conn);
      auto connIter = connRange.begin() + (lineId * 2);

      if (SurfaceNets::GenerateXLine(sqCase))
      {
        *offsetIter++ = static_cast<ValueType>(2 * lineId++);
        *connIter++ = pIds[1];
        *connIter++ = pIds[1] + 1; // in the +x direction
      }

      if (SurfaceNets::GenerateYLine(sqCase))
      {
        *offsetIter++ = static_cast<ValueType>(2 * lineId++);
        *connIter++ = pIds[1];
        *connIter++ = pIds[2]; // in the +y direction
      }
    } // operator()
  };  // GenerateLinesImpl

  // Finalize the lines array: after all the lines are inserted,
  // the last offset has to be added to complete the offsets array.
  struct FinalizeLinesOffsetsImpl
  {
    template <typename CellStateT>
    void operator()(CellStateT& state, vtkIdType numLines)
    {
      using ValueType = typename CellStateT::ValueType;
      auto* offsets = state.GetOffsets();
      auto offsetRange = vtk::DataArrayValueRange<1>(offsets);
      auto offsetIter = offsetRange.begin() + numLines;
      *offsetIter = static_cast<ValueType>(2 * numLines);
    }
  };

  // Produce the smoothing stencils for this square.
  struct GenerateStencilImpl
  {
    template <typename CellStateT>
    void operator()(CellStateT& state, unsigned char sqCase, vtkIdType* pIds, vtkIdType& sOffset)
    {
      // The point on which the stencil operates
      vtkIdType pId = pIds[1];

      auto* offsets = state.GetOffsets();
      auto* conn = state.GetConnectivity();

      auto offsetRange = vtk::DataArrayValueRange<1>(offsets);
      auto offsetIter = offsetRange.begin() + pId;
      auto connRange = vtk::DataArrayValueRange<1>(conn);
      auto connIter = connRange.begin() + sOffset;

      // Create the stencil. Note that for stencils with just one connection
      // (e.g., on the boundary of the image), the stencil point is "locked"
      // in place to prevent any motion to avoid shrinkage etc.
      vtkIdType numEdges = SurfaceNets::GetNumberOfStencilEdges(sqCase);
      *offsetIter++ = sOffset;
      sOffset += numEdges;

      if (numEdges == 1)
      {
        *connIter = pId;
        return;
      }

      // Create up to four stencil edges connecting the square edge
      // neighbors.
      const unsigned char* sEdges = SurfaceNets::GetStencilEdges(sqCase);

      // Lower neighbor
      if (sEdges[1])
      {
        *connIter++ = pIds[0];
      }

      // Upper neighbor
      if (sEdges[2])
      {
        *connIter++ = pIds[2];
      }

      // Left neighbor
      if (sEdges[3])
      {
        *connIter++ = pId - 1;
      }

      // Right neighbor
      if (sEdges[4])
      {
        *connIter++ = pId + 1;
      }
    } // operator()
  };  // GenerateStencilImpl

  // Finalize the stencils array: after all the stencils are inserted, the
  // last offset has to be added to complete the offsets array.
  struct FinalizeStencilsOffsetsImpl
  {
    template <typename CellStateT>
    void operator()(CellStateT& state, vtkIdType numPts, vtkIdType numSEdges)
    {
      using ValueType = typename CellStateT::ValueType;
      auto* offsets = state.GetOffsets();
      auto offsetRange = vtk::DataArrayValueRange<1>(offsets);
      auto offsetIter = offsetRange.begin() + numPts;
      *offsetIter = static_cast<ValueType>(numSEdges);
    }
  };

  // Initialize the 2-tuple cell scalars array. Used when only a
  // singled labeled region is being extracted (for performance
  // reasons).
  void InitializeScalars(vtkIdType numScalars)
  {
    T label = this->LabelValues[0];
    T background = this->BackgroundLabel;
    cout << "Background Label: " << background << "\n";
    T* s = this->NewScalars;
    for (auto i = 0; i < numScalars; ++i)
    {
      *s++ = label;
      *s++ = background;
    }
  }

  // Given a dyad i,j, return the pixel value. Note that the
  // dyad i,j are shifted by 1 due to the padding of the image
  // with boundary dyads.
  T GetPixelForDyad(vtkIdType i, vtkIdType row)
  {
    return *(this->Scalars + (row - 1) * this->Inc1 + (i - 1) * this->Inc0);
  }

  // Generate the 2-tuple scalar cell data for the generated
  // line segments. Used when multiple labeled regions are
  // being extracted. Since only line segments can be created
  // in the +x and +y directions, only the dyads to the right
  // and top of the square is needed.
  void GenerateScalars(unsigned char sqCase, vtkIdType i, vtkIdType row, unsigned char rDyad,
    unsigned char rDyadAbove, unsigned char dyadAbove, vtkIdType& scalarId)
  {
    T backgroundLabel = this->BackgroundLabel;
    T s0, s1;
    T* scalars = this->NewScalars + 2 * scalarId;

    // Get the in/out state of the three pixels which form the "corner"
    // of the square that the lines intersect.
    bool inOut[3];
    inOut[0] = (dyadAbove & 0x1);
    inOut[1] = (rDyadAbove & 0x1);
    inOut[2] = (rDyad & 0x1);

    // Process the two potential edges independently
    if (SurfaceNets::GenerateXLine(sqCase))
    {
      s0 = (inOut[2] ? this->GetPixelForDyad(i + 1, row) : backgroundLabel);
      s1 = (inOut[1] ? this->GetPixelForDyad(i + 1, row + 1) : backgroundLabel);
      if (s0 == backgroundLabel || (s1 != backgroundLabel && s0 > s1))
      {
        // Background label is placed last; s0<s1 if both inside
        std::swap(s0, s1);
      }
      *scalars++ = s0; // write 2-tuple
      *scalars++ = s1;
      ++scalarId;
    }

    if (SurfaceNets::GenerateYLine(sqCase))
    {
      s0 = (inOut[0] ? this->GetPixelForDyad(i, row + 1) : backgroundLabel);
      s1 = (inOut[1] ? this->GetPixelForDyad(i + 1, row + 1) : backgroundLabel);
      if (s0 == backgroundLabel || (s1 != backgroundLabel && s0 > s1))
      {
        // Background label is placed last; s0<s1 if both inside
        std::swap(s0, s1);
      }
      *scalars++ = s0; // write 2-tuple
      *scalars++ = s1;
      ++scalarId;
    }
  } // GenerateScalars

  // The following are methods supporting the four passes of the
  // surface nets extraction.

  // The first pass is used to classify the x-edges of the dyads.
  // Threading integration via SMPTools; this method supports the
  // processing of a single x-edge,
  void ClassifyXEdges(T* inPtr, vtkIdType row, vtkLabelMapLookup<T>* lMap); // PASS 1

  // The second pass is used to classify the y-edges of the dyads.
  // This method supports the processing of a x-row of squares.
  void ClassifyYEdges(T* inPtr, vtkIdType row); // PASS 2

  // The third pass is a prefix sum over the edge metadata to determine where
  // the algorithm should write its output, and then allocate output. This is
  // a serial method.
  void ProduceSquareCases(vtkIdType rowPair, bool odd);
  void ConfigureOutput(vtkPoints* newPts, vtkCellArray* newLines, vtkDataArray* newScalars,
    vtkCellArray* stencils); // PASS 3

  // The fourth pass produces the output geometry (i.e., points) and
  // topology (line segments and smoothing stencils). It supports
  // processing a x-row of squares,
  void GenerateOutput(vtkIdType row); // PASS 4

}; // SurfaceNets

// Case tables to control the generation of lines, and to produce
// smoothing stencils.

// Control how boundary lines are generated. The three numbers per case
// represent then number of edges generated, and then whether an +x edge
// of +y is to be produced.
template <class T>
const unsigned char SurfaceNets<T>::EdgeCases[16][3] = {
  { 0, 0, 0 }, // case 0
  { 0, 0, 0 }, // case 1
  { 1, 0, 1 }, // case 2
  { 1, 0, 1 }, // case 3
  { 0, 0, 0 }, // case 4
  { 0, 0, 0 }, // case 5
  { 1, 0, 1 }, // case 6
  { 1, 0, 1 }, // case 7
  { 1, 1, 0 }, // case 8
  { 1, 1, 0 }, // case 9
  { 2, 1, 1 }, // case 10
  { 2, 1, 1 }, // case 11
  { 1, 1, 0 }, // case 12
  { 1, 1, 0 }, // case 13
  { 2, 1, 1 }, // case 14
  { 2, 1, 1 }, // case 15
};

// Control how smoothing stencils are generated. The five numbers per case
// represent the 1) number of connecting edges (there are a maximum of four);
// 2-5) flags indicating which of the square's edge neighbors to which the
// square connects. Note that the square's edges are numbered as defined
// by a vtkPixel cell (i.e., so stencil edges in the -y,+y,-x,+x
// directions). The stencils are tweaked so that certain situations (like "T"
// junctions) the point stencil allows motion only along the boundary edge
// (i.e., along the top of the T) to produce better results.
template <class T>
const unsigned char SurfaceNets<T>::StencilCases[16][5] = {
  { 0, 0, 0, 0, 0 }, // case 0
  { 1, 1, 0, 0, 0 }, // case 1
  { 1, 0, 1, 0, 0 }, // case 2
  { 2, 1, 1, 0, 0 }, // case 3
  { 1, 0, 0, 1, 0 }, // case 4
  { 2, 1, 0, 1, 0 }, // case 5
  { 2, 0, 1, 1, 0 }, // case 6
  { 2, 1, 1, 0, 0 }, // case 7
  { 1, 0, 0, 0, 1 }, // case 8
  { 2, 1, 0, 0, 1 }, // case 9
  { 2, 0, 1, 0, 1 }, // case 10
  { 2, 1, 1, 0, 0 }, // case 11
  { 2, 0, 0, 1, 1 }, // case 12
  { 2, 0, 0, 1, 1 }, // case 13
  { 2, 0, 0, 1, 1 }, // case 14
  { 4, 1, 1, 1, 1 }, // case 15
};

// Implementations of the four passes of the surface nets boundary extraction
// process.

//------------------------------------------------------------------------------
// Support PASS 1: Process a single x-row and associated dyad's x-axis for
// each pixel on that row.  Trim intersections along the row. Note that only dyads
// associated with a pixel are processed: the padded / partial dyads are treated
// as special cases.
template <typename T>
void SurfaceNets<T>::ClassifyXEdges(T* inPtr, vtkIdType row, vtkLabelMapLookup<T>* lMap)
{
  T s0, s1 = (*inPtr); // s1 first pixel in row
  bool isLV0, isLV1 = lMap->IsLabelValue(s1);
  vtkIdType numDyads = this->DyadDims[0];
  unsigned char* rowDyadPtr = this->DyadCases + row * this->DyadDims[0];
  vtkIdType minInt = numDyads, maxInt = 0;
  vtkIdType* eMD = this->EdgeMetaData + row * EdgeMetaDataSize;

  // Run along the entire x-edge classifying the dyad x axes. Be careful
  // with the padded dyads: only process dyads whose origin is on a pixel.
  // Note that the ith dyad corresponds to the (i-1) image pixel.
  for (vtkIdType i = 0; i < (numDyads - 1); ++i)
  {
    // This handles the left-hand edge of the image as well as setting up for
    // the next dyad.
    unsigned char* dPtr = rowDyadPtr + i;
    s0 = s1;
    isLV0 = isLV1;

    // Check if this is the right-hand edge of the image.
    if (i == (numDyads - 2))
    {
      s1 = s0;
      isLV1 = isLV0;
    }
    else
    {
      // Processing dyads which are associated with pixels.
      s1 = static_cast<T>(*(inPtr + i * this->Inc0));
      isLV1 = (s0 == s1 ? isLV0 : lMap->IsLabelValue(s1));
    }

    // Is the current dyad origin vertex a label value?
    unsigned char vertCase = (isLV0 ? SurfaceNets::Inside : SurfaceNets::Outside);

    // Is the current x-edge split (i.e., different labels on each end).
    if ((isLV0 || isLV1) && s0 != s1)
    {
      vertCase |= SurfaceNets::XIntersection;
    }

    // If the triad origin is inside a labeled region, or if either x- or
    // y-triad edge intersects contour, then the voxels will have to be
    // processed.
    if (isLV0 || vertCase > SurfaceNets::Outside)
    {
      this->SetDyadClassification(dPtr, vertCase);
      minInt = (i < minInt ? i : minInt);
      maxInt = i + 1;
    } // if contour interacts with this dyad
  }   // for all dyad-x-edges along this image x-edge

  // The beginning and ending of intersections along the edge is used for
  // computational trimming.
  eMD[3] = minInt;
  eMD[4] = (maxInt < numDyads ? maxInt : numDyads);
} // ClassifyXEdges

//------------------------------------------------------------------------------
// Support PASS 2: Classify the y-axis portion of the dyads along a single
// x-row of squares.
template <typename T>
void SurfaceNets<T>::ClassifyYEdges(T* inPtr, vtkIdType row)
{
  // The dyad y-edges along the top and bottom do not need classification.
  if (row == 0 || row >= (this->DyadDims[1] - 2))
  {
    return;
  }

  // Classification may be required.
  // Dyad cases: this row, and the one above it.
  vtkIdType numDyads = this->DyadDims[0];
  unsigned char* dPtr = this->DyadCases + row * numDyads;
  unsigned char* dPtrAbove = dPtr + this->DyadDims[0];

  // Edge metadata: this edge eMD, and the one above it eMDAbove
  vtkIdType* eMD = this->EdgeMetaData + row * EdgeMetaDataSize;
  vtkIdType* eMDAbove = this->EdgeMetaData + (row + 1) * EdgeMetaDataSize;

  // Get the trim edges. Since we are advancing point numbering on two
  // rows simultaneously, need to take into account the trim on these
  // edges.
  vtkIdType xL = ((eMD[3] < eMDAbove[3]) ? eMD[3] : eMDAbove[3]);
  vtkIdType xR = ((eMD[4] > eMDAbove[4]) ? eMD[4] : eMDAbove[4]);

  // Determine whether this row of squares needs processing. If no x-edge
  // intersections exist in this row or the row above, and the row above has
  // the same pixel value as this row, then this row can be skipped.
  if ((eMD[3] == numDyads) && (eMDAbove[3] == numDyads) &&
    ((*(dPtr + 1) == SurfaceNets::Outside && *(dPtr + 1) == *(dPtrAbove + 1)) ||
      (*inPtr == *(inPtr + this->Inc1))))
  {
    return; // there are no x- or y-ints, thus no contour, skip row of squares
  }

  // Classify all the dyad y-edges, excluding the padded dyads on the LHS and
  // RHS of the image.
  unsigned char dCase, dCaseAbove;
  for (vtkIdType i = xL; i < xR; ++i)
  {
    dCase = *(dPtr + i);
    dCaseAbove = *(dPtrAbove + i);
    dCase |= this->ClassifyYEdge(inPtr, (i - 1), dCase, dCaseAbove);
    this->SetDyadClassification(dPtr + i, dCase);
  } // for all squares in this image x-row

} // ClassifyYEdges

// Process the squares in a row, combining dyads to determine the square cases.
// If a square case is non-zero, then a point will be generated in the
// square, as well as a stencil and possibly some line segments. To simplify
// the code, a bit is set in the dyad corresponding to the square (ProducePoint).
// Because the dyads from two rows are combined to produce a square case,
// setting this bit produces a race condition. Thus the processing of squares
// is interleaved (i.e., odd and even rows) to avoid the race condition.
template <typename T>
void SurfaceNets<T>::ProduceSquareCases(vtkIdType rowPair, bool odd)
{
  vtkIdType row = 2 * rowPair + (odd ? 1 : 0);
  if (row >= (this->DyadDims[1] - 1))
  {
    return; // don't process the last padded edge
  }
  vtkIdType numDyads = this->DyadDims[0];
  vtkIdType minInt = numDyads, maxInt = 0;
  vtkIdType* eMD = this->EdgeMetaData + row * EdgeMetaDataSize;
  unsigned char* dPtr = this->DyadCases + row * numDyads;
  unsigned char* dPtrAbove = dPtr + this->DyadDims[0];

  for (auto i = 0; i < (numDyads - 1); ++i)
  {
    unsigned char lCase = *(dPtr + i);
    unsigned char rCase = *(dPtr + i + 1);
    unsigned char lCaseAbove = *(dPtrAbove + i);
    unsigned char sqCase = this->GetSquareCase(lCase, rCase, lCaseAbove);

    if (sqCase > 0) // then a point must be generated
    {
      // Set the bit indicating the dyad's square will generate a point
      lCase |= SurfaceNets::ProducePoint;
      this->SetDyadClassification(dPtr + i, lCase);
      // Update metadata
      eMD[0]++;                                        // number of points generated
      eMD[1] += this->GetNumberOfLines(sqCase);        // number of lines
      eMD[2] += this->GetNumberOfStencilEdges(sqCase); // stencil edges
      // Edge trimming
      minInt = (i < minInt ? i : minInt);
      maxInt = i + 1;
    } // if produces a point
  }   // for all dyads on this row
  eMD[3] = minInt;
  eMD[4] = (maxInt < numDyads ? maxInt : numDyads);
} // ProduceSquareCases

//------------------------------------------------------------------------------
// PASS 3: Dyad classification is complete. Now combine the dyads to produce
// square cases, which indicate whether points, lines, and stencils are to
// be generated. A prefix sum is used to sum up and determine beginning point,
// line, and stencil numbers for each row. The trim edges per row can also be
// set (to avoid processing squares during output generation).
template <typename T>
void SurfaceNets<T>::ConfigureOutput(
  vtkPoints* newPts, vtkCellArray* newLines, vtkDataArray* newScalars, vtkCellArray* stencils)
{
  // Traverse all rows, combining dyads to determine square cases. Using the
  // case, sum up the number of points, lines, and stencils generated for
  // each row. Note that to avoid race conditions, pairs of rows are processed
  // (i.e., row interleaving is performed).
  vtkIdType numRows = this->DyadDims[1];
  vtkIdType numRowPairs = (numRows - 1) / 2 + 1;
  vtkSMPTools::For(0, numRowPairs, [this](vtkIdType rowPair, vtkIdType endRowPair) {
    for (; rowPair < endRowPair; ++rowPair)
    {
      this->ProduceSquareCases(rowPair, false); // even rows
    }
  });
  vtkSMPTools::For(0, numRowPairs, [this](vtkIdType rowPair, vtkIdType endRowPair) {
    for (; rowPair < endRowPair; ++rowPair)
    {
      this->ProduceSquareCases(rowPair, true); // odd rows
    }
  });

  // Begin prefix sum to determine the point, line, and stencil number
  // offsets for each row.
  vtkIdType* eMD;
  vtkIdType row, numPts, numLines, numSEdges;

  // Accumulate the total number of points, lines, and stencil edges
  // across all the image x-rows.
  vtkIdType numOutPts = 0;
  vtkIdType numOutLines = 0;
  vtkIdType numOutSEdges = 0;

  // Visit all edge metadata: Process all dyads and associated squares.
  // The very top row does not need processing.
  for (row = 0; row < (numRows - 1); ++row)
  {
    eMD = this->EdgeMetaData + row * EdgeMetaDataSize;
    numPts = eMD[0];
    numLines = eMD[1];
    numSEdges = eMD[2];

    eMD[0] = numOutPts;
    eMD[1] = numOutLines;
    eMD[2] = numOutSEdges;

    numOutPts += numPts;
    numOutLines += numLines;
    numOutSEdges += numSEdges;
  }

  // Output can now be allocated.
  if (numOutPts > 0)
  {
    // Points, which are floats
    newPts->SetNumberOfPoints(numOutPts);
    vtkFloatArray* fPts = static_cast<vtkFloatArray*>(newPts->GetData());
    this->NewPts = fPts->GetPointer(0);

    // Boundaries, a set of lines contained in vtkCellArray
    newLines->ResizeExact(numOutLines, 2 * numOutLines);
    newLines->Visit(FinalizeLinesOffsetsImpl{}, numOutLines);
    this->NewLines = newLines;

    // Scalars, which are of type T and 2-components
    if (newScalars)
    {
      newScalars->SetNumberOfTuples(numOutLines);
      this->NewScalars = static_cast<T*>(newScalars->GetVoidPointer(0));
      // In the special case when there is just a single segmented
      // object extracted, the scalars are initialized with the
      // two labels: [LabelValues[0],BackgroundLabel].
      if (this->NumLabels == 1)
      {
        this->InitializeScalars(numOutLines);
      }
    }

    // Smoothing stencils, which are represented by a vtkCellArray
    stencils->ResizeExact(numOutPts, numOutSEdges);
    stencils->Visit(FinalizeStencilsOffsetsImpl{}, numOutPts, numOutSEdges);
    this->NewStencils = stencils;
  }
} // ConfigureOutput

//------------------------------------------------------------------------------
// PASS 4: Process the x-row dyads to generate output primitives, including
// point coordinates, line primitives, and smoothing stencils. This is the
// fourth pass of the algorithm. Implementation notes: the image origin,
// spacing, and orientation is taken into account later when
// vtkImageTransform::TransformPointSet() is invoked.  When generating the
// points below, computations are performed in canonical image space. Also,
// to generate points, lines, and stencils, the point ids are determined by
// advancing the starting point ids from the current dyad row, as well as the
// rows below and above the current row.
template <typename T>
void SurfaceNets<T>::GenerateOutput(vtkIdType row)
{
  vtkIdType* eMD = this->EdgeMetaData + row * EdgeMetaDataSize;
  vtkIdType* eMDAbove = eMD + EdgeMetaDataSize;
  vtkIdType* eMDBelow = (row > 0 ? eMD - EdgeMetaDataSize : nullptr);
  // Return if there is nothing to do (i.e., no points, lines or stencils to
  // generate).
  if (eMD[0] == eMDAbove[0])
  {
    return;
  }

  // Get the trim edges. Since we are advancing point numbering on three
  // rows simultaneously, need to take into account the trim on these three
  // edges.
  vtkIdType xL = ((eMD[3] < eMDAbove[3]) ? eMD[3] : eMDAbove[3]);
  vtkIdType xR = ((eMD[4] > eMDAbove[4]) ? eMD[4] : eMDAbove[4]);
  xL = ((!eMDBelow || xL < eMDBelow[3]) ? xL : eMDBelow[3]);
  xR = ((!eMDBelow || xR > eMDBelow[4]) ? xR : eMDBelow[4]);

  // Grab the dyads for the current row, and the rows above and below.
  unsigned char* dPtr = this->DyadCases + row * this->DyadDims[0];
  unsigned char* dPtrAbove = dPtr + this->DyadDims[0];
  unsigned char* dPtrBelow = (row > 0 ? dPtr - this->DyadDims[0] : nullptr);

  // To determine the case of a square, we need to combine three dyads: the
  // square's dyad, the dyad to the right of the square, and the dyad above
  // the current square. The dyads[3] are the dyads of the current square
  // dyads[1], and the dyads of the square below dyads[0] and above dyads[2].
  unsigned char dyads[3];   // dyads below, current, and above the current square
  unsigned char rDyad;      // dyad of the right square
  unsigned char rDyadAbove; // dyad of the top-right square
  unsigned char squareCase; // the case of the current square

  // Initialize the point numbering process using a row iterator. This uses
  // the information gathered from the prefix sum (Pass3) and contained in
  // the edge meta data to obtain point numbers/ids, and the number/size of lines
  // and stencils. The pIds[3] are the current starting point ids for the row
  // below, current row, and row above; they carry the "state" of the iterator
  // and are used to determine the point ids defining the lines and stencils.
  // The point ids are advanced as a function of the three dyads dyads[3].
  vtkIdType pIds[3];
  this->InitRowIterator(row, pIds);
  vtkIdType lineId = eMD[1];   // starting line id for this row of squares
  vtkIdType sOffset = eMD[2];  // starting stencil offset for this row of squares
  vtkIdType scalarId = lineId; // starting scalar id to generate 2-tuples

  // Control whether 2-tuple scalars need to be generated.
  bool genScalars = (this->NewScalars && this->NumLabels > 1);

  // Now traverse all the squares in this row, generating points, lines,
  // stencils, and optional scalar data. Points are only generated from the
  // current row; line segments from the current square in the right and
  // upper directions; and stencils connecting a square's point to four
  // possible edge neighbors (below, above, left, right).
  for (auto i = xL; i < xR; ++i)
  {
    dyads[0] = (dPtrBelow ? *(dPtrBelow + i) : 0);
    dyads[1] = *(dPtr + i);
    dyads[2] = *(dPtrAbove + i);

    // See if anything is to be generated in this square.
    if (this->ProducesPoint(dyads[1]))
    {
      rDyad = *(dPtr + i + 1);
      squareCase = this->GetSquareCase(dyads[1], rDyad, dyads[2]);

      // Output point in the center of the square
      this->GeneratePoint(pIds[1], i, row);

      // Lines, if any. (Only +x and +y line segments can be generated.)
      // If lines are produced, then scalar data may need to be generated
      // as well.
      if (this->GetNumberOfLines(squareCase) > 0)
      {
        this->NewLines->Visit(GenerateLinesImpl{}, squareCase, pIds, lineId);
        if (genScalars)
        {
          rDyadAbove = *(dPtrAbove + i + 1);
          this->GenerateScalars(squareCase, i, row, rDyad, rDyadAbove, dyads[2], scalarId);
        }
      }

      // Smoothing stencil (i.e., how generated points are connected to other points)
      this->NewStencils->Visit(GenerateStencilImpl{}, squareCase, pIds, sOffset);

    } // if generate a point

    // Need to increment the point ids
    this->AdvanceRowIterator(dyads, pIds);
  } // for all dyads on this row

} // GenerateOutput

// This worker controls the overall algorithm flow, and handles templated
// dispatch based on the input scalar type. It also interfaces the algorithm
// to the vtkSMPTools / threading infrastructure.
struct NetsWorker
{
  // PASS 1: Process all dyads on the given x-rows to classify dyad
  // x-axis. Interface to vtkSMPTools::For(). Note that dyad row i
  // corresponds to image row (i-1). Also note that looking up labels can be
  // expensive, so a vtkLabelMapLookup is used to accelerate the lookup
  // process.
  template <typename T>
  struct Pass1
  {
    SurfaceNets<T>* Algo;
    // The label map lookup caches information, so to avoid race conditions,
    // an instance per thread must be created.
    vtkSMPThreadLocal<vtkLabelMapLookup<T>*> LMap;
    Pass1(SurfaceNets<T>* algo) { this->Algo = algo; }
    void Initialize()
    {
      this->LMap.Local() =
        vtkLabelMapLookup<T>::CreateLabelLookup(Algo->LabelValues, Algo->NumLabels);
    }
    void operator()(vtkIdType row, vtkIdType end)
    {
      vtkLabelMapLookup<T>* lMap = this->LMap.Local();
      T* rowPtr = this->Algo->Scalars + (row - 1) * this->Algo->Inc1;
      for (; row < end; ++row)
      {
        this->Algo->ClassifyXEdges(rowPtr, row, lMap);
        rowPtr += this->Algo->Inc1;
      } // for all rows in this batch
    }
    void Reduce()
    {
      // Delete all of the label map lookups
      for (auto lmItr = this->LMap.begin(); lmItr != this->LMap.end(); ++lmItr)
      {
        delete *lmItr;
      } // over all threads

      // Pass1 does not process the bottom and top edges (because there is no
      // underlying image pixels). Here set the trim information on these edges
      // so that they are not processed.
      vtkIdType* eMD = this->Algo->EdgeMetaData;
      eMD[3] = this->Algo->DyadDims[0];
      eMD[4] = 0;
      eMD = this->Algo->EdgeMetaData + (this->Algo->DyadDims[1] - 1) * this->Algo->EdgeMetaDataSize;
      eMD[3] = this->Algo->DyadDims[0];
      eMD[4] = 0;
    }
  }; // Pass1 dispatch

  // PASS 2: Process all squares on the given x-rows to classify dyad y-axis,
  // and classify squares. Interface to vtkSMPTools::For(). Note that dyad row i
  // corresponds to image row (i-1).
  template <typename T>
  struct Pass2
  {
    SurfaceNets<T>* Algo;
    Pass2(SurfaceNets<T>* algo) { this->Algo = algo; }
    void operator()(vtkIdType row, vtkIdType end)
    {
      T* rowPtr = this->Algo->Scalars + (row - 1) * this->Algo->Inc1;
      for (; row < end; ++row)
      {
        this->Algo->ClassifyYEdges(rowPtr, row);
        rowPtr += this->Algo->Inc1;
      } // for all rows in this batch
    }
  }; // Pass2 dispatch

  // PASS 3: Configure and allocate output based on classification of
  // the first two passes.
  template <typename T>
  void Pass3(SurfaceNets<T>* algo, vtkPoints* newPts, vtkCellArray* newLines,
    vtkDataArray* newScalars, vtkCellArray* stencils)
  {
    algo->ConfigureOutput(newPts, newLines, newScalars, stencils);
  } // Pass3

  // PASS 4: Process all squares on given x-rows to produce output. Interface
  // to vtkSMPTools::For().
  template <typename T>
  struct Pass4
  {
    SurfaceNets<T>* Algo;
    Pass4(SurfaceNets<T>* algo) { this->Algo = algo; }
    void operator()(vtkIdType row, vtkIdType end)
    {
      for (; row < end; ++row)
      {
        this->Algo->GenerateOutput(row);
      } // for all rows in this batch
    }
  }; // Pass4 dispatch

  // Dispatch to SurfaceNets.
  template <typename ST>
  void operator()(ST* scalarsArray, vtkSurfaceNets2D* self, vtkImageData* input, int* updateExt,
    vtkPoints* newPts, vtkCellArray* newLines, vtkDataArray* newScalars, vtkCellArray* stencils)
  {
    // The type of data carried by the scalarsArray
    using ValueType = vtk::GetAPIType<ST>;

    // The update extent may be different than the extent of the image.
    // The only problem with using the update extent is that one or two
    // sources enlarge the update extent.  This behavior is slated to be
    // eliminated.
    vtkIdType incs[3];
    input->GetIncrements(incs);
    int* ext = input->GetExtent();

    // Figure out which 2D plane the image lies in. Capture information for
    // subsequent processing.
    SurfaceNets<ValueType> algo;
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
      vtkLog(ERROR, "Expecting 2D data.");
      return;
    }

    // Now allocate working arrays. The DyadCases array tracks case# for each
    // pixel dyad (and the corresponding square).  Note that each input image
    // pixel has an associated dyad, and the "grid" of dyads is padded out in
    // the +/-x and +/-y directions (i.e., at the left hand side and bottom of
    // the image). This simplifies the generation of the surface net, but be
    // aware that the dyads on the edges of the image are treated specially.
    // Note that the allocation of the dyads initializes them to zero; we depend
    // on this as the initial dyad classification.
    algo.Dims[0] = algo.Max0 - algo.Min0 + 1;
    algo.Dims[1] = algo.Max1 - algo.Min1 + 1;
    algo.DyadDims[0] = algo.Dims[0] + 2; // padded in the +/-x direction
    algo.DyadDims[1] = algo.Dims[1] + 2; // padded in the +/-y direction
    algo.DyadCases = new unsigned char[algo.DyadDims[0] * algo.DyadDims[1]]();

    // Also allocate the characterization (metadata) array for the x edges,
    // including the padded out -y (image bottom) dyads. This edge metadata
    // array (often referred to as eMD[5]) tracks 0) the number points
    // added along each x-row; as well as 1) the number of line primitives;
    // 2) the number of stencil edges; and the 3) xMin_i and 4) xMax_i
    // (minimum index of first intersection, maximum index of intersection
    // for row i, so-called trim edges used for computational trimming). Note
    // that the edge meta data is zero initialized.
    algo.EdgeMetaData = new vtkIdType[algo.DyadDims[1] * algo.EdgeMetaDataSize]();

    // Compute the starting offset location for scalar data.  We may be operating
    // on a part of the image.
    ValueType* scalars = static_cast<ValueType*>(static_cast<ST*>(scalarsArray)->GetPointer(0));
    algo.Scalars = scalars + incs[0] * (updateExt[0] - ext[0]) + incs[1] * (updateExt[2] - ext[2]) +
      incs[2] * (updateExt[4] - ext[4]) + self->GetArrayComponent();

    // This algorithm executes just once no matter how many contour/label
    // values, requiring a fast lookup as to whether a data/pixel value is a
    // contour value, or should be considered part of the background. In
    // Pass1, instances of vtkLabelMapLookup<T> are created (per thread)
    // which performs the fast label lookup.
    algo.NumLabels = self->GetNumberOfLabels();
    algo.LabelValues = self->GetValues();
    algo.BackgroundLabel = static_cast<ValueType>(self->GetBackgroundLabel());

    // Now execute the four passes of the surface nets boundary extraction
    // algorithm.
    // Process the dyad x-edges: note that boundary-padded dyads are not
    // processed.
    Pass1<ValueType> pass1(&algo);
    vtkSMPTools::For(1, algo.DyadDims[1] - 1, pass1);

    // Classify the dyad y-edges; finalize the dyad classification.
    Pass2<ValueType> pass2(&algo);
    vtkSMPTools::For(0, algo.DyadDims[1] - 1, pass2);

    // Prefix sum to determine the size and character of the output, and
    // then allocate it.
    Pass3(&algo, newPts, newLines, newScalars, stencils);

    // Generate the output points, lines, and scalar data.
    Pass4<ValueType> pass4(&algo);
    vtkSMPTools::For(0, algo.DyadDims[1] - 1, pass4);

    // Clean up and return
    delete[] algo.DyadCases;
    delete[] algo.EdgeMetaData;
  }

}; // NetsWorker

// This function is used to smooth the output points and lines to produce a
// more pleasing result.
void SmoothOutput(vtkPolyData* geomCache, vtkCellArray* stencils, vtkPolyData* output,
  vtkConstrainedSmoothingFilter* smoother)
{
  vtkLog(INFO, "Smoothing output");

  // Smooth the data and replace the output points.
  smoother->SetInputData(geomCache);
  smoother->SetSmoothingStencils(stencils);
  smoother->Update();

  // Shallow copy / replace points.
  output->CopyStructure(smoother->GetOutput());
  output->GetCellData()->PassData(smoother->GetOutput()->GetCellData());
} // SmoothOutput

} // anonymous namespace

//============================================================================
//------------------------------------------------------------------------------
// Here is the VTK class proper.
vtkSurfaceNets2D::vtkSurfaceNets2D()
{
  this->Labels = vtkSmartPointer<vtkContourValues>::New();

  this->ComputeScalars = true;
  this->ArrayComponent = 0;
  this->BackgroundLabel = 0;

  this->Smoothing = true;
  this->Smoother = vtkSmartPointer<vtkConstrainedSmoothingFilter>::New();

  this->DataCaching = true;
  this->GeometryCache = vtkSmartPointer<vtkPolyData>::New();
  this->StencilsCache = vtkSmartPointer<vtkCellArray>::New();

  // by default process active point scalars
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::SCALARS);
}

//------------------------------------------------------------------------------
// Overload standard modified time function. If label values or the smoothing
// filter are modified, then this object is modified as well.
vtkMTimeType vtkSurfaceNets2D::GetMTime()
{
  vtkMTimeType mTime = this->Superclass::GetMTime();
  vtkMTimeType mTime2 = this->Labels->GetMTime();
  mTime = (mTime > mTime2 ? mTime : mTime2);
  mTime2 = this->Smoother->GetMTime();

  return (mTime2 > mTime ? mTime2 : mTime);
}

//------------------------------------------------------------------------------
// Surface nets filter specialized to 2D images (or slices from images).
//
int vtkSurfaceNets2D::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkLog(INFO, "Executing Surface Nets 2D");

  // Get the information objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Get the input and output
  vtkImageData* input = vtkImageData::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPolyData* output = vtkPolyData::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  // Determine whether boundary extraction is necessary, or whether we can jump
  // directly to smoothing (and reuse the geometry cache).
  if (!this->DataCaching || this->IsCacheEmpty() ||
    this->Superclass::GetMTime() > this->SmoothingTime)
  {
    // Make sure there is data to output.
    vtkIdType numLabels = this->GetNumberOfLabels();
    if (numLabels < 1)
    {
      return 1;
    }

    int* ext = inInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT());
    vtkDataArray* inScalars = this->GetInputArrayToProcess(0, inputVector);
    if (inScalars == nullptr)
    {
      vtkLog(ERROR, "Scalars must be defined for surface nets");
      return 1;
    }

    int numComps = inScalars->GetNumberOfComponents();
    if (this->ArrayComponent >= numComps)
    {
      vtkLog(ERROR,
        "Scalars have " << numComps << " components. "
                        << "ArrayComponent must be smaller than " << numComps);
      return 1;
    }

    // Create necessary objects to hold the output. We will defer the
    // actual allocation to a later point.
    vtkNew<vtkCellArray> newLines;
    vtkNew<vtkPoints> newPts;
    newPts->SetDataTypeToFloat(); // hardwired to float
    vtkSmartPointer<vtkDataArray> newScalars;

    // Produce boundary labels if requested, use the same type as the input
    // scalars.
    if (this->ComputeScalars)
    {
      // Note that the output scalars are the same type T as the input
      // scalars due to the use of NewInstance().
      newScalars.TakeReference(inScalars->NewInstance());
      newScalars->SetNumberOfComponents(2);
      newScalars->SetName("BoundaryLabels");
    }

    // SurfaceNets requires a smoothing stencil to smooth the
    // output edges. Later the stencil will be allocated and
    // populated as the output is generated.
    vtkNew<vtkCellArray> stencils;

    // Templated algorithm goes here. Dispatch on input scalar type. Note that since all
    // VTK types are processed, we don't need dispatch fallback to vtkDataArray. Note that
    // there is a fastpath when generating output scalars when only one segmented region
    // is being extracted.
    using NetsDispatch = vtkArrayDispatch::DispatchByValueType<vtkArrayDispatch::AllTypes>;
    NetsWorker netsWorker;
    if (!NetsDispatch::Execute(
          inScalars, netsWorker, this, input, ext, newPts, newLines, newScalars, stencils))
    {
      vtkLog(ERROR, "Unsupported data type"); // shouldn't happen because all types are supported
      return 1;
    }

    vtkLog(INFO,
      "Extracted: " << newPts->GetNumberOfPoints() << " points, " << newLines->GetNumberOfCells()
                    << " lines");

    // Update ourselves.
    output->SetPoints(newPts);
    output->SetLines(newLines);

    // Add the label cell data, this 2-tuple indicates what regions/labels are
    // on either side of a line segment.
    if (newScalars)
    {
      int idx = output->GetCellData()->AddArray(newScalars);
      output->GetCellData()->SetActiveAttribute(idx, vtkDataSetAttributes::SCALARS);
    }

    // Transform results into physical space. It's necessary to do this
    // before smoothing.
    vtkImageTransform::TransformPointSet(input, output);

    // For now let's stash the data. If caching is disabled, we'll flush it
    // at the end.
    this->CacheData(output, stencils);

  } // Extract boundary geometry

  // If smoothing is to occur, then do it now. It has to be done after image
  // transformation. The smoothing process will replace the current
  // output points. Make sure there is something to smooth
  vtkCellArray* stencils = this->StencilsCache;
  if (stencils && stencils->GetNumberOfCells() > 0 && this->Smoothing &&
    this->Smoother->GetNumberOfIterations() > 0)
  {
    SmoothOutput(this->GeometryCache, this->StencilsCache, output, this->Smoother);
  }
  else
  {
    output->CopyStructure(this->GeometryCache);
    output->GetCellData()->PassData(this->GeometryCache->GetCellData());
  }
  this->SmoothingTime.Modified();

  // Flush the cache if caching is disabled.
  if (!this->DataCaching)
  {
    this->GeometryCache = nullptr;
    this->StencilsCache = nullptr;
  }

  return 1;
}

//------------------------------------------------------------------------------
bool vtkSurfaceNets2D::IsCacheEmpty()
{
  return (!this->StencilsCache || this->GeometryCache->GetNumberOfPoints() < 1);
}

//------------------------------------------------------------------------------
void vtkSurfaceNets2D::CacheData(vtkPolyData* pd, vtkCellArray* stencils)
{
  if (this->DataCaching)
  {
    this->GeometryCache->CopyStructure(pd);
    this->GeometryCache->GetCellData()->PassData(pd->GetCellData());

    this->StencilsCache = stencils;
  }
}

//------------------------------------------------------------------------------
int vtkSurfaceNets2D::FillInputPortInformation(int, vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

//------------------------------------------------------------------------------
void vtkSurfaceNets2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  this->Labels->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Compute Scalars: " << (this->ComputeScalars ? "On\n" : "Off\n");
  os << indent << "Background Label: " << this->BackgroundLabel << "\n";
  os << indent << "ArrayComponent: " << this->ArrayComponent << endl;

  os << indent << "Smoother: " << this->Smoother.Get() << "\n";

  os << indent << "Data Caching: " << (this->DataCaching ? "On\n" : "Off\n");
}
VTK_ABI_NAMESPACE_END
