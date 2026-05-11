/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGoldenBallSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.

  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include <vtkDataObject.h>
#include <vtkGoldenBallSource.h>
#include <vtkNew.h>
#include <vtkTestUtilities.h>

int TestGoldenBallSource(int argc, char* argv[])
{
  vtkNew<vtkGoldenBallSource> ballSource;
  ballSource->SetResolution(200);
  ballSource->SetRadius(5.0);
  ballSource->GenerateNormalsOn();
  ballSource->IncludeCenterPointOn();
  ballSource->Update();

  return vtkTestUtilities::RegressionTest(
           argc, argv, ballSource->GetOutputDataObject(0), "/Data/goldenBallBaseline200.vtkhdf")
    ? EXIT_SUCCESS
    : EXIT_FAILURE;
}
