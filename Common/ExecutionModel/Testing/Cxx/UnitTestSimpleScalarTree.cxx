/*=========================================================================

  Program:   Visualization Toolkit
  Module:    UnitTestSimpleScalarTree

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkSmartPointer.h"
#include "vtkSimpleScalarTree.h"
#include "vtkImageData.h"
#include "vtkDataArray.h"
#include "vtkFloatArray.h"
#include "vtkSphereSource.h"
#include "vtkIdList.h"
#include "vtkTestErrorObserver.h"

#include <sstream>

static vtkSmartPointer<vtkImageData> MakeImage(int, int, int);

#define CHECK_ERROR_MSG(errorObserver, msg, status)      \
  { \
  std::string expectedMsg(msg); \
  if (!errorObserver->GetError()) \
  { \
    std::cout << "Failed to catch any error. Expected the error message to contain \"" << expectedMsg << std::endl; \
    status++; \
  } \
  else \
  { \
    std::string gotMsg(errorObserver->GetErrorMessage()); \
    if (gotMsg.find(expectedMsg) == std::string::npos) \
    { \
      std::cout << "Error message does not contain \"" << expectedMsg << "\" got \n\"" << gotMsg << std::endl; \
      status++; \
    } \
  } \
  } \
  errorObserver->Clear()

int UnitTestSimpleScalarTree(int, char*[])
{
  int status = 0;

  vtkSmartPointer<vtkTest::ErrorObserver>  errorObserver =
    vtkSmartPointer<vtkTest::ErrorObserver>::New();

  std::cout << "Testing empty Print...";
  vtkSmartPointer<vtkSimpleScalarTree> stree =
    vtkSmartPointer<vtkSimpleScalarTree>::New();
  std::ostringstream streePrint;
  stree->Print(streePrint);
  std::cout << "Passed" << std::endl;

  std::cout << "Testing no data error...";
  int status1 = 0;
  stree->AddObserver(vtkCommand::ErrorEvent, errorObserver);
  stree->BuildTree();
  CHECK_ERROR_MSG(errorObserver, "No data to build tree with", status1);
  if (status1)
  {
    status++;
    std::cout << "Failed" << std::endl;
  }
  else
  {
    std::cout << "Passed" << std::endl;
  }

  std::cout << "Testing no scalar data error...";
  int status2 = 0;
  vtkSmartPointer<vtkSphereSource> aSphere =
    vtkSmartPointer<vtkSphereSource>::New();
  aSphere->Update();
  stree->SetDataSet(aSphere->GetOutput());
  stree->BuildTree();
  CHECK_ERROR_MSG(errorObserver, "No scalar data to build trees with", status2);
  if (status2)
  {
    status++;
    std::cout << "Failed" << std::endl;
  }
  else
  {
    std::cout << "Passed" << std::endl;
  }

  std::cout << "Testing GetNextCell...";
  int status3 = 0;
  int dim = 5;
  vtkSmartPointer<vtkImageData> anImage =
    MakeImage(dim, dim, dim);
  stree->SetDataSet(anImage);
  stree->SetMaxLevel(VTK_INT_MAX);

  vtkIdType cell;
  vtkIdList *ids;
  vtkSmartPointer<vtkFloatArray> scalars =
    vtkSmartPointer<vtkFloatArray>::New();

  // Verify that branching does not affect the number of cells found
  int numExpected = (dim - 1) * (dim - 1);
  for (int b = 2; b < dim * dim; ++b)
  {
    stree->SetBranchingFactor(b);
    stree->Initialize();
    stree->BuildTree();
    for (double v = 0; v < dim - 1; v += 1.0)
    {
      int numGot = 0;
      stree->InitTraversal(v + 0.5);
      while (stree->GetNextCell(cell, ids, scalars))
      {
        ++numGot;
      }
      if (numGot != numExpected)
      {
        std::cout << "For " << v
                  << " Expected " << numExpected
                  << " but got " << numGot << std::endl;
        ++status3;
      }
    }
  }
  if (status3)
  {
    std::cout << "Failed" << std::endl;
    ++status;
  }
  else
  {
    std::cout << "Passed" << std::endl;
  }

  std::cout << "Testing Print...";
  stree->Print(streePrint);
  std::cout << "Passed" << std::endl;

  if (status)
  {
    return EXIT_FAILURE;
  }
  else
  {
    return EXIT_SUCCESS;
  }
}

vtkSmartPointer<vtkImageData> MakeImage(int dimx, int dimy, int dimz)
{
  vtkSmartPointer<vtkImageData> anImage =
    vtkSmartPointer<vtkImageData>::New();
  anImage->SetDimensions(dimx, dimy, dimz);
  anImage->AllocateScalars(VTK_FLOAT,1);
  float* pixel = static_cast<float *>(anImage->GetScalarPointer(0,0,0));
  float value = 0.0;

  for(int z = 0; z < dimz; z++)
  {
    for(int y = 0; y < dimy; y++)
    {
      for(int x = 0; x < dimx; x++)
      {
        *pixel++ = value;
      }
    }
    value += 1.0;
  }
  return anImage;
}
