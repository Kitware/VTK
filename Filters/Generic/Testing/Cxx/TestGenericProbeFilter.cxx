// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This example demonstrates how to implement a vtkGenericDataSet
// (here vtkBridgeDataSet) and to use vtkGenericProbeFilter on it.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
// -D <path> => path to the data; the data should be in <path>/Data/

#include "vtkActor.h"
#include "vtkAttributesErrorMetric.h"
#include "vtkBridgeDataSet.h"
#include "vtkDataSetMapper.h"
#include "vtkGenericCellTessellator.h"
#include "vtkGenericGeometryFilter.h"
#include "vtkGenericProbeFilter.h"
#include "vtkGeometricErrorMetric.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSimpleCellTessellator.h"
#include "vtkTestUtilities.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"
#include "vtkUnstructuredGrid.h"
#include "vtkXMLUnstructuredGridReader.h"

#include <cassert>

int TestGenericProbeFilter(int argc, char* argv[])
{
  // Standard rendering classes
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Load the mesh geometry and data from a file
  vtkNew<vtkXMLUnstructuredGridReader> reader;
  char* cfname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/quadraticTetra01.vtu");

  reader->SetFileName(cfname);
  delete[] cfname;

  // Force reading
  reader->Update();

  // Initialize the bridge
  vtkNew<vtkBridgeDataSet> ds;
  ds->SetDataSet(reader->GetOutput());

  // Set the error metric thresholds:
  // 1. for the geometric error metric
  vtkNew<vtkGeometricErrorMetric> geometricError;
  geometricError->SetRelativeGeometricTolerance(0.1, ds);

  ds->GetTessellator()->GetErrorMetrics()->AddItem(geometricError);

  // 2. for the attribute error metric
  vtkNew<vtkAttributesErrorMetric> attributesError;
  attributesError->SetAttributeTolerance(0.01);

  ds->GetTessellator()->GetErrorMetrics()->AddItem(attributesError);

  static_cast<vtkSimpleCellTessellator*>(ds->GetTessellator())->SetMaxSubdivisionLevel(10);

  // Create the probe plane
  vtkNew<vtkPlaneSource> plane;
  plane->SetResolution(100, 100);
  vtkNew<vtkTransform> transp;
  transp->Translate(0.5, 0.5, 0);
  transp->Scale(5, 5, 5);
  vtkNew<vtkTransformFilter> tpd;
  tpd->SetInputConnection(0, plane->GetOutputPort(0));
  tpd->SetTransform(transp);

  // Create the filter
  vtkNew<vtkGenericProbeFilter> probe;
  probe->SetInputConnection(0, tpd->GetOutputPort(0));
  probe->SetSourceData(ds);

  probe->Update(); // So that we can call GetRange() on the scalars

  assert(probe->GetOutput() != nullptr);

  // This creates a blue to red lut.
  vtkNew<vtkLookupTable> lut;
  lut->SetHueRange(0.667, 0.0);

  vtkNew<vtkDataSetMapper> mapper;
  mapper->SetLookupTable(lut);
  mapper->SetInputConnection(0, probe->GetOutputPort(0));

  if (probe->GetOutput()->GetPointData() != nullptr)
  {
    if (probe->GetOutput()->GetPointData()->GetScalars() != nullptr)
    {
      mapper->SetScalarRange(probe->GetOutput()->GetPointData()->GetScalars()->GetRange());
    }
  }

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  // Standard testing code.
  renderer->SetBackground(0.5, 0.5, 0.5);
  renWin->SetSize(300, 300);
  renWin->Render();
  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
