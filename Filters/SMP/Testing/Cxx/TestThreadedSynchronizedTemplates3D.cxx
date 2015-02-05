/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestThreadedSynchronizedTemplates3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCompositeDataIterator.h"
#include "vtkImageData.h"
#include "vtkMultiBlockDataSet.h"
#include "vtkNew.h"
#include "vtkPolyData.h"
#include "vtkRTAnalyticSource.h"
#include "vtkSmartPointer.h"
#include "vtkSMPTools.h"
#include "vtkSynchronizedTemplates3D.h"
#include "vtkThreadedSynchronizedTemplates3D.h"
#include "vtkTimerLog.h"


int TestThreadedSynchronizedTemplates3D(int, char *[])
{
  static const int dim = 256;
  static int ext[6] = { 0, dim - 1, 0, dim - 1, 0, dim - 1 };

  //vtkSMPTools::Initialize(4);
  vtkNew<vtkTimerLog> tl;

  vtkNew<vtkRTAnalyticSource> source;
  source->SetWholeExtent(ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
  tl->StartTimer();
  source->Update();
  tl->StopTimer();

  double drange[2];
  source->GetOutput()->GetScalarRange(drange);
  double isoval = (drange[0] + drange[1]) * 0.5;

  cout << "Creation time: " << tl->GetElapsedTime() << " seconds" << endl;

  vtkNew<vtkThreadedSynchronizedTemplates3D> cf;
  cf->SetInputData(source->GetOutput());
  cf->SetInputArrayToProcess(0, 0, 0, 0, "RTData");
  cf->SetValue(0, isoval);
  cf->ComputeNormalsOn();
  cf->ComputeScalarsOff();
  tl->StartTimer();
  cf->Update();
  tl->StopTimer();

  double parallelTime = tl->GetElapsedTime();
  cout << "Parallel execution time: " << parallelTime << " seconds" << endl;

  vtkIdType parNumCells = 0, numPieces = 0;
  vtkSmartPointer<vtkCompositeDataIterator> iter;
  iter.TakeReference(static_cast<vtkCompositeDataSet*>(
    cf->GetOutputDataObject(0))->NewIterator());
  iter->InitTraversal();
  while(!iter->IsDoneWithTraversal())
    {
    vtkPolyData* piece =
      static_cast<vtkPolyData*>(iter->GetCurrentDataObject());
    parNumCells += piece->GetNumberOfCells();
    ++numPieces;
    iter->GoToNextItem();
    }

  cout << "Total num. cells: " << parNumCells << endl;


  vtkNew<vtkSynchronizedTemplates3D> st;
  st->SetInputData(source->GetOutput());
  st->SetInputArrayToProcess(0, 0, 0, 0, "RTData");
  st->SetValue(0, isoval);
  st->ComputeNormalsOn();
  st->ComputeScalarsOff();
  tl->StartTimer();
  st->Update();
  tl->StopTimer();

  double serialTime = tl->GetElapsedTime();
  cout << "Serial execution time: " << serialTime << " seconds" << endl;

  vtkIdType serNumCells = st->GetOutput()->GetNumberOfCells();
  cout << "Serial num. cells: " << serNumCells << endl;

  if (parNumCells != serNumCells)
    {
    cout << "Number of cells did not match." << endl;
    return EXIT_FAILURE;
    }

  cout << "Success!" << endl;
  cout << "speedup = " << serialTime/parallelTime << "x with "
       << numPieces << " threads" << endl;

  return EXIT_SUCCESS;
}
