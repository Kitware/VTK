/*=========================================================================

  Program:   Visualization Toolkit
  Module:    DistributedData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Test of vtkProjectSphereFilter. It checks the output in here
// and doesn't compare to an image.

#include "vtkArrayCalculator.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkNew.h"
#include "vtkPointData.h"
#include "vtkPointDataToCellData.h"
#include "vtkProjectSphereFilter.h"
#include "vtkSphereSource.h"
#include "vtkTestUtilities.h"

namespace
{
  // returns true for success. type is Point or Cell to give
  // feedback for errors in the passed in array.
  bool CheckFieldData(const char* type, vtkDataArray* array, int component,
                      double minValue, double maxValue)
  {
    double values[3];
    for(vtkIdType i=0;i<array->GetNumberOfTuples();i++)
      {
      array->GetTuple(i, values);
      for(int j=0;j<3;j++)
        {
        if(j == component)
          {
          if(values[j] < minValue || values[j] > maxValue)
            {
            vtkGenericWarningMacro("Array type " << type << " with name "
                                   << array->GetName() << " has bad value of " << values[j]
                                   << " but should be between " << minValue << " and " << maxValue);
            return false;
            }
          }
        else if(values[j] < -0.001 || values[j] > 0.001)
          {
          vtkGenericWarningMacro("Array type " << type << " with name " << array->GetName()
                                 << " should be 0 but has value of " << values[j]);
          return false;
          }
        }
      }
    return true;
  }
}

int TestProjectSphereFilter(int vtkNotUsed(argc), char* [])
{
  int numberOfErrors = 0;

  vtkNew<vtkSphereSource> sphere;
  sphere->SetRadius(1);
  sphere->SetCenter(0, 0, 0);
  sphere->SetThetaResolution(50);
  sphere->SetPhiResolution(50);

  vtkNew<vtkArrayCalculator> calculator;
  calculator->SetInputConnection(sphere->GetOutputPort());
  calculator->SetResultArrayName("result");
  calculator->SetFunction("-coordsY*iHat/sqrt(coordsY^2+coordsX^2)+coordsX*jHat/sqrt(coordsY^2+coordsX^2)");
  calculator->SetAttributeModeToUsePointData();
  calculator->AddCoordinateScalarVariable("coordsX", 0);
  calculator->AddCoordinateScalarVariable("coordsY", 1);

  vtkNew<vtkProjectSphereFilter> projectSphere;
  projectSphere->SetCenter(0, 0, 0);
  projectSphere->SetInputConnection(calculator->GetOutputPort());

  vtkNew<vtkPointDataToCellData> pointToCell;
  pointToCell->SetInputConnection(projectSphere->GetOutputPort());
  pointToCell->PassPointDataOn();

  pointToCell->Update();

  vtkDataSet* grid = pointToCell->GetOutput();
  if(grid->GetNumberOfPoints() != 2450)
    {
    vtkGenericWarningMacro(
      "Wrong number of points. There are " << grid->GetNumberOfPoints() <<
      " but should be 2450.");
    numberOfErrors++;
    }
  if(grid->GetNumberOfCells() != 4700)
    {
    vtkGenericWarningMacro(
      "Wrong number of cells. There are " << grid->GetNumberOfCells() <<
      " but should be 4700.");
    numberOfErrors++;
    }

  if(CheckFieldData("Point", grid->GetPointData()->GetArray("result"), 0, .99, 1.01)
     == false)
    {
    numberOfErrors++;
    }

  if(CheckFieldData("Point", grid->GetPointData()->GetArray("Normals"), 2, .99, 1.01)
     == false)
    {
    numberOfErrors++;
    }

  if(CheckFieldData("Cell", grid->GetCellData()->GetArray("Normals"), 2, .99, 1.01)
     == false)
    {
    numberOfErrors++;
    }

  return numberOfErrors;
}
