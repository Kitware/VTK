// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
#include <vtkCamera.h>
#include <vtkCellData.h>
#include <vtkCompositeDataSet.h>
#include <vtkCompositeDataSetRange.h>
#include <vtkCompositePolyDataMapper.h>
#include <vtkDataArraySelection.h>
#include <vtkDataObject.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkIOSSReader.h>
#include <vtkLogger.h>
#include <vtkNew.h>
#include <vtkPointData.h>
#include <vtkProperty.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkTestUtilities.h>

static std::string GetFileName(int argc, char* argv[], const std::string& fnameC)
{
  char* fileNameC = vtkTestUtilities::ExpandDataFileName(argc, argv, fnameC.c_str());
  std::string fname(fileNameC);
  delete[] fileNameC;
  return fname;
}

int TestIOSSCGNS(int argc, char* argv[])
{
  auto fname = GetFileName(argc, argv, std::string("Data/CGNS/fluid.cgns.4.0"));
  vtkNew<vtkIOSSReader> reader;
  reader->ReadIdsOn();
  reader->AddFileName(fname.c_str());
  reader->GenerateFileIdOn();
  reader->UpdateInformation();
  reader->GetSideSetSelection()->EnableAllArrays();

  reader->Update();
  for (auto dObj : vtk::Range(vtkCompositeDataSet::SafeDownCast(reader->GetOutputDataObject(0))))
  {
    auto ds = vtkDataSet::SafeDownCast(dObj);
    if (ds->GetCellData()->GetArray("file_id") == nullptr)
    {
      vtkLogF(ERROR, "missing 'file_id'");
      return EXIT_FAILURE;
    }
    if (ds->GetCellData()->GetArray("cell_ids") == nullptr)
    {
      vtkLogF(ERROR, "missing 'cell_ids'");
      return EXIT_FAILURE;
    }
    if (ds->GetPointData()->GetArray("cell_node_ids") == nullptr)
    {
      vtkLogF(ERROR, "missing 'cell_node_ids'");
      return EXIT_FAILURE;
    }
  }

  vtkNew<vtkDataSetSurfaceFilter> surface;
  vtkNew<vtkCompositePolyDataMapper> mapper;
  vtkNew<vtkActor> actor;
  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindowInteractor> iren;

  surface->SetInputConnection(reader->GetOutputPort());
  mapper->SetInputConnection(surface->GetOutputPort());
  actor->SetMapper(mapper);
  actor->GetProperty()->EdgeVisibilityOn();
  renWin->AddRenderer(ren);
  iren->SetRenderWindow(renWin);

  ren->AddActor(actor);
  renWin->SetSize(300, 300);
  auto cam = ren->GetActiveCamera();
  cam->SetPosition(10., 10., 5.);
  cam->SetViewUp(0., 0.4, 1.);
  ren->ResetCamera();
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
