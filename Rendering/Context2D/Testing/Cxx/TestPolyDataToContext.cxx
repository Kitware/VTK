/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPolyDataToContext.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkAxis.h"
#include "vtkAbstractMapper.h"
#include "vtkBandedPolyDataContourFilter.h"
#include "vtkBoundingBox.h"
#include "vtkCellData.h"
#include "vtkInteractiveArea.h"
#include "vtkInteractorStyle.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkFeatureEdges.h"
#include "vtkFloatArray.h"
#include "vtkLookupTable.h"
#include "vtkNew.h"
#include "vtkPen.h"
#include "vtkPointData.h"
#include "vtkPolyDataConnectivityFilter.h"
#include "vtkPolyDataItem.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkTextProperty.h"
#include "vtkTestUtilities.h"
#include "vtkXMLPolyDataReader.h"


//------------------------------------------------------------------------------
vtkSmartPointer<vtkXMLPolyDataReader> ReadUVCDATPolyData(int argc, char* argv[])
{
  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv,
    "Data/isofill_0.vtp");
  vtkSmartPointer<vtkXMLPolyDataReader> reader =
    vtkSmartPointer<vtkXMLPolyDataReader>::New();
  reader->SetFileName(fileName);
  reader->Update();

  return reader;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkPolyDataItem> CreateMapItem(int argc, char* argv[])
{
  vtkSmartPointer<vtkXMLPolyDataReader> reader = ReadUVCDATPolyData(argc, argv);
  vtkPolyData* poly = reader->GetOutput();

  // Select point/cell data
  double range[2];
  int scalarMode = VTK_SCALAR_MODE_USE_POINT_DATA; // VTK_SCALAR_MODE_USE_CELL_DATA

  vtkDataArray* activeData = scalarMode == VTK_SCALAR_MODE_USE_POINT_DATA ?
    poly->GetPointData()->GetScalars() : poly->GetCellData()->GetScalars();
  activeData->GetRange(range, 0);

  // Map scalars
  vtkLookupTable* colorLut = activeData->GetLookupTable();
  if (!colorLut)
  {
    activeData->CreateDefaultLookupTable();
    colorLut = activeData->GetLookupTable();
    colorLut->SetAlpha(1.0);
    colorLut->SetRange(range[0], range[1]);
  }
  vtkUnsignedCharArray* mappedColors = colorLut->MapScalars(activeData,
    VTK_COLOR_MODE_DEFAULT, 0);

  // Setup item
  vtkSmartPointer<vtkPolyDataItem> polyItem =
    vtkSmartPointer<vtkPolyDataItem>::New();
  polyItem->SetPolyData(poly);
  polyItem->SetScalarMode(scalarMode);
  polyItem->SetMappedColors(mappedColors);
  mappedColors->Delete();

  return polyItem;
}

//------------------------------------------------------------------------------
vtkSmartPointer<vtkPolyDataItem> CreateContourItem(int argc, char* argv[])
{
  vtkSmartPointer<vtkXMLPolyDataReader> reader = ReadUVCDATPolyData(argc, argv);
  vtkNew<vtkBandedPolyDataContourFilter> contour;
  contour->SetInputConnection(reader->GetOutputPort());
  contour->GenerateValues(20, 6, 40);
  contour->ClippingOn();
  contour->SetClipTolerance(0.);
  contour->Update();

  vtkNew<vtkPolyDataConnectivityFilter> connectivity;
  connectivity->SetInputConnection(contour->GetOutputPort());
  connectivity->SetExtractionModeToAllRegions();
  connectivity->ColorRegionsOn();
  connectivity->Update();

  vtkNew<vtkPolyDataConnectivityFilter> extract;
  extract->SetInputConnection(connectivity->GetOutputPort());
  extract->ScalarConnectivityOn();
  extract->SetScalarRange(6, 58);

  vtkNew<vtkFeatureEdges> edge;
  edge->SetInputConnection(extract->GetOutputPort());
  edge->BoundaryEdgesOn();
  edge->FeatureEdgesOff();
  edge->ManifoldEdgesOff();
  edge->NonManifoldEdgesOff();
  edge->Update();

  // Select point/cell data
  double range[2];
  int scalarMode = VTK_SCALAR_MODE_USE_CELL_DATA;

  vtkPolyData* poly = edge->GetOutput();
  vtkDataArray* activeData = scalarMode == VTK_SCALAR_MODE_USE_POINT_DATA ?
    poly->GetPointData()->GetScalars() : poly->GetCellData()->GetScalars();
  activeData->GetRange(range, 0);

  // Map scalars
  vtkLookupTable* colorLut = activeData->GetLookupTable();
  if (!colorLut)
  {
    activeData->CreateDefaultLookupTable();
    colorLut = activeData->GetLookupTable();
    colorLut->SetAlpha(1.0);
    colorLut->SetRange(range[0], range[1]);
  }
  vtkUnsignedCharArray* mappedColors = colorLut->MapScalars(activeData,
    VTK_COLOR_MODE_DEFAULT, 0);

  // Setup item
  vtkSmartPointer<vtkPolyDataItem> polyItem =
    vtkSmartPointer<vtkPolyDataItem>::New();
  polyItem->SetPolyData(poly);
  polyItem->SetScalarMode(scalarMode);
  polyItem->SetMappedColors(mappedColors);
  mappedColors->Delete();

  return polyItem;
}

