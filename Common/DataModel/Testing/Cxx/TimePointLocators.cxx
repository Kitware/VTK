/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPointLocators.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkPointLocator.h"
#include "vtkStaticPointLocator.h"
#include "vtkKdTree.h"
#include "vtkKdTreePointLocator.h"
#include "vtkOctreePointLocator.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkTimerLog.h"
#include "vtkMath.h"


int TimePointLocators(int , char *[])
{
  int nPts = 1000000;
  int nQ = nPts/10;
  int N = 10;
  double R = 0.01;

  vtkTimerLog* timer = vtkTimerLog::New();
  double buildTime[4], cpTime[4], cnpTime[4], crpTime[4];
  for (int i=0; i<4; ++i)
  {
    buildTime[i] = cpTime[i] = cnpTime[i] = crpTime[i] = 0.0;
  }

  cout << "\nTiming for " << nPts << " points, " << nQ << " queries\n";

  // Populate a list of points and query locations
  vtkPoints* points = vtkPoints::New();
  points->SetDataTypeToDouble();
  points->SetNumberOfPoints(nPts);
  for (int i=0; i<nPts; ++i)
  {
    points->SetPoint(i, vtkMath::Random(-1,1), vtkMath::Random(-1,1),
                     vtkMath::Random(-1,1));
  }

  vtkPolyData* polydata = vtkPolyData::New();
  polydata->SetPoints(points);
  points->ComputeBounds();

  // Create points array which are positions to probe data with FindClosestPoint().
  vtkPoints* qPoints = vtkPoints::New();
  qPoints->SetDataTypeToDouble();
  qPoints->SetNumberOfPoints(nQ);
  vtkMath::RandomSeed(314159);
  for (int i=0; i<nQ; ++i)
  {
    qPoints->SetPoint(i, vtkMath::Random(-1,1), vtkMath::Random(-1,1),
                          vtkMath::Random(-1,1));
  }

  vtkIdList *closest = vtkIdList::New();

  //---------------------------------------------------------------------------
  // The simple uniform binning point locator
  vtkPointLocator* uniformLocator = vtkPointLocator::New();
  timer->StartTimer();
  uniformLocator->SetDataSet(polydata);
  uniformLocator->BuildLocator();
  timer->StopTimer();
  uniformLocator->Delete();
  buildTime[0] = timer->GetElapsedTime();

  uniformLocator = vtkPointLocator::New();
  uniformLocator->SetDataSet(polydata);
  uniformLocator->BuildLocator();
  timer->StartTimer();
  for (int i=0; i<nQ; ++i)
  {
    uniformLocator->FindClosestPoint(qPoints->GetPoint(i));
  }
  timer->StopTimer();
  cpTime[0] = timer->GetElapsedTime();

  timer->StartTimer();
  for (int i=0; i<nQ; ++i)
  {
    uniformLocator->FindClosestNPoints(N, qPoints->GetPoint(i), closest);
  }
  timer->StopTimer();
  cnpTime[0] = timer->GetElapsedTime();

  timer->StartTimer();
  for (int i=0; i<nQ; ++i)
  {
    uniformLocator->FindPointsWithinRadius(R, qPoints->GetPoint(i), closest);
  }
  timer->StopTimer();
  crpTime[0] = timer->GetElapsedTime();
  uniformLocator->Delete();

  //---------------------------------------------------------------------------
  // The simple uniform binning point locator, this time built
  // statically and threaded (may be much faster on threaded machine).
  vtkStaticPointLocator* staticLocator = vtkStaticPointLocator::New();
  timer->StartTimer();
  staticLocator->SetDataSet(polydata);
  staticLocator->BuildLocator();
  timer->StopTimer();
  staticLocator->Delete();
  buildTime[1] = timer->GetElapsedTime();

  staticLocator = vtkStaticPointLocator::New();
  staticLocator->SetDataSet(polydata);
  staticLocator->BuildLocator();
  timer->StartTimer();
  for (int i=0; i<nQ; ++i)
  {
    staticLocator->FindClosestPoint(qPoints->GetPoint(i));
  }
  timer->StopTimer();
  cpTime[1] = timer->GetElapsedTime();

  timer->StartTimer();
  for (int i=0; i<nQ; ++i)
  {
    staticLocator->FindClosestNPoints(N, qPoints->GetPoint(i), closest);
  }
  timer->StopTimer();
  cnpTime[1] = timer->GetElapsedTime();

  timer->StartTimer();
  for (int i=0; i<nQ; ++i)
  {
    staticLocator->FindPointsWithinRadius(R, qPoints->GetPoint(i), closest);
  }
  timer->StopTimer();
  crpTime[1] = timer->GetElapsedTime();
  staticLocator->Delete();

  //---------------------------------------------------------------------------
  // The KD Tree point locator
  vtkKdTreePointLocator* kdTreeLocator = vtkKdTreePointLocator::New();
  timer->StartTimer();
  kdTreeLocator->SetDataSet(polydata);
  kdTreeLocator->BuildLocator();
  kdTreeLocator->Delete();
  timer->StopTimer();
  buildTime[2] = timer->GetElapsedTime();

  kdTreeLocator = vtkKdTreePointLocator::New();
  kdTreeLocator->SetDataSet(polydata);
  kdTreeLocator->BuildLocator();
  timer->StartTimer();
  for (int i=0; i<nQ; ++i)
  {
    kdTreeLocator->FindClosestPoint(qPoints->GetPoint(i));
  }
  timer->StopTimer();
  cpTime[2] = timer->GetElapsedTime();

  timer->StartTimer();
  for (int i=0; i<nQ; ++i)
  {
    kdTreeLocator->FindClosestNPoints(N, qPoints->GetPoint(i), closest);
  }
  timer->StopTimer();
  cnpTime[2] = timer->GetElapsedTime();

  timer->StartTimer();
  for (int i=0; i<nQ; ++i)
  {
    kdTreeLocator->FindPointsWithinRadius(R, qPoints->GetPoint(i), closest);
  }
  timer->StopTimer();
  crpTime[2] = timer->GetElapsedTime();
  kdTreeLocator->Delete();

  //---------------------------------------------------------------------------
  // Octree point locator
  vtkOctreePointLocator* octreeLocator = vtkOctreePointLocator::New();
  timer->StartTimer();
  octreeLocator->SetDataSet(polydata);
  octreeLocator->BuildLocator();
  octreeLocator->Delete();
  timer->StopTimer();
  buildTime[3] = timer->GetElapsedTime();

  octreeLocator = vtkOctreePointLocator::New();
  octreeLocator->SetDataSet(polydata);
  octreeLocator->BuildLocator();
  timer->StartTimer();
  for (int i=0; i<nQ; ++i)
  {
    octreeLocator->FindClosestPoint(qPoints->GetPoint(i));
  }
  timer->StopTimer();
  cpTime[3] = timer->GetElapsedTime();

  timer->StartTimer();
  for (int i=0; i<nQ; ++i)
  {
    octreeLocator->FindClosestNPoints(N, qPoints->GetPoint(i), closest);
  }
  timer->StopTimer();
  cnpTime[3] = timer->GetElapsedTime();

  timer->StartTimer();
  for (int i=0; i<nQ; ++i)
  {
    octreeLocator->FindPointsWithinRadius(R, qPoints->GetPoint(i), closest);
  }
  timer->StopTimer();
  crpTime[3] = timer->GetElapsedTime();
  octreeLocator->Delete();

  //---------------------------------------------------------------------------
  // Print out the statistics
  cout << "Build and delete tree\n";
  cout << "\tUniform: " << buildTime[0] << "\n";
  cout << "\tStatic: " << buildTime[1] << "\n";
  cout << "\tOctree: " << buildTime[2] << "\n";
  cout << "\tKD Tree: " << buildTime[3] << "\n";

  cout << "Closest point queries\n";
  cout << "\tUniform: " << cpTime[0] << "\n";
  cout << "\tStatic: " << cpTime[1] << "\n";
  cout << "\tOctree: " << cpTime[2] << "\n";
  cout << "\tKD Tree: " << cpTime[3] << "\n";

  cout << "Closest N points queries\n";
  cout << "\tUniform: " << cnpTime[0] << "\n";
  cout << "\tStatic: " << cnpTime[1] << "\n";
  cout << "\tOctree: " << cnpTime[2] << "\n";
  cout << "\tKD Tree: " << cnpTime[3] << "\n";

  cout << "Closest points within radius queries\n";
  cout << "\tUniform: " << crpTime[0] << "\n";
  cout << "\tStatic: " << crpTime[1] << "\n";
  cout << "\tOctree: " << crpTime[2] << "\n";
  cout << "\tKD Tree: " << crpTime[3] << "\n";

  cout << "Total time\n";
  cout << "\tUniform: " << (buildTime[0] + cpTime[0] + cnpTime[0] + crpTime[0]) << "\n";
  cout << "\tStatic: " << (buildTime[1] + cpTime[1] + cnpTime[1] + crpTime[1]) << "\n";
  cout << "\tOctree: " << (buildTime[2] + cpTime[2] + cnpTime[2] + crpTime[2]) << "\n";
  cout << "\tKD Tree: " << (buildTime[3] + cpTime[3] + cnpTime[3] + crpTime[3]) << "\n";

  timer->Delete();
  points->Delete();
  qPoints->Delete();
  polydata->Delete();
  closest->Delete();

  // Always return success, although the test infrastructure should catch
  // excessive execution times.
  return 0;
}
