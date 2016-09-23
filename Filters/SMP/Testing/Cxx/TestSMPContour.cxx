/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCutter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkNew.h"
#include "vtkRTAnalyticSource.h"
#include "vtkPolyData.h"
#include "vtkDataSetTriangleFilter.h"
#include "vtkSMPContourGrid.h"
#include "vtkSMPContourGridManyPieces.h"
#include "vtkContourGrid.h"
#include "vtkContourFilter.h"
#include "vtkUnstructuredGrid.h"
#include "vtkTimerLog.h"
#include "vtkNonMergingPointLocator.h"
#include "vtkSMPTools.h"
#include "vtkXMLMultiBlockDataWriter.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositeDataIterator.h"
#include "vtkElevationFilter.h"
#include "vtkPointData.h"
#include "vtkCellData.h"
#include "vtkPointDataToCellData.h"
#include "vtkXMLPolyDataWriter.h"

#define WRITE_DEBUG 0

const int EXTENT = 30;
int TestSMPContour(int, char *[])
{
  vtkSMPTools::Initialize(2);

  vtkNew<vtkTimerLog> tl;

  vtkNew<vtkRTAnalyticSource> imageSource;
#if 1
  imageSource->SetWholeExtent(-EXTENT, EXTENT, -EXTENT, EXTENT, -EXTENT, EXTENT);
#else
  imageSource->SetWholeExtent(-EXTENT, EXTENT, -EXTENT, EXTENT, 0, 0);
#endif

  vtkNew<vtkElevationFilter> ev;
  ev->SetInputConnection(imageSource->GetOutputPort());
  ev->SetLowPoint(-EXTENT, -EXTENT, -EXTENT);
  ev->SetHighPoint(EXTENT, EXTENT, EXTENT);

  vtkNew<vtkDataSetTriangleFilter> tetraFilter;
  tetraFilter->SetInputConnection(ev->GetOutputPort());

  tl->StartTimer();

  vtkNew<vtkPointDataToCellData> p2c;
  p2c->SetInputConnection(tetraFilter->GetOutputPort());
  p2c->Update();

  tetraFilter->GetOutput()->GetCellData()->ShallowCopy(p2c->GetOutput()->GetCellData());

  tl->StopTimer();
  cout << "Data generation time: " << tl->GetElapsedTime() << endl;

  cout << "Contour grid: " << endl;
  vtkNew<vtkContourGrid> cg;
  cg->SetInputData(tetraFilter->GetOutput());
  cg->SetInputArrayToProcess(0, 0, 0, 0, "RTData");
  cg->SetValue(0, 200);
  cg->SetValue(1, 220);
  tl->StartTimer();
  cg->Update();
  tl->StopTimer();

  vtkIdType baseNumCells = cg->GetOutput()->GetNumberOfCells();

  cout << "Number of cells: " << cg->GetOutput()->GetNumberOfCells() << endl;
  cout << "NUmber of points: " << cg->GetOutput()->GetNumberOfPoints() << endl;
  cout << "Time: " << tl->GetElapsedTime() << endl;

  cout << "Contour filter: " << endl;
  vtkNew<vtkContourFilter> cf;
  cf->SetInputData(tetraFilter->GetOutput());
  cf->SetInputArrayToProcess(0, 0, 0, 0, "RTData");
  cf->SetValue(0, 200);
  cf->SetValue(1, 220);
  tl->StartTimer();
  cf->Update();
  tl->StopTimer();

  cout << "Number of cells: " << cf->GetOutput()->GetNumberOfCells() << endl;
  cout << "Time: " << tl->GetElapsedTime() << endl;

  cout << "SMP Contour grid: " << endl;
  vtkNew<vtkSMPContourGrid> cg2;
  cg2->SetInputData(tetraFilter->GetOutput());
  cg2->SetInputArrayToProcess(0, 0, 0, 0, "RTData");
  cg2->SetValue(0, 200);
  cg2->SetValue(1, 220);
  tl->StartTimer();
  cg2->Update();
  tl->StopTimer();

  cout << "Time: " << tl->GetElapsedTime() << endl;

#if WRITE_DEBUG
  vtkNew<vtkXMLPolyDataWriter> pdwriter;
  pdwriter->SetInputData(cg2->GetOutput());
  pdwriter->SetFileName("contour.vtp");
  //pwriter->SetDataModeToAscii();
  pdwriter->Write();
#endif

  if (cg2->GetOutput()->GetNumberOfCells() != baseNumCells)
  {
    cout << "Error in vtkSMPContourGrid (MergePieces = true) output." << endl;
    cout << "Number of cells does not match expected, "
         << cg2->GetOutput()->GetNumberOfCells() << " vs. " << baseNumCells << endl;
    return EXIT_FAILURE;
  }

  cout << "SMP Contour grid: " << endl;
  cg2->MergePiecesOff();
  tl->StartTimer();
  cg2->Update();
  tl->StopTimer();

  cout << "Time: " << tl->GetElapsedTime() << endl;

  vtkIdType numCells = 0;

  vtkCompositeDataSet* cds = vtkCompositeDataSet::SafeDownCast(
    cg2->GetOutputDataObject(0));
  if (cds)
  {
    vtkCompositeDataIterator* iter = cds->NewIterator();
    iter->InitTraversal();
    while (!iter->IsDoneWithTraversal())
    {
      vtkPolyData* pd = vtkPolyData::SafeDownCast(
        iter->GetCurrentDataObject());
      if (pd)
      {
        numCells += pd->GetNumberOfCells();
      }
      iter->GoToNextItem();
    }
    iter->Delete();
  }

  if (numCells != baseNumCells)
  {
    cout << "Error in vtkSMPContourGrid (MergePieces = false) output." << endl;
    cout << "Number of cells does not match expected, "
         << numCells << " vs. " << baseNumCells << endl;
    return EXIT_FAILURE;
  }

  vtkNew<vtkSMPContourGridManyPieces> cg3;
  cg3->SetInputData(tetraFilter->GetOutput());
  cg3->SetInputArrayToProcess(0, 0, 0, 0, "RTData");
  cg3->SetValue(0, 200);
  cg3->SetValue(1, 220);
  cout << "SMP Contour grid: " << endl;
  tl->StartTimer();
  cg3->Update();
  tl->StopTimer();
  cout << "Time: " << tl->GetElapsedTime() << endl;

  numCells = 0;

  cds = vtkCompositeDataSet::SafeDownCast(
    cg2->GetOutputDataObject(0));
  if (cds)
  {
    vtkCompositeDataIterator* iter = cds->NewIterator();
    iter->InitTraversal();
    while (!iter->IsDoneWithTraversal())
    {
      vtkPolyData* pd = vtkPolyData::SafeDownCast(
        iter->GetCurrentDataObject());
      if (pd)
      {
        numCells += pd->GetNumberOfCells();
      }
      iter->GoToNextItem();
    }
    iter->Delete();
  }

  if (numCells != baseNumCells)
  {
    cout << "Error in vtkSMPContourGridManyPieces output." << endl;
    cout << "Number of cells does not match expected, "
         << numCells << " vs. " << baseNumCells << endl;
    return EXIT_FAILURE;
  }

#if WRITE_DEBUG
  vtkNew<vtkXMLMultiBlockDataWriter> writer;
  writer->SetInputData(cg2->GetOutputDataObject(0));
  writer->SetFileName("contour1.vtm");
  writer->SetDataModeToAscii();
  writer->Write();

  vtkNew<vtkXMLMultiBlockDataWriter> writer2;
  writer2->SetInputData(cg3->GetOutputDataObject(0));
  writer2->SetFileName("contour2.vtm");
  writer2->SetDataModeToAscii();
  writer2->Write();
#endif

  return EXIT_SUCCESS;
}
