// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include "vtkConstrainedSmoothingFilter.h"

#include "vtkArrayDispatch.h"
#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkCellLocator.h"
#include "vtkDoubleArray.h"
#include "vtkExtractEdges.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkLogger.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkSMPThreadLocalObject.h"
#include "vtkStaticCellLinksTemplate.h"
#include "vtkStreamingDemandDrivenPipeline.h"

VTK_ABI_NAMESPACE_BEGIN
vtkStandardNewMacro(vtkConstrainedSmoothingFilter);
VTK_ABI_NAMESPACE_END

// The following code defines core methods for the
// vtkConstrainedSmoothingFilter class.
//
namespace
{ // anonymous

// Build a smoothing stencil from a VTK cell links object that is produced
// from a network of edges (which is typically generated via
// vtkExtractEdges). The stencil, for each point p, is a set of points ps
// connected to p. Here the cell links is used to create the set ps.
struct BuildStencil
{
  vtkCellArray* Lines;
  vtkStaticCellLinksTemplate<vtkIdType>* Links;
  vtkIdType* Offsets;
  vtkIdType* Conn;
  // Avoid constructing/deleting the iterator
  vtkSMPThreadLocalObject<vtkIdList> TLIdList;

  BuildStencil(vtkCellArray* lines, vtkStaticCellLinksTemplate<vtkIdType>* links,
    vtkIdType* offsets, vtkIdType* conn)
    : Lines(lines)
    , Links(links)
    , Offsets(offsets)
    , Conn(conn)
  {
  }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    auto& idList = this->TLIdList.Local();
    vtkStaticCellLinksTemplate<vtkIdType>* links = this->Links;
    vtkIdType npts;
    const vtkIdType* pts;
    vtkIdType* o = this->Offsets + ptId;
    vtkIdType* c = this->Conn + links->GetOffset(ptId);

    for (; ptId < endPtId; ++ptId)
    {
      vtkIdType numEdges = links->GetNumberOfCells(ptId);
      vtkIdType* edges = links->GetCells(ptId);
      for (auto i = 0; i < numEdges; ++i)
      {
        this->Lines->GetCellAtId(edges[i], npts, pts, idList);
        *c++ = (pts[0] != ptId ? pts[0] : pts[1]);
      }
      *o++ = links->GetOffset(ptId);
    }
  }
}; // BuildStencil

// Create stencils if none were provided. Leverage the vtkExtractEdges filter
// (which is threaded) to build the stencils.
vtkSmartPointer<vtkCellArray> BuildStencils(vtkPointSet* input)
{
  vtkNew<vtkCellArray> stencils;

  // Create edges from dataset which will be used to build the stencils.
  vtkNew<vtkExtractEdges> extract;
  extract->SetInputData(input);
  extract->UseAllPointsOn();
  extract->Update();
  vtkPolyData* output = extract->GetOutput();

  vtkIdType numPts = output->GetNumberOfPoints();
  vtkCellArray* lines = output->GetLines();
  vtkIdType numLines = lines->GetNumberOfCells();

  // Make sure there is something to process
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

  // Now point by point build the smoothing stencils
  BuildStencil buildStencil(lines, &links, offsetsPtr, connPtr);
  vtkSMPTools::For(0, numPts, buildStencil);

  // The stencils have been defined, put them in the form of a vtkCellArray and return.
  stencils->SetData(offsets, conn);
  return stencils;
} // BuildStencils

// This functor performs a single smoothing iteration over a set of points. All of
// the vtkPoints (the original input points, the output points, and a temporary
// set of points) are of the same type.
template <typename PT>
struct SmoothPoints
{
  PT* InPts;
  PT* OutPts;
  PT* TmpPts;
  vtkCellArray* Stencils;
  double Relax;
  double CDist;
  const double* CBox;
  double CDist2; // Temporary variable for smoothing distance
  const double* CArray;
  double MaxDistance; // used to determine convergence
  // Avoid constructing/deleting the iterator
  vtkSMPThreadLocalObject<vtkIdList> TLIdList;
  // Maximum smoothing distance in this thread
  vtkSMPThreadLocal<double> MaxDistance2;

