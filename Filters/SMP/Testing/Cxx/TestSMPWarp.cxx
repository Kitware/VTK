/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSMPWarp.cxx

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
#include "vtkSMPWarpVector.h"
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

class vtkBoundsFunctor
{
  typedef vtkSMPThreadLocal<std::vector<double> > TLS;
  typedef TLS::iterator TLSIter;
public:
  TLS tlBounds;
  vtkFloatArray* pts;
  double bounds[6];
  double defaults[6];

  vtkBoundsFunctor()
    {
      static const double adefaults[] = { VTK_DOUBLE_MAX, - VTK_DOUBLE_MAX,
                                          VTK_DOUBLE_MAX, - VTK_DOUBLE_MAX,
                                          VTK_DOUBLE_MAX, - VTK_DOUBLE_MAX};
      memcpy(defaults, adefaults, 6*sizeof(double));
      memcpy(bounds, adefaults, 6*sizeof(double));
    }

  void Initialize()
    {
      std::vector<double>& lbounds = tlBounds.Local();
      lbounds.resize(6);
      memcpy(&lbounds[0], defaults, 6*sizeof(double));
    }

  void Reduce()
    {
      TLSIter end = tlBounds.end();
      for (TLSIter itr = tlBounds.begin(); itr != end; ++itr)
        {
        std::vector<double>& aBounds = *itr;
        bounds[0] = bounds[0] < aBounds[0] ? bounds[0] : aBounds[0];
        bounds[1] = bounds[1] > aBounds[1] ? bounds[1] : aBounds[1];
        bounds[2] = bounds[2] < aBounds[2] ? bounds[2] : aBounds[2];
        bounds[3] = bounds[3] > aBounds[3] ? bounds[3] : aBounds[3];
        bounds[4] = bounds[4] < aBounds[4] ? bounds[4] : aBounds[4];
        bounds[5] = bounds[5] > aBounds[5] ? bounds[5] : aBounds[5];
        }
    }

  void operator()(vtkIdType begin, vtkIdType end)
    {
      std::vector<double>& bds = tlBounds.Local();
      double* lbounds = &bds[0];

      /*
      double bounds[] = { VTK_DOUBLE_MAX, - VTK_DOUBLE_MAX,
                          VTK_DOUBLE_MAX, - VTK_DOUBLE_MAX,
                          VTK_DOUBLE_MAX, - VTK_DOUBLE_MAX};
      */

      //float x[3];
      float* x;
      float* fptr = pts->GetPointer(3*begin);
      for (vtkIdType i=begin; i<end; i++)
        {
        //pts->GetTupleValue(i, x);
        x = fptr;
        lbounds[0] = x[0] < lbounds[0] ? x[0] : lbounds[0];
        lbounds[1] = x[0] > lbounds[1] ? x[0] : lbounds[1];
        lbounds[2] = x[1] < lbounds[2] ? x[1] : lbounds[2];
        lbounds[3] = x[1] > lbounds[3] ? x[1] : lbounds[3];
        lbounds[4] = x[2] < lbounds[4] ? x[2] : lbounds[4];
        lbounds[5] = x[2] > lbounds[5] ? x[2] : lbounds[5];
        fptr += 3;
        }
    }
};

/*
class vtkSetFunctor
{
public:
  vtkFloatArray* pts;

  void  operator()(vtkIdType begin, vtkIdType end)
  {
    float pt[3];
    vtkIdType counter = 0;
    for (int k=begin; k<end; k++)
      for (int j=0; j<resolution; j++)
        for (int i=0; i<resolution; i++)
          {
          pt[0] = i*spacing;
          pt[1] = j*spacing;
          pt[2] = k*spacing;
#if 0
          pts->SetTuple(counter++, pt);
#else
          pts->SetTupleValue(counter++, pt);
          // pts->SetValue(counter++, i*spacing);
          // pts->SetValue(counter++, j*spacing);
          // pts->SetValue(counter++, k*spacing);
#endif
          }
  }
};
*/

int TestSMPWarp(int argc, char* argv[])
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

  vtkNew<vtkWarpVector> vw;
  vw->SetInputData(sg.GetPointer());
  vw->SetInputArrayToProcess(0, 0, 0,
                             vtkDataObject::FIELD_ASSOCIATION_POINTS,
                             "Disp");
  tl->StartTimer();
  vw->Update();
  tl->StopTimer();
  cout << "Serial warp: " << tl->GetElapsedTime() << endl;

  tl->StartTimer();
  double bounds[6];
  vw->GetOutput()->GetBounds(bounds);
  tl->StopTimer();
  cout << "Get bounds: " << tl->GetElapsedTime() << endl;

  vtkBoundsFunctor calcBounds;
  calcBounds.pts = (vtkFloatArray*)vw->GetOutput()->GetPoints()->GetData();
  tl->StartTimer();
  vtkSMPTools::For(0, resolution*resolution*resolution, calcBounds);
  tl->StopTimer();
  cout << "Get bounds (parallel): " << tl->GetElapsedTime() << endl;
  cout << calcBounds.bounds[0] << " " << calcBounds.bounds[1] << " "
       << calcBounds.bounds[2] << " " << calcBounds.bounds[3] << " "
       << calcBounds.bounds[4] << " " << calcBounds.bounds[5] << endl;

  for (int i=0; i<6; i++)
    {
    if (calcBounds.bounds[i] != bounds[i])
      {
      cout << "Bounds did not match after parallel bounds calculation" << endl;
      return EXIT_FAILURE;
      }
    }
  // Release memory so that we can do more.
  vw->GetOutput()->Initialize();

  vtkNew<vtkSMPWarpVector> smpvw;
  smpvw->SetInputData(sg.GetPointer());
  smpvw->SetInputArrayToProcess(0, 0, 0,
                                vtkDataObject::FIELD_ASSOCIATION_POINTS,
                                "Disp");
  tl->StartTimer();
  smpvw->Update();
  tl->StopTimer();
  cout << "Parallel warp: " << tl->GetElapsedTime() << endl;

  calcBounds.pts = (vtkFloatArray*)smpvw->GetOutput()->GetPoints()->GetData();
  tl->StartTimer();
  vtkSMPTools::For(0, resolution*resolution*resolution, calcBounds);
  tl->StopTimer();
  cout << "Get bounds (parallel): " << tl->GetElapsedTime() << endl;
  cout << calcBounds.bounds[0] << " " << calcBounds.bounds[1] << " "
       << calcBounds.bounds[2] << " " << calcBounds.bounds[3] << " "
       << calcBounds.bounds[4] << " " << calcBounds.bounds[5] << endl;

  for (int i=0; i<6; i++)
    {
    if (calcBounds.bounds[i] != bounds[i])
      {
      cout << "Bounds did not match after parallel bounds calculation" << endl;
      return EXIT_FAILURE;
      }
    }

  return EXIT_SUCCESS;
}
