// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkAttributeSmoothingFilter.h"

#include "vtkArrayListTemplate.h" // For processing attribute data
#include "vtkCellArray.h"
#include "vtkCellArrayIterator.h"
#include "vtkCellData.h"
#include "vtkCellLocator.h"
#include "vtkDoubleArray.h"
#include "vtkExtractEdges.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMarkBoundaryFilter.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkStaticCellLinksTemplate.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <vector>

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkAttributeSmoothingFilter);
VTK_ABI_NAMESPACE_END

// The following code defines core methods for the
// vtkAttributeSmoothingFilter class.
//
namespace
{ // anonymous

// Indicate smoothing state on a particular point. A value of ==1
// indicates that a point is to be smoothed. Any other value is
// not smoothed.
enum SmoothPointFlag
{
  NoSmooth = 0,
  SmoothPoint = 1,
  Boundary = 2
};

// Build a smoothing stencil from a VTK cell links object that is produced
// from a network of edges (which is typically generated via
// vtkExtractEdges). The stencil, for each point p, is a set of points ps
// connected to p. Here the cell links is used to create the set ps. Also
// BuildStencil builds weights associated with each stencil. The weights are
// multiplied by the relaxation factor, it's more efficient to do it
// in this method. Since the smoothing stencil also includes the current
// point, we could add the current point into the smoothing stencils and save
// work later (i.e., copying data around) but this would significantly
// increase memory usage since the each smoothing stencil would be increased
// by one point.
struct BuildStencil
{
  vtkPoints* Points;
  vtkCellArray* Lines;
  vtkStaticCellLinksTemplate<vtkIdType>* Links;
  vtkIdType* Offsets;
  vtkIdType* Conn;
  const unsigned char* Smooth;
  double* Weights;
  int WeightsType;
  double Relax;
  vtkIdType MaxStencilSize;

  // Avoid constructing/deleting the cell iterator
  vtkSMPThreadLocal<vtkSmartPointer<vtkCellArrayIterator>> Iter;
  // Maximum stencil size (for creating final stencils later)
  vtkSMPThreadLocal<vtkIdType> MaxSize;

  BuildStencil(vtkPoints* pts, vtkCellArray* lines, vtkStaticCellLinksTemplate<vtkIdType>* links,
    vtkIdType* offsets, vtkIdType* conn, const unsigned char* smooth, double* weights,
    int weightsType, double relax)
    : Points(pts)
    , Lines(lines)
    , Links(links)
    , Offsets(offsets)
    , Conn(conn)
    , Smooth(smooth)
    , Weights(weights)
    , WeightsType(weightsType)
    , Relax(relax)
    , MaxStencilSize(0)
  {
  }

  void Initialize()
  {
    this->Iter.Local().TakeReference(this->Lines->NewIterator());
    this->MaxSize.Local() = 0;
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    vtkCellArrayIterator* iter = this->Iter.Local();
    vtkIdType& maxStencilSize = this->MaxSize.Local();
    vtkStaticCellLinksTemplate<vtkIdType>* links = this->Links;
    vtkIdType npts;
    const vtkIdType* pts;
    const unsigned char* smooth = this->Smooth;
    double relax = this->Relax;
    int weightsType = this->WeightsType;

    for (; ptId < endPtId; ++ptId)
    {
      // Create a stencil and weights only if the attributes are to be
      // smoothed at this point. If no smoothing occurs at a point, then the
      // stencil and weights for that point are undefined.
      vtkIdType offset = links->GetOffset(ptId);
      if (!smooth || smooth[ptId] == SmoothPoint)
      {
        double x[3], y[3];
        this->Points->GetPoint(ptId, x);
        vtkIdType numEdges = links->GetNumberOfCells(ptId);
        maxStencilSize = (numEdges > maxStencilSize ? numEdges : maxStencilSize);
        vtkIdType* edges = links->GetCells(ptId);
        vtkIdType* c = this->Conn + offset;
        double* w = this->Weights + offset;
        double wSum = 0.0;
        int i, forceWeight = (-1);

        for (i = 0; i < numEdges; ++i)
        {
          iter->GetCellAtId(edges[i], npts, pts);
          vtkIdType id = (pts[0] != ptId ? pts[0] : pts[1]);
          c[i] = id;
          this->Points->GetPoint(id, y);
          w[i] = vtkMath::Distance2BetweenPoints(x, y);
          if (w[i] == 0)
          {
            forceWeight = i;
          }
          else
          {
            double weight = (weightsType == vtkAttributeSmoothingFilter::AVERAGE
                ? 1.0
                : (weightsType == vtkAttributeSmoothingFilter::DISTANCE ? sqrt(w[i]) : w[i]));
            w[i] = 1.0 / weight;
          }
          wSum += w[i];
        }
        // Normalize the final weights, and multiply by the
        // relaxation factor.
        if (forceWeight >= 0) // coincident point
        {
          for (auto j = 0; j < numEdges; ++j)
          {
            w[j] = 0.0;
          }
          w[forceWeight] = relax;
        }
        else
        {
          double f = relax / wSum;
          for (auto j = 0; j < numEdges; ++j)
          {
            w[j] *= f;
          }
        }
      } // if point is to be smoothed
      this->Offsets[ptId] = offset;
    } // for all points in this batch
  }   // operator()