  SmoothPoints(vtkCellArray* stencils, double relax, double cDist, double* cBox, double* cArray)
    : InPts(nullptr)
    , OutPts(nullptr)
    , TmpPts(nullptr)
    , Stencils(stencils)
    , Relax(relax)
    , CDist(cDist)
    , CBox(cBox)
    , CArray(cArray)
  {
    this->CDist2 = this->CDist * this->CDist;

    if (cBox)
    {
      // Make sure constraint box is valid
      if (cBox[0] <= 0 || cBox[1] <= 0 || cBox[2] <= 0)
      {
        this->CDist2 = 0;
      }
      else
      {
        this->CDist2 = vtkMath::Norm(cBox) / 2.0; // anying >0 will do
      }
    }
  }

  // Should be set before each iteration
  void SetSmoothingArrays(PT* inPts, PT* outPts, PT* tmpPts)
  {
    this->InPts = inPts;
    this->OutPts = outPts;
    this->TmpPts = tmpPts;
  }

  // Returns either the filter's constraint distance, or the value from
  // the constraint array.
  double GetConstraint2(vtkIdType ptId)
  {
    return (this->CArray ? (this->CArray[ptId] * this->CArray[ptId]) : this->CDist2);
  }

  void Initialize() { this->MaxDistance2.Local() = 0.0; }

  void operator()(vtkIdType ptId, vtkIdType endPtId)
  {
    auto& idList = this->TLIdList.Local();
    double& maxDistance2 = this->MaxDistance2.Local();

    const auto inPts = vtk::DataArrayTupleRange<3>(this->InPts);
    auto outPts = vtk::DataArrayTupleRange<3>(this->OutPts);
    const auto tmpPts = vtk::DataArrayTupleRange<3>(this->TmpPts);

    vtkIdType npts;
    const vtkIdType* pts;
    double relax = this->Relax;
    const double* cBox = this->CBox;

    for (; ptId < endPtId; ++ptId)
    {
      // Get the original point position and the stencil
      const auto xIn = inPts[ptId];
      this->Stencils->GetCellAtId(ptId, npts, pts, idList);

      // For each point in the stencil, compute an average position.
      // Make sure the stencil is valid (i.e., contains points).
      auto xOut = outPts[ptId];
      double constraint2 = this->GetConstraint2(ptId);
      if (npts < 1 || constraint2 == 0.0) // empty stencil, or totally constrained
      {
        xOut[0] = xIn[0];
        xOut[1] = xIn[1];
        xOut[2] = xIn[2];
        continue; // skip the rest
      }

      // We have a valid stencil, average stencil contributions.
      double xAve[3] = { 0, 0, 0 };
      for (auto i = 0; i < npts; ++i)
      {
        const auto xTmp = tmpPts[pts[i]];
        xAve[0] += xTmp[0];
        xAve[1] += xTmp[1];
        xAve[2] += xTmp[2];
      }
      xAve[0] /= static_cast<double>(npts);
      xAve[1] /= static_cast<double>(npts);
      xAve[2] /= static_cast<double>(npts);

      double x[3];
      auto xTmp = tmpPts[ptId];
      x[0] = xTmp[0] + relax * (xAve[0] - xTmp[0]);
      x[1] = xTmp[1] + relax * (xAve[1] - xTmp[1]);
      x[2] = xTmp[2] + relax * (xAve[2] - xTmp[2]);

      // Constrain the point movement.
      if (cBox) // Constrain with constraint box
      {
        double xC[3];
        xC[0] = xIn[0];
        xC[1] = xIn[1];
        xC[2] = xIn[2];
        double t, xInt[3];
        int plane;
        if (!vtkBoundingBox::ContainsLine(xC, cBox, x, t, xInt, plane))
        {
          x[0] = xInt[0];
          x[1] = xInt[1];
          x[2] = xInt[2];
        }
      }
      else // Constrain with constraint distance/sphere or constraint array
      {
        double d2 = vtkMath::Distance2BetweenPoints(x, xIn);
        if (d2 > constraint2)
        { // clamp smoothing position
          double t = sqrt(constraint2 / d2);
          x[0] = xIn[0] + t * (x[0] - xIn[0]);
          x[1] = xIn[1] + t * (x[1] - xIn[1]);
          x[2] = xIn[2] + t * (x[2] - xIn[2]);
        }
      }

      // Track convergence
      double maxD2 = vtkMath::Distance2BetweenPoints(x, xTmp);
      maxDistance2 = (maxD2 > maxDistance2 ? maxD2 : maxDistance2);

      // Update new points
      xOut[0] = x[0];
      xOut[1] = x[1];
      xOut[2] = x[2];
    } // over all points
  }

