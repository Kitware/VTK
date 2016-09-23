/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWarpTo.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWarpTo.h"

#include "vtkImageData.h"
#include "vtkImageDataToPointSet.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkRectilinearGrid.h"
#include "vtkRectilinearGridToPointSet.h"
#include "vtkStructuredGrid.h"

#include "vtkNew.h"
#include "vtkSmartPointer.h"

vtkStandardNewMacro(vtkWarpTo);

vtkWarpTo::vtkWarpTo()
{
  this->ScaleFactor = 0.5;
  this->Absolute = 0;
  this->Position[0] = this->Position[1] = this->Position[2] = 0.0;
}

int vtkWarpTo::FillInputPortInformation(int vtkNotUsed(port),
                                        vtkInformation *info)
{
  info->Remove(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPointSet");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  info->Append(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkRectilinearGrid");
  return 1;
}

int vtkWarpTo::RequestDataObject(vtkInformation *request,
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

int vtkWarpTo::RequestData(
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

  vtkPoints *inPts;
  vtkPoints *newPts;
  vtkIdType ptId, numPts;
  int i;
  double x[3], newX[3];
  double mag;
  double minMag = 0;

  vtkDebugMacro(<<"Warping data to a point");

  // First, copy the input to the output as a starting point
  output->CopyStructure( input );

  inPts = input->GetPoints();

  if (!inPts )
  {
    vtkErrorMacro(<<"No input data");
    return 1;
  }

  numPts = inPts->GetNumberOfPoints();
  newPts = vtkPoints::New(); newPts->SetNumberOfPoints(numPts);

  if (this->Absolute)
  {
    minMag = 1.0e10;
    for (ptId=0; ptId < numPts; ptId++)
    {
      inPts->GetPoint(ptId, x);
      mag = sqrt(vtkMath::Distance2BetweenPoints(this->Position,x));
      if (mag < minMag)
      {
        minMag = mag;
      }
    }
  }

  //
  // Loop over all points, adjusting locations
  //
  for (ptId=0; ptId < numPts; ptId++)
  {
    inPts->GetPoint(ptId, x);
    if (this->Absolute)
    {
      mag = sqrt(vtkMath::Distance2BetweenPoints(this->Position,x));
      for (i=0; i<3; i++)
      {
        newX[i] = this->ScaleFactor*
          (this->Position[i] + minMag*(x[i] - this->Position[i])/mag) +
          (1.0 - this->ScaleFactor)*x[i];
      }
    }
    else
    {
      for (i=0; i<3; i++)
      {
        newX[i] = (1.0 - this->ScaleFactor)*x[i] +
          this->ScaleFactor*this->Position[i];
      }
    }
    newPts->SetPoint(ptId, newX);
  }
  //
  // Update ourselves and release memory
  //
  output->GetPointData()->CopyNormalsOff(); // distorted geometry
  output->GetPointData()->PassData(input->GetPointData());

  output->SetPoints(newPts);
  newPts->Delete();

  return 1;
}

void vtkWarpTo::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Absolute: " << (this->Absolute ? "On\n" : "Off\n");

  os << indent << "Position: (" << this->Position[0] << ", "
    << this->Position[1] << ", " << this->Position[2] << ")\n";
  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
}