/**
 * Tests vtkPolyDataItem and shows its usage with an example. vtkPolyDataItem
 * renders vtkPolyData primitives into a vtkContextScene directly (without the
 * need of a vtkMapper).
 */

///////////////////////////////////////////////////////////////////////////////
int TestPolyDataToContext( int argc, char * argv [] )
{
  // Set up a 2D context view, context test object and add it to the scene
  vtkNew<vtkContextView> view;
  view->GetRenderer()->SetBackground(0.3, 0.3, 0.3);
  view->GetRenderWindow()->SetSize(600, 400);
  view->GetRenderWindow()->SetMultiSamples(0);
  view->GetInteractor()->Initialize();
  view->GetInteractor()->GetInteractorStyle()->SetCurrentRenderer(view->GetRenderer());

  // Create the container item that handles view transform (aspect, interaction,
  // etc.)
  vtkNew<vtkInteractiveArea> area;

  vtkSmartPointer<vtkPolyDataItem> mapItem = CreateMapItem(argc, argv);
  vtkSmartPointer<vtkPolyDataItem> contourItem = CreateContourItem(argc, argv);
  area->GetDrawAreaItem()->AddItem(mapItem);
  area->GetDrawAreaItem()->AddItem(contourItem);

  vtkBoundingBox bounds(mapItem->GetPolyData()->GetBounds());
  area->SetDrawAreaBounds(vtkRectd(bounds.GetBound(0), bounds.GetBound(2),
                                   bounds.GetLength(0), bounds.GetLength(1)));
  area->SetFixedAspect(bounds.GetLength(0) / bounds.GetLength(1));

  area->GetAxis(vtkAxis::BOTTOM)->SetTitle("X Axis");
  area->GetAxis(vtkAxis::LEFT)->SetTitle("Y Axis");
  area->GetAxis(vtkAxis::TOP)->SetVisible(false);
  area->GetAxis(vtkAxis::RIGHT)->SetVisible(false);
  for (int i = 0; i < 4; ++i)
  {
    vtkAxis *axis = area->GetAxis(static_cast<vtkAxis::Location>(i));
    axis->GetLabelProperties()->SetColor(.6, .6, .9);
    axis->GetTitleProperties()->SetColor(.6, .6, .9);
    axis->GetPen()->SetColor(.6 * 255, .6 * 255, .9 * 255, 255);
    axis->GetGridPen()->SetColor(.6 * 255, .6 * 255, .9 * 255, 128);
  }

  // Turn off the color buffer
  view->GetScene()->SetUseBufferId(false);
  view->GetScene()->AddItem(area.GetPointer());
  view->Render();

  int retVal = vtkRegressionTestImage(view->GetRenderWindow());
  if(retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    view->GetInteractor()->Initialize();
    view->GetInteractor()->Start();
  }

  return !retVal;
}
