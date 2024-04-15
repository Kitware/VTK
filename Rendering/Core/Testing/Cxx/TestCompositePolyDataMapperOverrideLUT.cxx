// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// This test overrides the lookup table used for different blocks of a composite dataset.

#include "vtkAbstractMapper.h"
#include "vtkCellData.h"
#include "vtkColorTransferFunction.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositePolyDataMapper.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkLogger.h"
#include "vtkLookupTable.h"
#include "vtkMapper.h"
#include "vtkNew.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkPointData.h"
#include "vtkPoints.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkScalarsToColors.h"
#include "vtkSmartPointer.h"
#include "vtkTrivialProducer.h"
#include "vtkUnsignedCharArray.h"
#include "vtkVector.h"

namespace
{
vtkPolyData* createAQuad(vtkVector3d center = vtkVector3d{ 0., 0., 0. })
{
  auto polydata = vtkPolyData::New();
  vtkNew<vtkPoints> points;
  vtkNew<vtkCellArray> polys;
  points->InsertNextPoint(center.GetX() - 0.5, center.GetY() - 0.5, center.GetZ()); // lower-left
  points->InsertNextPoint(center.GetX() + 0.5, center.GetY() - 0.5, center.GetZ()); // lower-right
  points->InsertNextPoint(center.GetX() + 0.5, center.GetY() + 0.5, center.GetZ()); // upper-right
  points->InsertNextPoint(center.GetX() - 0.5, center.GetY() + 0.5, center.GetZ()); // upper-left
  polys->InsertNextCell({ 0, 1, 2 });
  polys->InsertNextCell({ 2, 3, 0 });
  polydata->SetPoints(points);
  polydata->SetPolys(polys);
  return polydata;
}
}

