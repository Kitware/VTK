// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
// Test vtkAMRSliceFilter filter.

#include <vtkAMRSliceFilter.h>
#include <vtkActor.h>
#include <vtkCamera.h>
#include <vtkColorTransferFunction.h>
#include <vtkCompositeDataDisplayAttributes.h>
#include <vtkCompositePolyDataMapper.h>
#include <vtkDataObjectTreeIterator.h>
#include <vtkDataSetSurfaceFilter.h>
#include <vtkImageToAMR.h>
#include <vtkLookupTable.h>
#include <vtkNamedColors.h>
#include <vtkNew.h>
#include <vtkOverlappingAMR.h>
#include <vtkPointDataToCellData.h>
#include <vtkRTAnalyticSource.h>
#include <vtkRegressionTestImage.h>
#include <vtkRenderWindow.h>
#include <vtkRenderWindowInteractor.h>
#include <vtkRenderer.h>
#include <vtkUniformGridAMRIterator.h>

#include <array>

int TestAMRSliceFilterCellData(int argc, char* argv[])
{
  vtkNew<vtkRTAnalyticSource> imgSrc;

  vtkNew<vtkPointDataToCellData> cdSrc;
  cdSrc->SetInputConnection(imgSrc->GetOutputPort());

  vtkNew<vtkImageToAMR> amr;
  amr->SetInputConnection(cdSrc->GetOutputPort());
  amr->SetNumberOfLevels(3);

  vtkNew<vtkAMRSliceFilter> slicer;
  slicer->SetInputConnection(amr->GetOutputPort());
  slicer->SetNormal(1);
  slicer->SetOffsetFromOrigin(10);
  slicer->SetMaxResolution(2);

  vtkNew<vtkDataSetSurfaceFilter> surface;
  surface->SetInputConnection(slicer->GetOutputPort());
  surface->Update();

  // color map
  vtkNew<vtkNamedColors> colors;

  vtkNew<vtkColorTransferFunction> colormap;
  colormap->SetColorSpaceToDiverging();
  colormap->AddRGBPoint(0.0, 1.0, 0.0, 0.0);
  colormap->AddRGBPoint(1.0, 0.0, 0.0, 1.0);

  vtkNew<vtkLookupTable> lut;
  lut->SetNumberOfColors(256);
  for (int i = 0; i < lut->GetNumberOfColors(); ++i)
  {
    std::array<double, 4> color;
    colormap->GetColor(static_cast<double>(i) / lut->GetNumberOfColors(), color.data());
    color[3] = 1.0;
    lut->SetTableValue(i, color.data());
  }
  lut->Build();

  // Rendering
  vtkNew<vtkCompositePolyDataMapper> mapper;
  mapper->SetInputConnection(surface->GetOutputPort());
  mapper->SetLookupTable(lut);
  mapper->SetScalarRange(37.3531, 276.829);
  mapper->SetScalarModeToUseCellFieldData();
  mapper->SetInterpolateScalarsBeforeMapping(1);
  mapper->SelectColorArray("RTData");

  vtkNew<vtkCompositeDataDisplayAttributes> cdsa;
  mapper->SetCompositeDataDisplayAttributes(cdsa);

  int nonLeafNodes = 0;
  {
    vtkOverlappingAMR* oamr = vtkOverlappingAMR::SafeDownCast(slicer->GetOutputDataObject(0));
    vtkSmartPointer<vtkUniformGridAMRIterator> iter =
      vtkSmartPointer<vtkUniformGridAMRIterator>::New();
    iter->SetDataSet(oamr);
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      if (iter->GetCurrentLevel() < 2)
      {
        nonLeafNodes++;
      }
    }
  }

  // only show the leaf nodes
  vtkCompositeDataSet* input = vtkCompositeDataSet::SafeDownCast(surface->GetOutputDataObject(0));
  if (input)
  {
    vtkSmartPointer<vtkDataObjectTreeIterator> iter =
      vtkSmartPointer<vtkDataObjectTreeIterator>::New();
    iter->SetDataSet(input);
    iter->SkipEmptyNodesOn();
    iter->VisitOnlyLeavesOn();
    int count = 0;
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      unsigned int flatIndex = iter->GetCurrentFlatIndex();
      mapper->SetBlockVisibility(flatIndex, count > nonLeafNodes);
      count++;
    }
  }

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindow> rwin;
  rwin->AddRenderer(ren);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(rwin);

  ren->AddActor(actor);
  ren->GetActiveCamera()->SetPosition(15, 0, 0);
  ren->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  ren->ResetCamera();
  rwin->SetSize(300, 300);
  rwin->Render();

  int retVal = vtkRegressionTestImage(rwin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
