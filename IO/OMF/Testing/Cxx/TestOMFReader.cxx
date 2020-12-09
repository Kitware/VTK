/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOMFReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Description:
// Tests reading of a simple OMF file

#include "vtkOMFReader.h"

#include "vtkActor.h"
#include "vtkCellData.h"
#include "vtkDataAssembly.h"
#include "vtkDataSetMapper.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkStructuredGrid.h"
#include "vtkTestUtilities.h"

vtkIdType getDataElementIndex(const vtkDataAssembly* assembly, const char* name)
{
  auto nodeId = assembly->FindFirstNodeWithName(name);
  assert(nodeId != -1);
  auto dsIndices = assembly->GetDataSetIndices(nodeId);
  assert(dsIndices.size() == 1);
  return dsIndices[0];
}

int TestOMFReader(int argc, char* argv[])
{
  char* filename = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/omf-test-file.omf");

  vtkNew<vtkOMFReader> reader;
  reader->SetFileName(filename);
  delete[] filename;

  reader->UpdateInformation();
  assert(reader->GetNumberOfDataElementArrays() == 9);

  reader->Update();
  vtkSmartPointer<vtkPartitionedDataSetCollection> output =
    vtkPartitionedDataSetCollection::SafeDownCast(reader->GetOutputDataObject(0));
  assert(output->GetNumberOfPartitionedDataSets() == 9);
  auto assembly = output->GetDataAssembly();
  auto idx = getDataElementIndex(assembly, "Topography");

  auto pds = output->GetPartitionedDataSet(idx);
  auto data = vtkPolyData::SafeDownCast(pds->GetPartition(0));
  data->GetPointData()->SetActiveScalars("Elevation");

  vtkNew<vtkDataSetMapper> surfaceMapper;
  surfaceMapper->SetInputDataObject(data);
  surfaceMapper->ScalarVisibilityOn();
  surfaceMapper->SetScalarRange(0, 3600);

  vtkNew<vtkActor> surfaceActor;
  surfaceActor->SetMapper(surfaceMapper);
  surfaceActor->GetProperty()->SetRepresentationToSurface();

  // test point set element
  idx = getDataElementIndex(assembly, "collar");
  pds = output->GetPartitionedDataSet(idx);
  auto pse = vtkPolyData::SafeDownCast(pds->GetPartition(0));
  pse->GetPointData()->SetActiveScalars("holeid");

  vtkNew<vtkDataSetMapper> pointMapper;
  pointMapper->SetInputDataObject(pse);
  pointMapper->SetScalarRange(0, 54);

  vtkNew<vtkActor> pointActor;
  pointActor->SetMapper(pointMapper);
  pointActor->GetProperty()->SetRepresentationToSurface();

  // test line set element
  idx = getDataElementIndex(assembly, "wolfpass_WP_assay");
  pds = output->GetPartitionedDataSet(idx);
  auto lse = vtkPolyData::SafeDownCast(pds->GetPartition(0));
  lse->GetCellData()->SetActiveScalars("CU_pct");

  vtkNew<vtkDataSetMapper> lineMapper;
  lineMapper->SetInputDataObject(lse);
  lineMapper->SetScalarRange(0.02, 9.29);

  vtkNew<vtkActor> lineActor;
  lineActor->SetMapper(lineMapper);
  lineActor->GetProperty()->SetRepresentationToSurface();

  // test volume element
  idx = getDataElementIndex(assembly, "Block_Model");
  pds = output->GetPartitionedDataSet(idx);
  auto vol = vtkStructuredGrid::SafeDownCast(pds->GetPartition(0));
  vol->GetCellData()->SetActiveScalars("CU_pct");

  vtkNew<vtkDataSetMapper> volMapper;
  volMapper->SetInputDataObject(vol);
  volMapper->SetScalarRange(-0.46, 5.8);

  vtkNew<vtkActor> volActor;
  volActor->SetMapper(volMapper);
  volActor->GetProperty()->SetRepresentationToSurface();

  vtkNew<vtkRenderer> ren1;
  vtkNew<vtkRenderer> ren2;
  vtkNew<vtkRenderer> ren3;
  vtkNew<vtkRenderer> ren4;
  ren1->AddActor(surfaceActor);
  ren2->AddActor(pointActor);
  ren3->AddActor(lineActor);
  ren4->AddActor(volActor);
  ren1->SetBackground(0, 0, 0);
  ren2->SetBackground(0, 0, 0);
  ren3->SetBackground(0, 0, 0);
  ren4->SetBackground(0, 0, 0);
  // ren->ResetCamera();

  vtkNew<vtkRenderWindow> renWin;
  renWin->SetSize(300, 300);
  renWin->AddRenderer(ren1);
  renWin->AddRenderer(ren2);
  renWin->AddRenderer(ren3);
  renWin->AddRenderer(ren4);

  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renWin->Render();

  // testing surface element
  int r1 = vtkRegressionTestImage(renWin);
  if (r1 == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !r1;
}
