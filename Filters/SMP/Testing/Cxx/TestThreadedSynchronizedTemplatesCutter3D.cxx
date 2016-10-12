/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestThreadedSynchronizedTemplatesCutter3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAppendPolyData.h"
#include "vtkCleanPolyData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkRTAnalyticSource.h"
#include "vtkSmartPointer.h"
#include "vtkSMPTools.h"
#include "vtkSphere.h"
#include "vtkSynchronizedTemplatesCutter3D.h"
#include "vtkThreadedSynchronizedTemplatesCutter3D.h"
#include "vtkTimerLog.h"

#include <algorithm>


int TestThreadedSynchronizedTemplatesCutter3D(int, char *[])
{
  static const int dim = 257;
  static int ext[6] = { 0, dim - 1, 0, dim - 1, 0, dim - 1 };

  //vtkSMPTools::Initialize(4);
  vtkNew<vtkTimerLog> tl;

  vtkNew<vtkRTAnalyticSource> source;
  source->SetWholeExtent(ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
  tl->StartTimer();
  source->Update();
  tl->StopTimer();

  cout << "Creation time: " << tl->GetElapsedTime() << " seconds" << endl;

  double bounds[6];
  source->GetOutput()->GetBounds(bounds);

  double center[3] = {(bounds[0] + bounds[1])/2.0, (bounds[2] + bounds[3])/2.0,
                      (bounds[4] + bounds[5])/2.0};
  double radius = std::min((bounds[1] - bounds[0])/2.0,
                           std::min((bounds[3] - bounds[2])/2.0,
                                    (bounds[5] - bounds[4])/2.0));

  vtkNew<vtkSphere> impfunc;
  impfunc->SetRadius(radius);
  impfunc->SetCenter(center);

  vtkNew<vtkSynchronizedTemplatesCutter3D> sc;
  sc->SetInputData(source->GetOutput());
  sc->SetCutFunction(impfunc.GetPointer());
  tl->StartTimer();
  sc->Update();
  tl->StopTimer();

  double serialTime = tl->GetElapsedTime();
  cout << "Serial Execution Time: " << serialTime << " seconds" << endl;

  vtkNew<vtkThreadedSynchronizedTemplatesCutter3D> pc;
  pc->SetInputData(source->GetOutput());
  pc->SetCutFunction(impfunc.GetPointer());
  tl->StartTimer();
  pc->Update();
  tl->StopTimer();

  double parallelTime = tl->GetElapsedTime();
  cout << "SMP Execution Time: " << parallelTime << " seconds" << endl;

  int numPieces = 0;
  vtkNew<vtkAppendPolyData> appender;
  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(static_cast<vtkCompositeDataSet*>(
    pc->GetOutputDataObject(0))->NewIterator());
  iter->InitTraversal();
  while(!iter->IsDoneWithTraversal())
  {
    vtkPolyData* piece =
      static_cast<vtkPolyData*>(iter->GetCurrentDataObject());
    appender->AddInputData(piece);
    ++numPieces;
    iter->GoToNextItem();
  }
  tl->StartTimer();
  appender->Update();
  tl->StopTimer();

  cout << "Append Poly Time: " << tl->GetElapsedTime() << " seconds" << endl;

  vtkNew<vtkCleanPolyData> cleaner1, cleaner2;
  cleaner1->SetInputData(sc->GetOutput());
  cleaner1->Update();
  cleaner2->SetInputData(appender->GetOutput());
  cleaner2->Update();

  int npoints1 = cleaner1->GetOutput()->GetNumberOfPoints();
  int ntriangles1 = cleaner1->GetOutput()->GetNumberOfCells();
  int npoints2 = cleaner2->GetOutput()->GetNumberOfPoints();
  int ntriangles2 = cleaner2->GetOutput()->GetNumberOfCells();

  cout << "Serial Output: Triangles=" << ntriangles1 << ", Points="
       << npoints1 << endl;
  cout << "SMP Output: Triangles=" << ntriangles2 << ", Points="
       << npoints2 << endl;

  if (npoints1 == npoints2 && ntriangles1 == ntriangles2)
  {
    cout << "Outputs match" << endl;
    cout << "speedup = " << serialTime/parallelTime << "x with "
         << numPieces << " threads" << endl;
    return EXIT_SUCCESS;
  }

  cout << "Outputs don't match" << endl;
  return EXIT_FAILURE;
}
