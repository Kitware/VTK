// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

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

int TestCompositePolyDataMapperOverrideScalarArray(int argc, char* argv[])
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
  // colored by default color map (rainbow)
  auto polydata4 = vtk::TakeSmartPointer(::createAQuad(/*center=*/{ 1., 0., 1. }));
  // colored by cell data
  auto polydata5 = vtk::TakeSmartPointer(::createAQuad(/*center=*/{ 4., 1., 0. }));

  vtkNew<vtkFloatArray> scalars;
  scalars->SetName("scalars");
  scalars->SetNumberOfComponents(1);
  scalars->SetNumberOfTuples(4);
  for (vtkIdType i = 0; i < scalars->GetNumberOfTuples(); ++i)
  {
    scalars->SetTypedComponent(i, 0, static_cast<float>(i));
  }
  // scalars will be the active scalars only on polydata4
  polydata4->GetPointData()->SetScalars(scalars);

  vtkNew<vtkFloatArray> scalarsA;
  scalarsA->SetName("scalarsA");
  scalarsA->SetNumberOfComponents(3);
  scalarsA->SetNumberOfTuples(4);
  for (vtkIdType i = 0; i < scalarsA->GetNumberOfTuples(); ++i)
  {
    scalarsA->SetTypedComponent(i, 0, static_cast<float>(i));
    scalarsA->SetTypedComponent(i, 1, -static_cast<float>(i));
    scalarsA->SetTypedComponent(i, 2, 2 * static_cast<float>(i));
  }
  // scalarsA will be the active scalars on all polydata
  polydata0->GetPointData()->SetScalars(scalarsA);
  polydata1->GetPointData()->SetScalars(scalarsA);
  polydata2->GetPointData()->SetScalars(scalarsA);

  vtkNew<vtkFloatArray> scalarsB;
  scalarsB->SetName("scalarsB");
  scalarsB->SetNumberOfComponents(1);
  scalarsB->SetNumberOfTuples(4);
  for (vtkIdType i = 0; i < scalarsB->GetNumberOfTuples(); ++i)
  {
    scalarsB->SetTypedComponent(i, 0, static_cast<float>(i));
  }
  // scalarsB will be an extra array on the point data of all polydata
  polydata0->GetPointData()->AddArray(scalarsB);
  polydata1->GetPointData()->AddArray(scalarsB);
  polydata2->GetPointData()->AddArray(scalarsB);

  vtkNew<vtkFloatArray> scalarsC;
  scalarsC->SetName("scalarsC");
  scalarsC->SetNumberOfComponents(1);
  scalarsC->SetNumberOfTuples(4);
  for (vtkIdType i = 0; i < scalarsC->GetNumberOfTuples(); ++i)
  {
    scalarsC->SetTypedComponent(i, 0, 1.5 * static_cast<float>(i));
  }
  // scalarsC will be an extra array on the point data of all polydata
  polydata0->GetPointData()->AddArray(scalarsC);
  polydata1->GetPointData()->AddArray(scalarsC);
  polydata2->GetPointData()->AddArray(scalarsC);

  vtkNew<vtkFloatArray> scalarsD;
  scalarsD->SetName("scalarsD");
  scalarsD->SetNumberOfComponents(1);
  scalarsD->SetNumberOfTuples(2);
  scalarsD->SetValue(0, 0.0);
  scalarsD->SetValue(1, 4.0);
  // scalarsD will be an extra array on the cell data of all polydata
  polydata5->GetCellData()->AddArray(scalarsD);

  vtkNew<vtkPartitionedDataSetCollection> pdsc;
  pdsc->SetPartition(0, 0, polydata0);
  pdsc->SetPartition(1, 0, polydata1);
  pdsc->SetPartition(2, 0, polydata2);
  pdsc->SetPartition(3, 0, polydata3);
  pdsc->SetPartition(4, 0, polydata4);
  pdsc->SetPartition(5, 0, polydata5);

  pdsc->DebugOn();
  vtkDebugWithObjectMacro(pdsc, << "polydata0 " << vtkLogIdentifier(polydata0));
  vtkDebugWithObjectMacro(pdsc, << "polydata1 " << vtkLogIdentifier(polydata1));
  vtkDebugWithObjectMacro(pdsc, << "polydata2 " << vtkLogIdentifier(polydata2));
  vtkDebugWithObjectMacro(pdsc, << "polydata3 " << vtkLogIdentifier(polydata3));
  vtkDebugWithObjectMacro(pdsc, << "polydata4 " << vtkLogIdentifier(polydata4));
  vtkDebugWithObjectMacro(pdsc, << "polydata5 " << vtkLogIdentifier(polydata5));
  pdsc->DebugOff();

  vtkNew<vtkTrivialProducer> source;
  source->SetOutput(pdsc);

  // Create a base lookup table used by the mapper for arrays which do not define their own
  // lookuptable.
  vtkNew<vtkLookupTable> lut;
  lut->SetRange(scalars->GetRange());
  lut->SetNanColor(1., 1., 0., 1.);
  lut->Build();

  // create dedicated lookup tables and assign scalar ranges.
  scalarsA->CreateDefaultLookupTable();
  scalarsA->GetLookupTable()->SetRange(scalarsA->GetRange(2));
  scalarsA->GetLookupTable()->SetNumberOfTableValues(4);
  scalarsA->GetLookupTable()->SetTableValue(0, 1.0, 1.0, 1.0);
  scalarsA->GetLookupTable()->SetTableValue(1, 1.0, 0.0, 0.0);
  scalarsA->GetLookupTable()->SetTableValue(2, 1.0, 0.0, 0.0);
  scalarsA->GetLookupTable()->SetTableValue(3, 1.0, 1.0, 1.0);
  // customize scalarsB lookup table.
  scalarsB->CreateDefaultLookupTable();
  scalarsB->GetLookupTable()->SetNumberOfTableValues(2);
  scalarsB->GetLookupTable()->SetRange(scalarsB->GetRange());
  scalarsB->GetLookupTable()->SetTableValue(0, 0.0, 1.0, 0.0);
  scalarsB->GetLookupTable()->SetTableValue(1, 1.0, 1.0, 1.0);
  // customize scalarsC lookup table.
  scalarsC->CreateDefaultLookupTable();
  scalarsC->GetLookupTable()->SetNumberOfTableValues(4);
  scalarsC->GetLookupTable()->SetRange(scalarsC->GetRange());
  scalarsC->GetLookupTable()->SetTableValue(0, 0.0, 0.0, 1.0);
  scalarsC->GetLookupTable()->SetTableValue(1, 1.0, 1.0, 1.0);
  scalarsC->GetLookupTable()->SetTableValue(2, 1.0, 1.0, 1.0);
  scalarsC->GetLookupTable()->SetTableValue(3, 0.0, 0.0, 1.0);

  vtkNew<vtkCompositeDataDisplayAttributes> attributes;
  vtkNew<vtkCompositePolyDataMapper> mapper;
  mapper->SetCompositeDataDisplayAttributes(attributes);
  // setup base scalar mapping parameters to map the first array from PointData
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SetArrayAccessMode(VTK_GET_ARRAY_BY_ID);
  mapper->SetArrayId(0);
  mapper->SetUseLookupTableScalarRange(true);
  // override scalar array component for polydata0
  attributes->SetBlockArrayComponent(polydata0, 2);
  // override scalar name for polydata1
  attributes->SetBlockArrayAccessMode(polydata1, VTK_GET_ARRAY_BY_NAME);
  attributes->SetBlockArrayName(polydata1, scalarsB->GetName());
  // override scalar array ID for polydata2
  attributes->SetBlockArrayId(polydata2, 2); // scalarsC is the the third array on the pointdata.
  // override scalar array ID for polydata5
  attributes->SetBlockArrayAccessMode(polydata5, VTK_GET_ARRAY_BY_NAME);
  attributes->SetBlockScalarMode(polydata5, VTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
  attributes->SetBlockArrayName(
    polydata5, scalarsD->GetName()); // scalarsD is the the first array on the celldata.
  mapper->SetInputConnection(source->GetOutputPort());
  mapper->SetColorMissingArraysWithNanColor(true);
  mapper->SetLookupTable(lut);

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
  int retVal = vtkRegressionTestImageThreshold(renWin.GetPointer(), 15);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return EXIT_SUCCESS;
}
