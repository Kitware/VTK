// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// VTK_DEPRECATED_IN_9_7_0()
#define VTK_DEPRECATION_LEVEL 0

#include "vtkCellData.h"
#include "vtkCompositeDataIterator.h"
#include "vtkCompositeDataSet.h"
#include "vtkContourFilter.h"
#include "vtkContourGrid.h"
#include "vtkDataSetTriangleFilter.h"
#include "vtkElevationFilter.h"
#include "vtkNew.h"
#include "vtkPointDataToCellData.h"
#include "vtkPolyData.h"
#include "vtkRTAnalyticSource.h"
#include "vtkSMPContourGrid.h"
#include "vtkSMPTools.h"
#include "vtkTimerLog.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLPolyDataWriter.h"

#include <iostream>

#define WRITE_DEBUG 0

constexpr int EXTENT = 30;
int TestSMPContour(int, char*[])
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
  std::cout << "Data generation time: " << tl->GetElapsedTime() << std::endl;

  std::cout << "Contour grid: " << std::endl;
  vtkNew<vtkContourGrid> cg;
  cg->SetInputData(tetraFilter->GetOutput());
  cg->SetInputArrayToProcess(0, 0, 0, 0, "RTData");
  cg->SetValue(0, 200);
  cg->SetValue(1, 220);
  tl->StartTimer();
  cg->Update();
  tl->StopTimer();

  vtkIdType baseNumCells = cg->GetOutput()->GetNumberOfCells();

  std::cout << "Number of cells: " << cg->GetOutput()->GetNumberOfCells() << std::endl;
  std::cout << "NUmber of points: " << cg->GetOutput()->GetNumberOfPoints() << std::endl;
  std::cout << "Time: " << tl->GetElapsedTime() << std::endl;

  std::cout << "Contour filter: " << std::endl;
  vtkNew<vtkContourFilter> cf;
  cf->SetInputData(tetraFilter->GetOutput());
  cf->SetInputArrayToProcess(0, 0, 0, 0, "RTData");
  cf->SetValue(0, 200);
  cf->SetValue(1, 220);
  tl->StartTimer();
  cf->Update();
  tl->StopTimer();

  std::cout << "Number of cells: " << cf->GetOutput()->GetNumberOfCells() << std::endl;
  std::cout << "Time: " << tl->GetElapsedTime() << std::endl;

  std::cout << "SMP Contour grid: " << std::endl;
  vtkNew<vtkSMPContourGrid> cg2;
  cg2->SetInputData(tetraFilter->GetOutput());
  cg2->SetInputArrayToProcess(0, 0, 0, 0, "RTData");
  cg2->SetValue(0, 200);
  cg2->SetValue(1, 220);
  tl->StartTimer();
  cg2->Update();
  tl->StopTimer();

  std::cout << "Time: " << tl->GetElapsedTime() << std::endl;

#if WRITE_DEBUG
  vtkNew<vtkXMLPolyDataWriter> pdwriter;
  pdwriter->SetInputData(cg2->GetOutput());
  pdwriter->SetFileName("contour.vtp");
  // pwriter->SetDataModeToAscii();
  pdwriter->Write();
#endif

  if (cg2->GetOutput()->GetNumberOfCells() != baseNumCells)
  {
    std::cout << "Error in vtkSMPContourGrid (MergePieces = true) output." << std::endl;
    std::cout << "Number of cells does not match expected, " << cg2->GetOutput()->GetNumberOfCells()
              << " vs. " << baseNumCells << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "SMP Contour grid: " << std::endl;
  cg2->MergePiecesOff();
  tl->StartTimer();
  cg2->Update();
  tl->StopTimer();

  std::cout << "Time: " << tl->GetElapsedTime() << std::endl;

  vtkIdType numCells = 0;

  vtkCompositeDataSet* cds = vtkCompositeDataSet::SafeDownCast(cg2->GetOutputDataObject(0));
  if (cds)
  {
    vtkCompositeDataIterator* iter = cds->NewIterator();
    iter->InitTraversal();
    while (!iter->IsDoneWithTraversal())
    {
      vtkPolyData* pd = vtkPolyData::SafeDownCast(iter->GetCurrentDataObject());
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
    std::cout << "Error in vtkSMPContourGrid (MergePieces = false) output." << std::endl;
    std::cout << "Number of cells does not match expected, " << numCells << " vs. " << baseNumCells
              << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
