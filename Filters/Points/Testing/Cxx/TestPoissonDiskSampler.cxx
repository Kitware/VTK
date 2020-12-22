/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPoissonDiskSampler.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkIdList.h"
#include "vtkKdTreePointLocator.h"
#include "vtkNew.h"
#include "vtkPointSet.h"
#include "vtkPoints.h"
#include "vtkPoissonDiskSampler.h"
#include "vtkSphereSource.h"

//------------------------------------------------------------------------------
int TestPoissonDiskSampler(int, char*[])
{
  double radius = 0.05;

  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(200);
  sphere->SetPhiResolution(100);
  sphere->SetRadius(1.0);

  vtkNew<vtkPoissonDiskSampler> sampler;
  sampler->SetInputConnection(sphere->GetOutputPort());
  sampler->SetRadius(radius);
  sampler->Update();

  vtkPointSet* output = vtkPointSet::SafeDownCast(sampler->GetOutputDataObject(0));

  vtkNew<vtkKdTreePointLocator> locator;
  locator->SetDataSet(output);
  locator->BuildLocator();

  vtkPoints* points = output->GetPoints();
  vtkNew<vtkIdList> ids;

  for (vtkIdType pointId = 0; pointId < output->GetNumberOfPoints(); ++pointId)
  {
    locator->FindPointsWithinRadius(radius, points->GetPoint(pointId), ids);
    if (ids->GetNumberOfIds() > 1)
    {
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}