  // Roll up the maximum stencil size. This is used later to
  // allocate memory in order to create the final stencils and weights.
  void Reduce()
  {
    vtkIdType maxStencilSize = 0;
    vtkSMPThreadLocal<vtkIdType>::iterator itr;
    vtkSMPThreadLocal<vtkIdType>::iterator itrEnd = this->MaxSize.end();
    for (itr = this->MaxSize.begin(); itr != itrEnd; ++itr)
    {
      maxStencilSize = (*itr > maxStencilSize ? *itr : maxStencilSize);
    } // over all threads
    this->MaxStencilSize = maxStencilSize;
  }

}; // BuildStencil

// Create stencils if none were provided. Leverage the vtkExtractEdges filter
// (which is threaded) to build the stencils.
vtkSmartPointer<vtkCellArray> BuildStencils(vtkPolyData* edges, const unsigned char* smooth,
  std::vector<double>& weights, double relax, int weightsType, vtkIdType& maxStencilSize)
{
  vtkNew<vtkCellArray> stencils;

  vtkPoints* pts = edges->GetPoints();
  vtkIdType numPts = edges->GetNumberOfPoints();
  vtkCellArray* lines = edges->GetLines();
  vtkIdType numLines = lines->GetNumberOfCells();

  // Make sure there is something to process.
  if (numLines < 1)
  {
    return stencils;
  }

  // Use a threaded approach to build the stencils. Recall that we
  // use a vtkCellArray to represent the stencils. Begin by building
  // links from the points to the (line) cells using the output of
  // vtkExtractEdges.
  vtkStaticCellLinksTemplate<vtkIdType> links;
  links.BuildLinks(numPts, numLines, lines);
  vtkIdType linksSize = links.GetLinksSize();

  // Building the links does most of the work. Now we transform the links
  // into smoothing stencils. For each point p, using the links, determine
  // other points connected to the point p via the connecting lines. Also
  // update the connectivity offsets. This can be done in parallel.
  vtkNew<vtkIdTypeArray> offsets;
  offsets->SetNumberOfTuples(numPts + 1);
  vtkIdType* offsetsPtr = offsets->GetPointer(0);
  offsetsPtr[numPts] = linksSize;

  vtkNew<vtkIdTypeArray> conn;
  conn->SetNumberOfTuples(linksSize);
  vtkIdType* connPtr = conn->GetPointer(0);

  // This is for the distance weights needed the smoothing.
  weights.resize(linksSize);

  // Now point by point build the smoothing stencils.
  BuildStencil buildStencil(
    pts, lines, &links, offsetsPtr, connPtr, smooth, weights.data(), weightsType, relax);
  vtkSMPTools::For(0, numPts, buildStencil);
  maxStencilSize = buildStencil.MaxStencilSize;

  // The stencils have been defined, put them in the form of a vtkCellArray and return.
  stencils->SetData(offsets, conn);
  return stencils;
} // BuildStencils

// This functor performs a single smoothing iteration over a set of point
// data attributes. Points that are to be smoothed are marked with a
// Smooth[i]==1 value. Note for smoothing to depend on the relaxation factor,
// the smoothing stencil needs to expanded to also include the point being
// smoothed.
struct SmoothAttributes
{
  ArrayList* PD;
  vtkCellArray* Stencils;
  const unsigned char* Smooth;
  const double* Weights;
  double Relax;
  vtkIdType MaxStencilSize;

  // Avoid constructing/deleting the iterator
  vtkSMPThreadLocal<vtkSmartPointer<vtkCellArrayIterator>> Iter;
  // Avoid construction/resizing smoothing stencils and weights
  vtkSMPThreadLocal<std::vector<vtkIdType>> StencilPts;
  vtkSMPThreadLocal<std::vector<double>> StencilWeights;

