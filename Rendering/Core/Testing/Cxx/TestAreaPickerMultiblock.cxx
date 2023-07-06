/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestAreaPickerMultiblock.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This tests vtkHardwareSelector, vtkExtractSelectedFrustum,
// vtkRenderedAreaPicker, and vtkInteractorStyleRubberBandPick.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkAbstractArray.h"
#include "vtkBoundingBox.h"
#include "vtkCompositeDataSet.h"
#include "vtkDataSetAttributes.h"
#include "vtkInformation.h"
#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkDataSetReader.h"
#include "vtkExtractSelectedFrustum.h"
#include "vtkExtractSelection.h"
#include "vtkHardwareSelector.h"
#include "vtkInteractorStyleRubberBandPick.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderedAreaPicker.h"
#include "vtkRenderer.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkXMLMultiBlockDataReader.h"

static vtkSmartPointer<vtkRenderer> renderer;

static void EndPick(vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(eventId), void*, void*)
{
  vtkNew<vtkHardwareSelector> sel;
  sel->SetRenderer(renderer);

  double x0 = renderer->GetPickX1();
  double y0 = renderer->GetPickY1();
  double x1 = renderer->GetPickX2();
  double y1 = renderer->GetPickY2();

  sel->SetArea(
    static_cast<int>(x0), static_cast<int>(y0), static_cast<int>(x1), static_cast<int>(y1));
  vtkSmartPointer<vtkSelection> res;
  res.TakeReference(sel->Select());
  if (!res)
  {
    std::cout << "Selection not supported." << endl;
    return;
  }

  std::cout << "x0 " << x0 << " y0 " << y0 << "\t";
  std::cout << "x1 " << x1 << " y1 " << y1 << endl;

  // res->Print(std::cout);
  const auto numSelected = res->GetNumberOfNodes();
  for (unsigned int i = 0; i < numSelected; ++i)
  {
    auto node = res->GetNode(i);
    auto properties = node->GetProperties();
    const auto compositeIdx = properties->Get(vtkSelectionNode::COMPOSITE_INDEX());
    auto prop = properties->Get(vtkSelectionNode::PROP());
    auto actor = vtkActor::SafeDownCast(prop);
    auto cpdm = vtkCompositePolyDataMapper::SafeDownCast(actor->GetMapper());
    auto inputDataset = vtkCompositeDataSet::SafeDownCast(cpdm->GetInputDataObject(0, 0));
    vtkPolyData* polydata = vtkPolyData::SafeDownCast(inputDataset->GetDataSet(compositeIdx));
    const auto& numCells = polydata->GetNumberOfCells();
    const auto& numSelectedCells =
      node->GetSelectionData()->GetArray("SelectedIds")->GetNumberOfValues();
    std::cout << "numCells: " << numCells << " numSelectedCells: " << numSelectedCells << std::endl;
    double polyDataBounds[6];
    polydata->GetCellsBounds(polyDataBounds);
    double selectionBoundsWindowCoordinates[6];
    double selectionBoundsModelCoordinates[6];
    vtkBoundingBox polyDataBBox(polyDataBounds);
    vtkBoundingBox selectionBBox(selectionBoundsModelCoordinates);
    if (selectionBBox.Contains(polyDataBBox))
    {
      // accept this node with index i.
    }
    else
    {
      // remove this node from the result.
    }
  }
  // vtkSelectionNode* cellids = res->GetNode(0);
  // if (auto selectionList = cellids->GetSelectionList())
  // {
  //   selectionList->Print(std::cout);
  // }
}

int TestAreaPickerMultiblock(int argc, char* argv[])
{
  // Standard rendering classes
  renderer = vtkSmartPointer<vtkRenderer>::New();
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // set up the view
  renderer->GetActiveCamera()->SetPosition(1.5, -0.75, 7);
  renderer->GetActiveCamera()->SetFocalPoint(1.5, -0.75, 0);
  renderer->GetActiveCamera()->SetViewUp(0, 1, 0);
  renderer->SetBackground(0.0, 0.0, 0.0);
  renWin->SetSize(300, 300);

  // use the rubber band pick interactor style
  vtkRenderWindowInteractor* rwi = renWin->GetInteractor();
  vtkNew<vtkInteractorStyleRubberBandPick> rbp;
  rwi->SetInteractorStyle(rbp);

  vtkNew<vtkRenderedAreaPicker> areaPicker;
  rwi->SetPicker(areaPicker);

  ////////////////////////////////////////////////////////////
  // Create a unstructured grid data source to test FrustumExtractor with.
  vtkNew<vtkXMLMultiBlockDataReader> reader;
  char* cfname = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/mixed-mb.vtm");
  reader->SetFileName(cfname);
  delete[] cfname;

  vtkNew<vtkCompositePolyDataMapper> mapper;
  mapper->SetInputConnection(reader->GetOutputPort());

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);
  renderer->AddActor(actor);
  // pass pick events to the HardwareSelector
  vtkNew<vtkCallbackCommand> cbc;
  cbc->SetCallback(EndPick);
  cbc->SetClientData(renderer);
  rwi->AddObserver(vtkCommand::EndPickEvent, cbc);

  ////////////////////////////////////////////////////////////

  // run the test

  renWin->Render();
  iren->Start();

  renderer = nullptr;

  // Cleanup
  return 0;
}
