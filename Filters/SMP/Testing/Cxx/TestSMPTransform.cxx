/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSMPTransform.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkDataSet.h"
#include "vtkElevationFilter.h"
#include "vtkFloatArray.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkSMPTools.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkSMPTransform.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkStructuredGrid.h"
#include "vtkSMPThreadLocal.h"
#include "vtkTimerLog.h"

const double spacing = 0.1;
const int resolution = 101;

class vtkSetFunctor2
{
public:
  float* pts;
  float* disp;

  void  operator()(vtkIdType begin, vtkIdType end)
  {
    vtkIdType offset = 3 * begin * resolution * resolution;
    float* itr = pts + offset;
    float* ditr = disp + offset;

    for (int k=begin; k<end; k++)
      for (int j=0; j<resolution; j++)
        for (int i=0; i<resolution; i++)
          {
          *itr = i*spacing;
          itr++;
          *itr = j*spacing;
          itr++;
          *itr = k*spacing;
          itr++;

          *ditr = 10;
          ditr++;
          *ditr = 10;
          ditr++;
          *ditr = 10;
          ditr++;
          }
  }
};

int TestSMPTransform(int argc, char* argv[])
{
  int numThreads = 2;
  for(int argi=1; argi<argc; argi++)
    {
    if(std::string(argv[argi])=="--numThreads")
      {
      numThreads=atoi(argv[++argi]);
      break;
      }
    }
  cout << "Num. threads: " << numThreads << endl;
  vtkSMPTools::Initialize(numThreads);

  vtkNew<vtkTimerLog> tl;

  vtkNew<vtkStructuredGrid> sg;
  sg->SetDimensions(resolution, resolution, resolution);

  vtkNew<vtkPoints> pts;
  pts->SetNumberOfPoints(resolution*resolution*resolution);

  //vtkSetFunctor func;
  vtkSetFunctor2 func;
  func.pts = (float*)pts->GetVoidPointer(0);
  //func.pts = (vtkFloatArray*)pts->GetData();

  sg->SetPoints(pts.GetPointer());

  vtkNew<vtkFloatArray> disp;
  disp->SetNumberOfComponents(3);
  disp->SetNumberOfTuples(sg->GetNumberOfPoints());
  disp->SetName("Disp");
  sg->GetPointData()->AddArray(disp.GetPointer());
  func.disp = (float*)disp->GetVoidPointer(0);

  tl->StartTimer();
  vtkSMPTools::For(0, resolution, func);
  tl->StopTimer();
  cout << "Initialize: " << tl->GetElapsedTime() << endl;

  vtkNew<vtkTransformFilter> tr;
  tr->SetInputData(sg.GetPointer());

  vtkNew<vtkTransform> serialTr;
  serialTr->Identity();
  tr->SetTransform(serialTr.GetPointer());

  tl->StartTimer();
  tr->Update();
  tl->StopTimer();
  cout << "Serial transform: " << tl->GetElapsedTime() << endl;

  // Release memory so that we can do more.
  tr->GetOutput()->Initialize();

  vtkNew<vtkTransformFilter> tr2;
  tr2->SetInputData(sg.GetPointer());

  vtkNew<vtkSMPTransform> parallelTr;
  parallelTr->Identity();
  tr2->SetTransform(parallelTr.GetPointer());

  tl->StartTimer();
  tr2->Update();
  tl->StopTimer();
  cout << "Parallel transform: " << tl->GetElapsedTime() << endl;

  return EXIT_SUCCESS;
}