  SmoothAttributes(vtkCellArray* stencils, const unsigned char* smooth, const double* weights,
    double relax, vtkIdType maxStencilSize)
    : PD(nullptr)
    , Stencils(stencils)
    , Smooth(smooth)
    , Weights(weights)
    , Relax(relax)
    , MaxStencilSize(maxStencilSize)
  {
  }

  // Should be set before each iteration
  void SetSmoothingArrays(ArrayList* pd) { this->PD = pd; }

  void Initialize()
  {
    this->Iter.Local().TakeReference(this->Stencils->NewIterator());
    // Max point stencil size is used to allocate some local storage.
    // The final stencil and weights must include the current point.
    this->StencilPts.Local().resize(this->MaxStencilSize + 1);
    this->StencilWeights.Local().resize(this->MaxStencilSize + 1);
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    vtkCellArrayIterator* iter = this->Iter.Local();

    vtkIdType npts;
    const vtkIdType* pts;
    double relaxF = 1.0 - this->Relax;
    vtkIdType* pIds = this->StencilPts.Local().data();
    double* w = this->StencilWeights.Local().data();

    // Loop over all points and smooth associated attributes using a
    // distance weighted approach.
    for (; ptId < endPtId; ++ptId)
    {
      // Check to see whether this point should be smoothed. If so,
      // complete the stencil by including the current point and its
      // associated weight (the relaxation factor).
      if (!this->Smooth || this->Smooth[ptId] == SmoothPoint)
      {
        // Retrieve the current stencil surrounding this point.
        iter->GetCellAtId(ptId, npts, pts);
        vtkIdType offset = this->Stencils->GetOffset(ptId);
        const double* weights = this->Weights + offset;

        // Combine the weights from this point and the stencil. The first
        // point is from the current point (times 1-R relaxation factor). The
        // others are from the point stencil (i.e., edge connected
        // points). This extra work of copying is done to reduce memory
        // consumption (as compared to adding the current point into the
        // stencil when it is initially created).
        pIds[0] = ptId;
        w[0] = relaxF;
        for (auto i = 0; i < npts; ++i)
        {
          pIds[i + 1] = pts[i];
          w[i + 1] = weights[i];
        }

        this->PD->WeightedAverage(npts + 1, pIds, w, ptId);
      }
      else // Otherwise just copy the input data
      {
        this->PD->Copy(ptId, ptId);
      }
    } // over all points in this batch
  }

  void Reduce() {}