int TestCompositePolyDataMapperOverrideLUT(int argc, char* argv[])
{
  vtkNew<vtkRenderer> renderer;

  // colored with white to red gradient
  auto polydata0 = vtk::TakeSmartPointer(::createAQuad());
  // colored with green to white gradient
  auto polydata1 = vtk::TakeSmartPointer(::createAQuad(/*center=*/{ 1., 1., 0. }));
  // colored with blue to white gradient
  auto polydata2 = vtk::TakeSmartPointer(::createAQuad(/*center=*/{ 2., 0., 0. }));
  // colored by NaN color.
  auto polydata3 = vtk::TakeSmartPointer(::createAQuad(/*center=*/{ 1., -1, 0. }));
  // colored by default color map (rainbow) without interpolating scalars before mapping
  auto polydata4 = vtk::TakeSmartPointer(::createAQuad(/*center=*/{ 1., 0., 1. }));
  // colored by cell data
  auto polydata5 = vtk::TakeSmartPointer(::createAQuad(/*center=*/{ 4., 1., 0. }));
  // colored by cell data (with annotations using indexed lookup table)
  auto polydata6 = vtk::TakeSmartPointer(::createAQuad(/*center=*/{ 5., 0., 0. }));
  // colored with a solid color
  auto polydata7 = vtk::TakeSmartPointer(::createAQuad(/*center=*/{ 4., -1., 0. }));
  // colored by default color map (rainbow) with scalars interpolated before mapping
  auto polydata8 = vtk::TakeSmartPointer(::createAQuad(/*center=*/{ 4., 0., 1. }));
  // colored by red -> blue -> green -> orange with scalars interpolated before mapping
  auto polydata9 = vtk::TakeSmartPointer(::createAQuad(/*center=*/{ 3., 0., 0. }));

  vtkNew<vtkFloatArray> scalars;
  scalars->SetName("scalars");
  scalars->SetNumberOfComponents(1);
  scalars->SetNumberOfTuples(4);
  for (vtkIdType i = 0; i < scalars->GetNumberOfTuples(); ++i)
  {
    scalars->SetTypedComponent(i, 0, static_cast<float>(i));
  }
  // set active scalars on all polydata instances except polydata3
  polydata0->GetPointData()->SetScalars(scalars);
  polydata1->GetPointData()->SetScalars(scalars);
  polydata2->GetPointData()->SetScalars(scalars);
  polydata4->GetPointData()->SetScalars(scalars);
  polydata5->GetPointData()->SetScalars(scalars);
  polydata6->GetPointData()->SetScalars(scalars);
  polydata7->GetPointData()->SetScalars(scalars);
  polydata8->GetPointData()->SetScalars(scalars);
  polydata9->GetPointData()->SetScalars(scalars);

  vtkNew<vtkFloatArray> cellScalars;
  cellScalars->SetName("cellScalars");
  cellScalars->SetNumberOfComponents(1);
  cellScalars->SetNumberOfTuples(2);
  cellScalars->SetValue(0, 0.0);
  cellScalars->SetValue(1, 4.0);
  // cellScalars will be an extra array on the cell data of polydata5, polydata6
  polydata5->GetCellData()->AddArray(cellScalars);
  polydata6->GetCellData()->AddArray(cellScalars);

  vtkNew<vtkPartitionedDataSetCollection> pdsc;
  pdsc->SetPartition(0, 0, polydata0);
  pdsc->SetPartition(1, 0, polydata1);
  pdsc->SetPartition(2, 0, polydata2);
  pdsc->SetPartition(3, 0, polydata3);
  pdsc->SetPartition(4, 0, polydata4);
  pdsc->SetPartition(5, 0, polydata5);
  pdsc->SetPartition(6, 0, polydata6);
  pdsc->SetPartition(7, 0, polydata7);
  pdsc->SetPartition(8, 0, polydata8);
  pdsc->SetPartition(9, 0, polydata9);

  pdsc->DebugOn();
  vtkDebugWithObjectMacro(pdsc, << "polydata0 " << vtkLogIdentifier(polydata0));
  vtkDebugWithObjectMacro(pdsc, << "polydata1 " << vtkLogIdentifier(polydata1));
  vtkDebugWithObjectMacro(pdsc, << "polydata2 " << vtkLogIdentifier(polydata2));
  vtkDebugWithObjectMacro(pdsc, << "polydata3 " << vtkLogIdentifier(polydata3));
  vtkDebugWithObjectMacro(pdsc, << "polydata4 " << vtkLogIdentifier(polydata4));
  vtkDebugWithObjectMacro(pdsc, << "polydata5 " << vtkLogIdentifier(polydata5));
  vtkDebugWithObjectMacro(pdsc, << "polydata6 " << vtkLogIdentifier(polydata6));
  vtkDebugWithObjectMacro(pdsc, << "polydata7 " << vtkLogIdentifier(polydata7));
  vtkDebugWithObjectMacro(pdsc, << "polydata8 " << vtkLogIdentifier(polydata8));
  vtkDebugWithObjectMacro(pdsc, << "polydata9 " << vtkLogIdentifier(polydata9));
  pdsc->DebugOff();

  vtkNew<vtkTrivialProducer> source;
  source->SetOutput(pdsc);

  // Create a base lookup table used by the mapper for arrays which do not define their own
  // lookup table or blocks that do not have a lookup table specified in the
  // display attributes instance.
  vtkNew<vtkLookupTable> lut;
  lut->SetRange(scalars->GetRange());
  lut->SetNanColor(1., 1., 0., 1.);
  lut->Build();

  // white -> red -> red -> white
  vtkNew<vtkLookupTable> lutA;
  lutA->SetRange(scalars->GetRange(2));
  lutA->SetNumberOfTableValues(4);
  lutA->SetTableValue(0, 1.0, 1.0, 1.0);
  lutA->SetTableValue(1, 1.0, 0.0, 0.0);
  lutA->SetTableValue(2, 1.0, 0.0, 0.0);
  lutA->SetTableValue(3, 1.0, 1.0, 1.0);

  // green -> white
  vtkNew<vtkLookupTable> lutB;
  lutB->SetNumberOfTableValues(2);
  lutB->SetRange(scalars->GetRange());
  lutB->SetTableValue(0, 0.0, 1.0, 0.0);
  lutB->SetTableValue(1, 1.0, 1.0, 1.0);

  // blue -> white -> white -> blue
  vtkNew<vtkLookupTable> lutC;
  lutC->SetNumberOfTableValues(4);
  lutC->SetRange(scalars->GetRange());
  lutC->SetTableValue(0, 0.0, 0.0, 1.0);
  lutC->SetTableValue(1, 1.0, 1.0, 1.0);
  lutC->SetTableValue(2, 1.0, 1.0, 1.0);
  lutC->SetTableValue(3, 0.0, 0.0, 1.0);

  // green, orange
  vtkNew<vtkLookupTable> lutD;
  lutD->SetNumberOfTableValues(2);
  lutD->SetIndexedLookup(true);
  lutD->SetRange(cellScalars->GetRange());
  lutD->SetAnnotation(0.0, "Green");
  lutD->SetAnnotation(4.0, "Orange");
  lutD->SetTableValue(0, 0.0, 1.0, 0.0);
  lutD->SetTableValue(1, 1.0, 0.5, 0.0);

  // red -> green -> blue -> orange
  // a color transfer function is used for smooth interpolation
  vtkNew<vtkColorTransferFunction> lutE;
  lutE->AddRGBPoint(0, 1.0, 0.0, 0.0);
  lutE->AddRGBPoint(1, 0.0, 1.0, 0.0);
  lutE->AddRGBPoint(2, 0.0, 0.0, 1.0);
  lutE->AddRGBPoint(3, 1.0, 0.5, 0.0);

  vtkNew<vtkCompositeDataDisplayAttributes> attributes;
  attributes->SetBlockLookupTable(polydata0, lutA);
  attributes->SetBlockLookupTable(polydata1, lutB);
  attributes->SetBlockLookupTable(polydata2, lutC);
  // polydata5: override scalar array mode fo to use cell data array.
  attributes->SetBlockArrayAccessMode(polydata5, VTK_GET_ARRAY_BY_NAME);
  attributes->SetBlockScalarMode(polydata5, VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
  attributes->SetBlockArrayName(
    polydata5, cellScalars->GetName()); // cellScalars is the the first array on the celldata.
  // polydata6: override scalar array mode to use cell data array.
  const double redColor[3] = { 1., 0., 0. };
  attributes->SetBlockColor(polydata6, redColor); // the mapper should still color by scalar because
                                                  // scalar visibility is true (by default).
  attributes->SetBlockScalarMode(polydata6, VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
  // polydata6: use indexed lookup table for
  attributes->SetBlockLookupTable(polydata6, lutD);
  // polydata7: do not show scalars
  attributes->SetBlockScalarVisibility(polydata7, false);
  // polydata8: interpolate scalars before mapping. shows rainbow colors.
  attributes->SetBlockInterpolateScalarsBeforeMapping(polydata8, true);
  // polydata8: interpolate scalars before mapping. uses overridden lookup table.
  attributes->SetBlockLookupTable(polydata9, lutE);
  attributes->SetBlockInterpolateScalarsBeforeMapping(polydata9, true);

  vtkNew<vtkCompositePolyDataMapper> mapper;
  // setup base scalar mapping parameters to map the first array from PointData
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SetArrayAccessMode(VTK_GET_ARRAY_BY_ID);
  mapper->SetArrayId(0);
  mapper->SetUseLookupTableScalarRange(true);
  mapper->SetInputConnection(source->GetOutputPort());
  mapper->SetColorMissingArraysWithNanColor(true);
  mapper->SetLookupTable(lut);
  mapper->SetCompositeDataDisplayAttributes(attributes);

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(1., 1., 1.);
  renderer->AddActor(actor);

  vtkNew<vtkRenderWindowInteractor> iren;
  vtkNew<vtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  iren->SetRenderWindow(renWin);
  renWin->AddRenderer(renderer);

  renWin->SetSize(500, 500);
  renderer->GetActiveCamera()->SetPosition(0, 0, 1);
  renderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  renderer->GetActiveCamera()->SetViewUp(0, 1, 0);
  renderer->ResetCamera();

  renWin->Render();
  int retVal = vtkRegressionTestImageThreshold(renWin.GetPointer(), 0.05);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return EXIT_SUCCESS;
}
