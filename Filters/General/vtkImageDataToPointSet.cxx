/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRectilinearGridToTetrahedra.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
#include "vtkImageDataToPointSet.h"

#include "vtkCellData.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkStructuredGrid.h"

#include "vtkNew.h"

vtkStandardNewMacro(vtkImageDataToPointSet);

//-------------------------------------------------------------------------
vtkImageDataToPointSet::vtkImageDataToPointSet()
{
}

vtkImageDataToPointSet::~vtkImageDataToPointSet()
{
}

void vtkImageDataToPointSet::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-------------------------------------------------------------------------
int vtkImageDataToPointSet::FillInputPortInformation(int port,
                                                     vtkInformation *info)
{
  if (!this->Superclass::FillInputPortInformation(port, info))
  {
    return 0;
  }
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkImageData");
  return 1;
}

//-------------------------------------------------------------------------
int vtkImageDataToPointSet::CopyStructure(vtkStructuredGrid *outData,
                                          vtkImageData *inData)
{
  double origin[3];
  double spacing[3];

  inData->GetOrigin(origin);
  inData->GetSpacing(spacing);

  int extent[6];
  inData->GetExtent(extent);

  outData->SetExtent(extent);

  vtkNew<vtkPoints> points;
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(inData->GetNumberOfPoints());

  vtkIdType pointId = 0;
  int ijk[3];
  for (ijk[2] = extent[4]; ijk[2] <= extent[5]; ijk[2]++)
  {
    for (ijk[1] = extent[2]; ijk[1] <= extent[3]; ijk[1]++)
    {
      for (ijk[0] = extent[0]; ijk[0] <= extent[1]; ijk[0]++)
      {
        double coord[3];

        for (int axis = 0; axis < 3; axis++)
        {
          coord[axis] = origin[axis] + spacing[axis]*ijk[axis];
        }

        points->SetPoint(pointId, coord);
        pointId++;
      }
    }
  }

  if (pointId != points->GetNumberOfPoints())
  {
    vtkErrorMacro(<< "Somehow misscounted points");
    return 0;
  }

  outData->SetPoints(points.GetPointer());

  return 1;
}

//-------------------------------------------------------------------------
int vtkImageDataToPointSet::RequestData(vtkInformation *vtkNotUsed(request),
                                        vtkInformationVector **inputVector,
                                        vtkInformationVector *outputVector)
{
  vtkImageData *inData = vtkImageData::GetData(inputVector[0]);
  vtkStructuredGrid *outData = vtkStructuredGrid::GetData(outputVector);

  if (inData == NULL)
  {
    vtkErrorMacro(<< "Input data is NULL.");
    return 0;
  }
  if (outData == NULL)
  {
    vtkErrorMacro(<< "Output data is NULL.");
    return 0;
  }

  int result = vtkImageDataToPointSet::CopyStructure(outData, inData);
  if (!result)
  {
    return 0;
  }

  outData->GetPointData()->PassData(inData->GetPointData());
  outData->GetCellData()->PassData(inData->GetCellData());

  return 1;
}