  void Reduce()
  {
    // Roll up the maximum distance a point has moved.
    double maxDistance2 = 0.0;
    for (const auto& localMaxDistance : this->MaxDistance2)
    {
      maxDistance2 = (localMaxDistance > maxDistance2 ? localMaxDistance : maxDistance2);
    } // over all threads
    this->MaxDistance = sqrt(maxDistance2);
  }

}; // SmoothPoints

// Dispatch infrastructure for point smoothing. Basically, multiple smoothing iterations
// are invoked until convergence occurs or the maximum number of iterations is reached.
// The process uses a double-buffering approach so as to enable threaded smoothing. Note
// that the type of points: original, output points, and temporary points, is the same.
// This reduces the number of template parameters and reduces template bloat.
struct SmoothWorker
{
  template <typename PT>
  void operator()(PT* inPtsArray, vtkPoints* outPts, vtkPoints* tmpPts, vtkCellArray* stencils,
    double converge, int numIter, double relax, double cDist, double* cBox, double* cArray)
  {
    // Set up the smoother
    vtkIdType numPts = inPtsArray->GetNumberOfTuples();
    PT* outPtsArray = static_cast<PT*>(outPts->GetData());
    PT* tmpPtsArray = static_cast<PT*>(tmpPts->GetData());
    PT* swapPtsArray;

    // Setup the functor that does the smoothing
    SmoothPoints<PT> smooth(stencils, relax, cDist, cBox, cArray);

    // The first iteration uses the input points as the temporary points
    // to avoid making an initial copy.
    double maxDistance = VTK_FLOAT_MAX;
    int iterNum = 0;
    smooth.SetSmoothingArrays(inPtsArray, outPtsArray, inPtsArray);

    while (iterNum < numIter && maxDistance > converge)
    {
      // Parallel smooth
      vtkSMPTools::For(0, numPts, smooth);
      maxDistance = smooth.MaxDistance;

      // Double-buffer the smoothing operation. This is needed
      // to avoid race conditions.
      swapPtsArray = tmpPtsArray;
      tmpPtsArray = outPtsArray;
      outPtsArray = swapPtsArray;

      smooth.SetSmoothingArrays(inPtsArray, outPtsArray, tmpPtsArray);
      iterNum++;

    } // while still iterating

    // Now replace the output's points array with the final
    // iteration. Because a swap of arrays has already occurred, we use the
    // most recent array.
    outPts->SetData(tmpPtsArray);

  } // operator()

}; // SmoothWorker

// Generate error scalars and/or vectors.
struct AttrWorker
{
  template <typename PT>
  void operator()(
    PT* inPtsArray, vtkPoints* outputPts, vtkPointSet* output, bool errorScalars, bool errorVectors)
  {
    vtkIdType numPts = inPtsArray->GetNumberOfTuples();
    PT* outPtsArray = static_cast<PT*>(outputPts->GetData());
    int dataType = outPtsArray->GetDataType();

    vtkSmartPointer<vtkDataArray> scalars;
    vtkSmartPointer<vtkDataArray> vectors;
    if (errorScalars)
    {
      scalars.TakeReference(vtkDataArray::CreateDataArray(dataType));
      scalars->SetNumberOfTuples(numPts);
      scalars->SetName("SmoothingErrorScalars");
      output->GetPointData()->AddArray(scalars);
    }

    if (errorVectors)
    {
      vectors.TakeReference(vtkDataArray::CreateDataArray(dataType));
      vectors->SetNumberOfComponents(3);
      vectors->SetNumberOfTuples(numPts);
      vectors->SetName("SmoothingErrorVectors");
      output->GetPointData()->AddArray(vectors);
    }

    // In place lambda to compute error scalars and vectors
    vtkSMPTools::For(0, numPts,
      [&inPtsArray, &outPtsArray, &scalars, &vectors](vtkIdType ptId, vtkIdType endPtId)
      {
        const auto inPts = vtk::DataArrayTupleRange<3>(inPtsArray);
        const auto outPts = vtk::DataArrayTupleRange<3>(outPtsArray);

        for (; ptId < endPtId; ++ptId)
        {
          const auto xIn = inPts[ptId];
          const auto xOut = outPts[ptId];

          double v[3];
          v[0] = xOut[0] - xIn[0];
          v[1] = xOut[1] - xIn[1];
          v[2] = xOut[2] - xIn[2];

          if (scalars)
          {
            scalars->SetTuple1(ptId, vtkMath::Norm(v));
          }

          if (vectors)
          {
            vectors->SetTuple(ptId, v);
          }
        }
      });
  } // operator()

}; // AttrWorker

} // anonymous namespace