  // Perform a threaded smoothing pass on a pair of point data attributes.
  void Execute(vtkIdType numPts, ArrayList* attrPair)
  {
    this->SetSmoothingArrays(attrPair);
    vtkSMPTools::For(0, numPts, *this);
  }

}; // SmoothAttributes

// Mark vtkPolyData boundary points. This is a little faster than using
// vtkMarkBoundaryFilter.
void MarkPDBoundary(vtkPolyData* extractedEdges, vtkPolyData* inPolyData, unsigned char* smooth)
{
  // Needed for topological edge neighbor) operations.
  inPolyData->BuildLinks();

  vtkCellArray* lines = extractedEdges->GetLines();
  vtkIdType numLines = lines->GetNumberOfCells();
  auto iter = vtk::TakeSmartPointer(lines->NewIterator());
  vtkIdType npts;
  const vtkIdType* pts;
  vtkNew<vtkIdList> neis;

  // Traverse all edges in the dataset and determine if they
  // are boundary edges.
  for (auto lineId = 0; lineId < numLines; ++lineId)
  {
    iter->GetCellAtId(lineId, npts, pts);
    inPolyData->GetCellEdgeNeighbors((-1), pts[0], pts[1], neis);
    if (neis->GetNumberOfIds() == 1)
    { // it's a boundary edge
      smooth[pts[0]] = Boundary;
      smooth[pts[1]] = Boundary;
    }
  }
} // MarkPDBoundary

// Mark boundary points for a general dataset. Boundary points are those that
// are used by boundary faces. Boundary faces are determined by executing the
// vtkMarkBoundaryFilter.
void MarkDSBoundary(vtkDataSet* ds, unsigned char* smooth)
{
  // Produce an array indicating which points are on the boundary.
  vtkNew<vtkMarkBoundaryFilter> marker;
  marker->SetInputData(ds);
  marker->Update();

  vtkUnsignedCharArray* ptMarks = vtkUnsignedCharArray::SafeDownCast(
    marker->GetOutput()->GetPointData()->GetArray("BoundaryPoints"));
  if (!ptMarks)
  {
    return;
  }
  unsigned char* ptr = ptMarks->GetPointer(0);

  // Now copy the information over (with in-place lambda).
  vtkSMPTools::For(0, ds->GetNumberOfPoints(),
    [&ptr, &smooth](vtkIdType ptId, vtkIdType endPtId)
    {
      for (; ptId < endPtId; ++ptId)
      {
        if (ptr[ptId] != 0)
        {
          smooth[ptId] = Boundary;
        }
      }
    });
} // MarkDSBoundary

// Mark all points directly adjacent to the dataset boundary (i.e.,
// points are adjacent when connected by an edge to a boundary point).
// It is assumed that on entry to this function, all points have been
// marked either as NoSmooth or Boundary.
void MarkAdjacent(vtkPolyData* extractedEdges, unsigned char* smooth)
{
  vtkCellArray* lines = extractedEdges->GetLines();
  vtkIdType numLines = lines->GetNumberOfCells();
  auto iter = vtk::TakeSmartPointer(lines->NewIterator());
  vtkIdType npts;
  const vtkIdType* pts;

  for (auto lineId = 0; lineId < numLines; ++lineId)
  {
    iter->GetCellAtId(lineId, npts, pts);
    unsigned char s0 = smooth[pts[0]];
    unsigned char s1 = smooth[pts[1]];
    if (s0 == Boundary && s1 == NoSmooth)
    {
      smooth[pts[1]] = SmoothPoint;
    }
    else if (s1 == Boundary && s0 == NoSmooth)
    {
      smooth[pts[0]] = SmoothPoint;
    }
  }
} // MarkAdjacent

// Convenience method that excludes specified arrays from smoothing.
void ExcludeArrays(vtkPointData* inPD, ArrayList& arrList, std::vector<std::string>& exclArrays)
{
  // Manage arrays for interpolation
  for (auto const& itr : exclArrays)
  {
    vtkDataArray* array = inPD->GetArray(itr.c_str());
    if (array != nullptr)
    {
      arrList.ExcludeArray(array);
    }
  }
}

// Convenience method to add arrays excluded from smoothing.
void AddExcludedArrays(
  vtkPointData* inPD, vtkPointData* outPD, std::vector<std::string>& exclArrays)
{
  // Manage arrays for interpolation
  for (auto const& itr : exclArrays)
  {
    vtkDataArray* array = inPD->GetArray(itr.c_str());
    outPD->AddArray(array); // pass thru to filter output
  }
}

} // anonymous namespace

VTK_ABI_NAMESPACE_BEGIN
//=================Begin VTK class proper=======================================
//------------------------------------------------------------------------------
vtkAttributeSmoothingFilter::vtkAttributeSmoothingFilter()
{
  this->NumberOfIterations = 5;
  this->RelaxationFactor = 0.10;
  this->SmoothingStrategy = ALL_POINTS;
  this->WeightsType = DISTANCE2;
}

