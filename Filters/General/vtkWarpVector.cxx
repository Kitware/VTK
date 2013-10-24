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

#include "vtkCellData.h"
#include "vtkDataArrayIteratorMacro.h"
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
template <class InputIterator, class OutputType, class VectorIterator>
void vtkWarpVectorExecute2(vtkWarpVector *self,
                           InputIterator begin, InputIterator end,
                           OutputType *outPts, VectorIterator inVec)
{
  OutputType scaleFactor = static_cast<OutputType>(self->GetScaleFactor());

  // Loop over all points, adjusting locations
  vtkIdType counter = 0;
  vtkIdType numPts = static_cast<vtkIdType>(end - begin);
  while (begin != end)
    {
    if (!(counter & 0xfff))
      {
      self->UpdateProgress(static_cast<double>(counter) /
                           static_cast<double>(numPts+1));
      if (self->GetAbortExecute())
        {
        break;
        }
      }

    *outPts++ = *begin++ + scaleFactor * static_cast<OutputType>(*inVec++);
    *outPts++ = *begin++ + scaleFactor * static_cast<OutputType>(*inVec++);
    *outPts++ = *begin++ + scaleFactor * static_cast<OutputType>(*inVec++);
    }
}

//----------------------------------------------------------------------------
template <class InputIterator, class OutputType>
void vtkWarpVectorExecute(vtkWarpVector *self,
                          InputIterator begin,
                          InputIterator end,
                          OutputType *outPts,
                          vtkDataArray *vectors)
{
  // call templated function
  switch (vectors->GetDataType())
    {
    vtkDataArrayIteratorMacro(vectors,
      vtkWarpVectorExecute2(self, begin, end, outPts, vtkDABegin));
    default:
      break;
    }
}

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

  // We know that this array has a standard memory layout, as we just created
  // it above.
  void *outPtr = output->GetPoints()->GetVoidPointer(0);

  // call templated function
  switch (input->GetPoints()->GetDataType())
    {
    vtkDataArrayIteratorMacro(input->GetPoints()->GetData(),
      vtkWarpVectorExecute(this, vtkDABegin, vtkDAEnd,
                           static_cast<vtkDAValueType*>(outPtr), vectors));
    default:
      break;
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
