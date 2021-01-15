/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpVector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWarpVector.h"

#include "vtkArrayDispatch.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkDataArrayRange.h"
#include "vtkImageData.h"
#include "vtkImageDataToPointSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridToPointSet.h"
#include "vtkStructuredGrid.h"

#include "vtkNew.h"
#include "vtkSMPTools.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkWarpVector);

//------------------------------------------------------------------------------
vtkWarpVector::vtkWarpVector()
{
  this->ScaleFactor = 1.0;
  this->OutputPointsPrecision = vtkAlgorithm::DEFAULT_PRECISION;

  // by default process active point vectors
  this->SetInputArrayToProcess(
    0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_POINTS, vtkDataSetAttributes::VECTORS);
}

//------------------------------------------------------------------------------
vtkWarpVector::~vtkWarpVector() = default;

//------------------------------------------------------------------------------
int vtkWarpVector::FillInputPortInformation(int vtkNotUsed(port), vtkInformation* info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkRectilinearGrid");
  return 1;
}

//------------------------------------------------------------------------------
int vtkWarpVector::RequestDataObject(
  vtkInformation* request, vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkImageData* inImage = vtkImageData::GetData(inputVector[0]);
  vtkRectilinearGrid* inRect = vtkRectilinearGrid::GetData(inputVector[0]);

  if (inImage || inRect)
  {
    vtkStructuredGrid* output = vtkStructuredGrid::GetData(outputVector);
    if (!output)
    {
      vtkNew<vtkStructuredGrid> newOutput;
      outputVector->GetInformationObject(0)->Set(vtkDataObject::DATA_OBJECT(), newOutput);
    }
    return 1;
  }
  else
  {
    return this->Superclass::RequestDataObject(request, inputVector, outputVector);
  }
}

//------------------------------------------------------------------------------
// Core methods to scale points with vectors
namespace
{ // anonymous

struct WarpWorker
{
  template <typename InPT, typename OutPT, typename VT>
  void operator()(InPT* inPts, OutPT* outPts, VT* vectors, vtkWarpVector* self, double sf)

  {
    vtkIdType numPts = inPts->GetNumberOfTuples();
    const auto ipts = vtk::DataArrayTupleRange<3>(inPts);
    auto opts = vtk::DataArrayTupleRange<3>(outPts);
    const auto vecs = vtk::DataArrayTupleRange<3>(vectors);

    // For smaller data sizes, serial processing is faster than spinning up
    // threads. The cutoff point between serial and threaded is empirical and
    // is likely to change.
    static constexpr int VTK_SMP_THRESHOLD = 1000000;
    if (numPts >= VTK_SMP_THRESHOLD)
    {
      vtkSMPTools::For(0, numPts, [&](vtkIdType ptId, vtkIdType endPtId) {
        for (; ptId < endPtId; ++ptId)
        {
          const auto xi = ipts[ptId];
          auto xo = opts[ptId];
          const auto v = vecs[ptId];

          xo[0] = xi[0] + sf * v[0];
          xo[1] = xi[1] + sf * v[1];
          xo[2] = xi[2] + sf * v[2];
        }
      }); // lambda
    }     // threaded

    else // serial
    {
      for (vtkIdType ptId = 0; ptId < numPts; ptId++)
      {
        if (!(ptId % 10000))
        {
          self->UpdateProgress((double)ptId / numPts);
          if (self->GetAbortExecute())
          {
            break;
          }
        }

        const auto xi = ipts[ptId];
        auto xo = opts[ptId];
        const auto v = vecs[ptId];

        xo[0] = xi[0] + sf * v[0];
        xo[1] = xi[1] + sf * v[1];
        xo[2] = xi[2] + sf * v[2];

      } // over all points
    }   // serial processing
  }
};

} // anonymous namespace

//------------------------------------------------------------------------------
int vtkWarpVector::RequestData(vtkInformation* vtkNotUsed(request),
  vtkInformationVector** inputVector, vtkInformationVector* outputVector)
{
  vtkSmartPointer<vtkPointSet> input = vtkPointSet::GetData(inputVector[0]);
  vtkPointSet* output = vtkPointSet::GetData(outputVector);

  if (!input)
  {
    // Try converting image data.
    vtkImageData* inImage = vtkImageData::GetData(inputVector[0]);
    if (inImage)
    {
      vtkNew<vtkImageDataToPointSet> image2points;
      image2points->SetInputData(inImage);
      image2points->Update();
      input = image2points->GetOutput();
    }
  }

  if (!input)
  {
    // Try converting rectilinear grid.
    vtkRectilinearGrid* inRect = vtkRectilinearGrid::GetData(inputVector[0]);
    if (inRect)
    {
      vtkNew<vtkRectilinearGridToPointSet> rect2points;
      rect2points->SetInputData(inRect);
      rect2points->Update();
      input = rect2points->GetOutput();
    }
  }

  if (!input)
  {
    vtkErrorMacro(<< "Invalid or missing input");
    return 0;
  }

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  vtkPoints* inPts;
  if (input == nullptr || (inPts = input->GetPoints()) == nullptr)
  {
    return 1;
  }
  vtkIdType numPts = inPts->GetNumberOfPoints();
  vtkDataArray* vectors = this->GetInputArrayToProcess(0, inputVector);

  if (!vectors || !numPts)
  {
    vtkDebugMacro(<< "No input data");
    return 1;
  }

  // Create the output points. By default, the output type is the
  // same as the input type.
  vtkNew<vtkPoints> newPts;
  if (this->OutputPointsPrecision == vtkAlgorithm::DEFAULT_PRECISION)
  {
    newPts->SetDataType(inPts->GetDataType());
  }
  else if (this->OutputPointsPrecision == vtkAlgorithm::SINGLE_PRECISION)
  {
    newPts->SetDataType(VTK_FLOAT);
  }
  else
  {
    newPts->SetDataType(VTK_DOUBLE);
  }
  newPts->SetNumberOfPoints(numPts);
  output->SetPoints(newPts);

  assert(vectors->GetNumberOfComponents() == 3);
  assert(inPts->GetData()->GetNumberOfComponents() == 3);
  assert(newPts->GetData()->GetNumberOfComponents() == 3);

  // Dispatch over point and scalar types. Fastpath for real types, fallback to slower
  // path for non-real types.
  using vtkArrayDispatch::Reals;
  using WarpDispatch = vtkArrayDispatch::Dispatch3ByValueType<Reals, Reals, Reals>;
  WarpWorker warpWorker;

  if (!WarpDispatch::Execute(
        inPts->GetData(), newPts->GetData(), vectors, warpWorker, this, this->ScaleFactor))
  { // fallback to slowpath
    warpWorker(inPts->GetData(), newPts->GetData(), vectors, this, this->ScaleFactor);
  }

  // now pass the data.
  output->GetPointData()->CopyNormalsOff(); // distorted geometry
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  return 1;
}

//------------------------------------------------------------------------------
void vtkWarpVector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