//------------------------------------------------------------------------------
int vtkAttributeSmoothingFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output. This filter can process general vtkDataSets.
  vtkDataSet* input = vtkDataSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkDataSet* output = vtkDataSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkLog(TRACE, "Executing constrained attribute filter");

  // Sanity check the input
  vtkIdType numPts = input->GetNumberOfPoints();
  if (numPts < 1)
  {
    return 1;
  }

  // Pass structure and point data through, the points attribute data will be
  // updated later.
  vtkPointData *inPD = input->GetPointData(), *outPD = output->GetPointData();
  vtkCellData *inCD = input->GetCellData(), *outCD = output->GetCellData();
  output->CopyStructure(input);
  outCD->PassData(inCD);

  // Make sure these is work to do.
  vtkPolyData* inPolyData = vtkPolyData::SafeDownCast(input);
  vtkDataSet* ds = vtkDataSet::SafeDownCast(input);
  if ((!inPolyData && !ds) || this->NumberOfIterations < 1)
  {
    outPD->PassData(inPD);
    return 1;
  }

  // Create edges from dataset which will be used to build the stencils
  // and perform topological analysis if necessary.
  vtkNew<vtkExtractEdges> extract;
  extract->SetInputData(input);
  extract->UseAllPointsOn();
  extract->Update();
  vtkPolyData* extractedEdges = extract->GetOutput();

  // Determine how the smooth flag per point is to be configured.
  unsigned char* smooth = nullptr;
  std::vector<unsigned char> smoothVector;
  if (this->SmoothingStrategy == SMOOTHING_MASK)
  {
    smooth = ((this->SmoothingMask && this->SmoothingMask->GetNumberOfTuples() == numPts)
        ? this->SmoothingMask->GetPointer(0)
        : nullptr);
  }
  else if (this->SmoothingStrategy == ALL_POINTS)
  {
    smooth = nullptr;
  }
  else // need to do some topological analysis
  {
    smoothVector.resize(numPts);
    smooth = smoothVector.data();

    if (this->SmoothingStrategy == ALL_BUT_BOUNDARY)
    {
      std::fill(smoothVector.begin(), smoothVector.end(), SmoothPoint);
      (inPolyData ? MarkPDBoundary(extractedEdges, inPolyData, smooth)
                  : MarkDSBoundary(ds, smooth));
    }
    else // if ( this->SmoothingStrategy == ADJACENT_TO_BOUNDARY )
    {
      std::fill(smoothVector.begin(), smoothVector.end(), NoSmooth);
      (inPolyData ? MarkPDBoundary(extractedEdges, inPolyData, smooth)
                  : MarkDSBoundary(ds, smooth));
      MarkAdjacent(extractedEdges, smooth);
    }
  }

  // Define a smoothing stencil, or use what's provided.
  std::vector<double> weights;
  double relax = this->RelaxationFactor;
  vtkIdType maxStencilSize;
  vtkSmartPointer<vtkCellArray> stencils =
    BuildStencils(extractedEdges, smooth, weights, relax, this->WeightsType, maxStencilSize);

  // With the stencil defined, perform the smoothing. Use a double buffering
  // approach. Since we are using a threaded algorithm and hence ArrayList, we
  // must create multiple instances of ArrayList to smooth to and from the
  // appropriate arrays.
  int numIter = this->NumberOfIterations;
  SmoothAttributes smoothAttr(stencils, smooth, weights.data(), relax, maxStencilSize);

  // Setup the smoothing iterations. Create some "temporary" point data that
  // is used to configure pairs of data.
  vtkNew<vtkPointData> evenPD; // smoothing iterations 0,2,4,6,...
  evenPD->CopyAllocate(inPD, numPts);
  vtkNew<vtkPointData> oddPD; // iterations 1,3,5,7,....

  // Setup the initial iteration
  ArrayList initPair;
  ExcludeArrays(inPD, initPair, this->ExcludedArrays);
  initPair.AddArrays(numPts, inPD, evenPD);

  // At a minimum, one iteration with the initial pair of arrays
  smoothAttr.Execute(numPts, &initPair);
  vtkPointData* smoothedPD = evenPD;

  // Now additional iterations if requested. Subsequent iterations require
  // double buffering of attributes.
  if (numIter > 1)
  {
    oddPD->CopyAllocate(inPD, numPts);
    ArrayList oddPair;
    ExcludeArrays(inPD, oddPair, this->ExcludedArrays);
    oddPair.AddArrays(numPts, evenPD, oddPD);

    ArrayList evenPair;
    ExcludeArrays(inPD, evenPair, this->ExcludedArrays);
    evenPair.AddArrays(numPts, oddPD, evenPD);

    // Double buffer attribute smoothing.
    for (auto iter = 1; iter < numIter; ++iter)
    {
      if ((iter % 2)) // odd
      {
        smoothAttr.Execute(numPts, &oddPair);
        smoothedPD = oddPD;
      }
      else // even
      {
        smoothAttr.Execute(numPts, &evenPair);
        smoothedPD = evenPD;
      }
    } // for all remaining iterations
  }   // if more than one iteration

  // After completion, shallow copy the point data to the output. Don't
  // forget to add back in the excluded arrays.
  outPD->PassData(smoothedPD);
  AddExcludedArrays(inPD, outPD, this->ExcludedArrays);

  return 1;
}

//------------------------------------------------------------------------------
void vtkAttributeSmoothingFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Number of Iterations: " << this->NumberOfIterations << "\n";
  os << indent << "Relaxation Factor: " << this->RelaxationFactor << "\n";
  os << indent << "Smoothing Strategy: " << this->SmoothingStrategy << "\n";
  os << indent << "Smoothing Mask: " << this->SmoothingMask.Get() << "\n";
  os << indent << "Weights Type: " << this->WeightsType << "\n";

  os << indent << "Number of Excluded Arrays:" << this->GetNumberOfExcludedArrays() << endl;
  vtkIndent nextIndent = indent.GetNextIndent();
  for (int i = 0; i < this->GetNumberOfExcludedArrays(); ++i)
  {
    os << nextIndent << "Excluded Array: " << this->ExcludedArrays[i] << endl;
  }
}
VTK_ABI_NAMESPACE_END
