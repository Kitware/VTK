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
#include "vtkSmartPointer.h"

#include <cstdlib>

vtkStandardNewMacro(vtkWarpVector);

//----------------------------------------------------------------------------
vtkWarpVector::vtkWarpVector()
{
  this->ScaleFactor = 1.0;

  // by default process active point vectors
  this->SetInputArrayToProcess(0,0,0,vtkDataObject::FIELD_ASSOCIATION_POINTS,
                               vtkDataSetAttributes::VECTORS);
}

//----------------------------------------------------------------------------
vtkWarpVector::~vtkWarpVector()
{
}

//----------------------------------------------------------------------------
int vtkWarpVector::FillInputPortInformation(int vtkNotUsed(port),
                                            vtkInformation *info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkRectilinearGrid");
  return 1;
}

//----------------------------------------------------------------------------
int vtkWarpVector::RequestDataObject(vtkInformation *request,
                                     vtkInformationVector **inputVector,
                                     vtkInformationVector *outputVector)
{
  vtkImageData *inImage = vtkImageData::GetData(inputVector[0]);
  vtkRectilinearGrid *inRect = vtkRectilinearGrid::GetData(inputVector[0]);

  if (inImage || inRect)
  {
    vtkStructuredGrid *output = vtkStructuredGrid::GetData(outputVector);
    if (!output)
    {
      vtkNew<vtkStructuredGrid> newOutput;
      outputVector->GetInformationObject(0)->Set(
        vtkDataObject::DATA_OBJECT(), newOutput.GetPointer());
    }
    return 1;
  }
  else
  {
    return this->Superclass::RequestDataObject(request,
                                               inputVector,
                                               outputVector);
  }
}

//----------------------------------------------------------------------------
namespace {
// Used by the WarpVectorDispatch1Vector worker, defined below:
template <typename VectorArrayT>
struct WarpVectorDispatch2Points
{
  vtkWarpVector *Self;
  VectorArrayT *Vectors;

  WarpVectorDispatch2Points(vtkWarpVector *self, VectorArrayT *vectors)
    : Self(self), Vectors(vectors)
  {}

  template <typename InPointArrayT, typename OutPointArrayT>
  void operator()(InPointArrayT *inPtArray, OutPointArrayT *outPtArray)
  {
    typedef typename OutPointArrayT::ValueType PointValueT;
    const vtkIdType numTuples = inPtArray->GetNumberOfTuples();
    const double scaleFactor = this->Self->GetScaleFactor();

    assert(this->Vectors->GetNumberOfComponents() == 3);
    assert(inPtArray->GetNumberOfComponents() == 3);
    assert(outPtArray->GetNumberOfComponents() == 3);

    for (vtkIdType t = 0; t < numTuples; ++t)
    {
      if (!(t & 0xfff))
      {
        this->Self->UpdateProgress(t / static_cast<double>(numTuples));
        if (this->Self->GetAbortExecute())
        {
          return;
        }
      }

      for (int c = 0; c < 3; ++c)
      {
        PointValueT val = inPtArray->GetTypedComponent(t, c) +
            scaleFactor * this->Vectors->GetTypedComponent(t, c);
        outPtArray->SetTypedComponent(t, c, val);
      }
    }
  }
};

// Dispatch just the vector array first, we can cut out some generated code
// since the point arrays will have the same type.
struct WarpVectorDispatch1Vector
{
  vtkWarpVector *Self;
  vtkDataArray *InPoints;
  vtkDataArray *OutPoints;

  WarpVectorDispatch1Vector(vtkWarpVector *self,
                            vtkDataArray *inPoints, vtkDataArray *outPoints)
    : Self(self), InPoints(inPoints), OutPoints(outPoints)
  {}

  template <typename VectorArrayT>
  void operator()(VectorArrayT *vectors)
  {
    WarpVectorDispatch2Points<VectorArrayT> worker(this->Self, vectors);
    if (!vtkArrayDispatch::Dispatch2SameValueType::Execute(
          this->InPoints, this->OutPoints, worker))
    {
      vtkGenericWarningMacro("Error dispatching point arrays.");
    }
  }
};
} // end anon namespace

//----------------------------------------------------------------------------
int vtkWarpVector::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  vtkSmartPointer<vtkPointSet> input = vtkPointSet::GetData(inputVector[0]);
  vtkPointSet *output = vtkPointSet::GetData(outputVector);

  if (!input)
  {
    // Try converting image data.
    vtkImageData *inImage = vtkImageData::GetData(inputVector[0]);
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
    vtkRectilinearGrid *inRect = vtkRectilinearGrid::GetData(inputVector[0]);
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

  vtkPoints *points;
  vtkIdType numPts;

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  if (input == NULL || input->GetPoints() == NULL)
  {
    return 1;
  }
  numPts = input->GetPoints()->GetNumberOfPoints();

  vtkDataArray *vectors = this->GetInputArrayToProcess(0,inputVector);

  if ( !vectors || !numPts)
  {
    vtkDebugMacro(<<"No input data");
    return 1;
  }

  // SETUP AND ALLOCATE THE OUTPUT
  numPts = input->GetNumberOfPoints();
  points = input->GetPoints()->NewInstance();
  points->SetDataType(input->GetPoints()->GetDataType());
  points->Allocate(numPts);
  points->SetNumberOfPoints(numPts);
  output->SetPoints(points);
  points->Delete();

  // call templated function.
  // We use two dispatches since we need to dispatch 3 arrays and two share a
  // value type. Implementating a second type-restricted dispatch reduces
  // the amount of generated templated code.
  WarpVectorDispatch1Vector worker(this, input->GetPoints()->GetData(),
                                   output->GetPoints()->GetData());
  if (!vtkArrayDispatch::Dispatch::Execute(vectors, worker))
  {
    vtkWarningMacro("Dispatch failed for vector array.");
  }

  // now pass the data.
  output->GetPointData()->CopyNormalsOff(); // distorted geometry
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  return 1;
}

//----------------------------------------------------------------------------
void vtkWarpVector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
}
