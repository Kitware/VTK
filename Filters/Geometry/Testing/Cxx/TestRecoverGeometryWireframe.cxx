// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCellData.h"
#include "vtkCompositeDataSet.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkDataSet.h"
#include "vtkDataSetSurfaceFilter.h"
#include "vtkOpenGLRenderer.h"
#include "vtkPartitionedDataSet.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRecoverGeometryWireframe.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkTestUtilities.h"
#include "vtkUnsignedCharArray.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLUnstructuredGridReader.h"

// ----------------------------------------------------------------------------
int TestRecoverGeometryWireframe(int argc, char* argv[])
{
  char* filename = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/quadraticTetra01.vtu");

  vtkSmartPointer<vtkXMLUnstructuredGridReader> reader =
    vtkSmartPointer<vtkXMLUnstructuredGridReader>::New();
  reader->SetFileName(filename);
  delete[] filename;

  vtkNew<vtkDataSetSurfaceFilter> dsSurface;
  dsSurface->SetInputConnection(reader->GetOutputPort());
  dsSurface->PassThroughCellIdsOn();
  dsSurface->SetOriginalCellIdsName("MyOriginalCellIds");
  dsSurface->SetNonlinearSubdivisionLevel(2);
  dsSurface->Update();

  vtkNew<vtkRecoverGeometryWireframe> recover;
  recover->SetInputData(dsSurface->GetOutput());
  recover->SetCellIdsAttribute("MyOriginalCellIds");
  recover->Update();

  vtkPolyData* output = recover->GetOutput();
  vtkNew<vtkCompositePolyDataMapper> mapper;
  mapper->SetInputDataObject(output);

  vtkNew<vtkActor> surfActor;
  surfActor->SetMapper(mapper);
  surfActor->GetProperty()->SetRepresentationToSurface();
  surfActor->GetProperty()->SetEdgeVisibility(true);
  surfActor->GetProperty()->SetEdgeColor(0, 0, 1);

  vtkNew<vtkRenderer> renderer;
  renderer->AddActor(surfActor);

  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);

  vtkNew<vtkRenderWindowInteractor> iren;
  renWin->SetInteractor(iren);

  renderer->ResetCamera();
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