VTK_ABI_NAMESPACE_BEGIN
//=================Begin VTK class proper=======================================
//------------------------------------------------------------------------------
vtkConstrainedSmoothingFilter::vtkConstrainedSmoothingFilter()
{
  this->Convergence = 0.0;
  this->NumberOfIterations = 10;
  this->RelaxationFactor = 0.01;

  this->ConstraintStrategy = DEFAULT;
  this->ConstraintDistance = 0.001;
  this->ConstraintBox[0] = 1;
  this->ConstraintBox[1] = 1;
  this->ConstraintBox[2] = 1;

  this->GenerateErrorScalars = false;
  this->GenerateErrorVectors = false;
  this->OutputPointsPrecision = vtkAlgorithm::DEFAULT_PRECISION;
}

//------------------------------------------------------------------------------
int vtkConstrainedSmoothingFilter::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  // get the info objects
  vtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkPointSet* input = vtkPointSet::SafeDownCast(inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkPointSet* output = vtkPointSet::SafeDownCast(outInfo->Get(vtkDataObject::DATA_OBJECT()));

  vtkLog(TRACE, "Executing constrained smoothing filter");

  // Sanity check the input
  vtkIdType numPts = input->GetNumberOfPoints();
  if (numPts < 1)
  {
    return 1;
  }

  // Pass everything through, the points will be updated later
  vtkPointData *inPD = input->GetPointData(), *outPD = output->GetPointData();
  vtkCellData *inCD = input->GetCellData(), *outCD = output->GetCellData();
  output->CopyStructure(input);
  outPD->PassData(inPD);
  outCD->PassData(inCD);

  // Make sure these is work to do.
  if (this->NumberOfIterations < 1)
  {
    return 1;
  }

  // Create some new points of the proper precision, and a temporary array
  // for double buffering the smoothing process.
  vtkPoints* inPts = input->GetPoints();
  vtkNew<vtkPoints> newPts;
  vtkNew<vtkPoints> tmpPts;
  if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    if (inPts->GetDataType() == VTK_FLOAT || inPts->GetDataType() == VTK_DOUBLE)
    {
      newPts->SetDataType(inPts->GetDataType());
    }
    else
    {
      newPts->SetDataType(VTK_FLOAT);
    }
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    newPts->SetDataType(VTK_FLOAT);
  }
  else // if (this->OutputPointsPrecision == vtkAlgorithm::DOUBLE_PRECISION)
  {
    newPts->SetDataType(VTK_DOUBLE);
  }
  newPts->SetNumberOfPoints(numPts);
  output->SetPoints(newPts);

  // The temporary, scratch points are always the same type ad the output points.
  tmpPts->SetDataType(newPts->GetDataType());
  tmpPts->SetNumberOfPoints(numPts);

  // If the type of the input points is the same as the output points, we can
  // avoid an initial copy of the points prior to smoothing. (The smoothing
  // worker is templated on a single points type.) This also simplifies the
  // dispatch, and reduces template bloat. Most of the time, the input and
  // output point types are the same.
  vtkSmartPointer<vtkPoints> copyPts;
  if (inPts->GetDataType() != newPts->GetDataType())
  {
    copyPts = vtkSmartPointer<vtkPoints>::New();
    vtkSmartPointer<vtkDataArray> copyArray = vtkDataArray::CreateDataArray(newPts->GetDataType());
    copyPts->SetData(copyArray);
    copyArray->DeepCopy(inPts->GetData()); // cast type and copy
    inPts = copyPts.Get();                 // replace the current input points with the copy
  }

  // Extract a constraint array, if any, from the input point data.
  double cDist = this->ConstraintDistance;
  double* cBox = (this->ConstraintStrategy == CONSTRAINT_BOX ? this->ConstraintBox : nullptr);
  vtkDoubleArray* constraints =
    vtkDoubleArray::SafeDownCast(inPD->GetArray("SmoothingConstraints"));
  double* cArray = (constraints ? constraints->GetPointer(0) : nullptr);
  if (this->ConstraintStrategy == DEFAULT)
  {
    // preference is constraint array
  }
  else if (this->ConstraintStrategy == CONSTRAINT_DISTANCE ||
    this->ConstraintStrategy == CONSTRAINT_BOX)
  {
    cArray = nullptr; // force using constraint distance or constraint box
  }
  else if (this->ConstraintStrategy == CONSTRAINT_ARRAY && !cArray)
  {
    cDist = VTK_FLOAT_MAX; // effectively no constraints
    vtkLog(WARNING, "Constraint array not found, smoothing unconstrained");
  }

  // Define a smoothing stencil, or use what's provided.
  vtkSmartPointer<vtkCellArray> stencils = this->SmoothingStencils;
  if (!stencils)
  {
    stencils = BuildStencils(input);
  }

  // With the stencil defined, perform the smoothing. Use a double buffering
  // approach: smooth over point array #1 using the point array #0; then swap
  // the arrays.
  double converge = this->Convergence;
  int numIter = this->NumberOfIterations;
  double relax = this->RelaxationFactor;

  // Now smooth the points.
  using vtkArrayDispatch::Reals;
  using SmoothingDispatch = vtkArrayDispatch::DispatchByValueType<Reals>;
  SmoothWorker smoothWorker;
  if (!SmoothingDispatch::Execute(inPts->GetData(), smoothWorker, newPts, tmpPts, stencils,
        converge, numIter, relax, cDist, cBox, cArray))
  { // Fallback to slowpath for other point types
    smoothWorker(
      inPts->GetData(), newPts, tmpPts, stencils, converge, numIter, relax, cDist, cBox, cArray);
  }

  // If error scalars or vectors are requested, compute these.
  if (this->GenerateErrorScalars || this->GenerateErrorVectors)
  {
    using AttrDispatch = vtkArrayDispatch::DispatchByValueType<Reals>;
    AttrWorker attrWorker;
    if (!AttrDispatch::Execute(inPts->GetData(), attrWorker, newPts, output,
          this->GenerateErrorScalars, this->GenerateErrorVectors))
    { // Fallback to slowpath for other point types
      attrWorker(
        inPts->GetData(), newPts, output, this->GenerateErrorScalars, this->GenerateErrorVectors);
    }
  }

  return 1;
}

//------------------------------------------------------------------------------
void vtkConstrainedSmoothingFilter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Convergence: " << this->Convergence << "\n";
  os << indent << "Number of Iterations: " << this->NumberOfIterations << "\n";
  os << indent << "Relaxation Factor: " << this->RelaxationFactor << "\n";

  os << indent << "Constraint Strategy: " << this->ConstraintStrategy << "\n";
  os << indent << "Constraint Distance: " << this->ConstraintDistance << "\n";
  os << indent << "Constraint Box: (" << this->ConstraintBox[0] << ", " << this->ConstraintBox[1]
     << ", " << this->ConstraintBox[2] << ")\n";
  os << indent << "Smoothing Stencils: " << this->SmoothingStencils.Get() << "\n";

  os << indent << "Generate Error Scalars: " << (this->GenerateErrorScalars ? "On\n" : "Off\n");
  os << indent << "Generate Error Vectors: " << (this->GenerateErrorVectors ? "On\n" : "Off\n");
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
VTK_ABI_NAMESPACE_END
