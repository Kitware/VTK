// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// This tests vtkVisibleCellSelector, vtkExtractSelectedFrustum,
// vtkRenderedAreaPicker, and vtkInteractorStyleRubberBandPick.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "vtkPolyDataMapper.h"
#include "vtkRegressionTestImage.h"
#include "vtkTestUtilities.h"

#include "vtkActor.h"
#include "vtkBitArray.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkElevationFilter.h"
#include "vtkGlyph3DMapper.h"
#include "vtkHardwareSelector.h"
#include "vtkIdTypeArray.h"
#include "vtkImageActor.h"
#include "vtkInteractorStyleRubberBandPick.h"
#include "vtkNew.h"
#include "vtkPlaneSource.h"
#include "vtkPointData.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderedAreaPicker.h"
#include "vtkRenderer.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSphereSource.h"
#include <cassert>

#include <iostream>

static vtkRenderer* renderer = nullptr;

class MyEndPickCommand : public vtkCommand
{
public:
  MyEndPickCommand()
  {
    this->Renderer = nullptr; // no reference counting
    this->Mask = nullptr;     // no reference counting
    this->DataSet = nullptr;
  }

  ~MyEndPickCommand() override = default;

  void Execute(vtkObject* vtkNotUsed(caller), unsigned long vtkNotUsed(eventId),
    void* vtkNotUsed(callData)) override
  {
    assert("pre: renderer_exists" && this->Renderer != nullptr);

    vtkNew<vtkHardwareSelector> sel;
    sel->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_POINTS);
    sel->SetRenderer(renderer);

    double x0 = renderer->GetPickX1();
    double y0 = renderer->GetPickY1();
    double x1 = renderer->GetPickX2();
    double y1 = renderer->GetPickY2();
    sel->SetArea(static_cast<unsigned int>(x0), static_cast<unsigned int>(y0),
      static_cast<unsigned int>(x1), static_cast<unsigned int>(y1));

    auto res = vtk::TakeSmartPointer(sel->Select());

#if 0
    std::cerr << "x0 " << x0 << " y0 " << y0 << "\t";
    std::cerr << "x1 " << x1 << " y1 " << y1 << std::endl;
    res->Print(std::cout);
#endif

    // Reset the mask to false.
    vtkIdType numPoints = this->Mask->GetNumberOfTuples();
    for (vtkIdType i = 0; i < numPoints; i++)
    {
      this->Mask->SetValue(i, false);
    }

    vtkSelectionNode* glyphids = res->GetNode(0);
    if (glyphids != nullptr)
    {
      vtkAbstractArray* abs = glyphids->GetSelectionList();
      if (abs == nullptr)
      {
        std::cout << "abs is null" << std::endl;
      }
      vtkIdTypeArray* ids = vtkArrayDownCast<vtkIdTypeArray>(abs);
      if (ids == nullptr)
      {
        std::cout << "ids is null" << std::endl;
      }
      else
      {
        // modify mask array with selection.
        vtkIdType numSelPoints = ids->GetNumberOfTuples();
        for (vtkIdType i = 0; i < numSelPoints; i++)
        {
          vtkIdType value = ids->GetValue(i);
          if (value >= 0 && value < numPoints)
          {
            std::cout << "Turn On: " << value << std::endl;
            this->Mask->SetValue(value, true);
          }
          else
          {
            std::cout << "Ignoring: " << value << std::endl;
          }
        }
      }
    }
    this->DataSet->Modified();
  }

  void SetRenderer(vtkRenderer* r) { this->Renderer = r; }

  vtkRenderer* GetRenderer() const { return this->Renderer; }

  void SetMask(vtkBitArray* m) { this->Mask = m; }
  void SetDataSet(vtkDataSet* ds) { this->DataSet = ds; }

protected:
  vtkRenderer* Renderer;
  vtkBitArray* Mask;
  vtkDataSet* DataSet;
};

int TestGlyph3DMapperPicking(int argc, char* argv[])
{
  int res = 6;
  vtkNew<vtkPlaneSource> plane;
  plane->SetResolution(res, res);
  vtkNew<vtkElevationFilter> colors;
  colors->SetInputConnection(plane->GetOutputPort());
  colors->SetLowPoint(-0.25, -0.25, -0.25);
  colors->SetHighPoint(0.25, 0.25, 0.25);

  vtkNew<vtkSphereSource> squad;
  squad->SetPhiResolution(25);
  squad->SetThetaResolution(25);

  vtkNew<vtkGlyph3DMapper> glypher;
  glypher->SetInputConnection(colors->GetOutputPort());
  glypher->SetScaleFactor(0.1);
  glypher->SetSourceConnection(squad->GetOutputPort());

  // selection is performed on actor1
  vtkNew<vtkActor> glyphActor1;
  glyphActor1->SetMapper(glypher);
  glyphActor1->PickableOn();

  // result of selection is on actor2
  vtkNew<vtkActor> glyphActor2;
  glyphActor2->PickableOff();
  colors->Update(); // make sure output is valid.
  auto selection = vtk::TakeSmartPointer(colors->GetOutput()->NewInstance());
  selection->ShallowCopy(colors->GetOutput());

  vtkNew<vtkBitArray> selectionMask;
  selectionMask->SetName("mask");
  selectionMask->SetNumberOfComponents(1);
  selectionMask->SetNumberOfTuples(selection->GetNumberOfPoints());
  // Initially, everything is selected
  vtkIdType i = 0;
  vtkIdType c = selectionMask->GetNumberOfTuples();
  while (i < c)
  {
    selectionMask->SetValue(i, true);
    ++i;
  }
  selection->GetPointData()->AddArray(selectionMask);

  vtkNew<vtkGlyph3DMapper> glypher2;
  glypher2->SetMasking(true);
  glypher2->SetMaskArray("mask");

  glypher2->SetInputData(selection);
  glypher2->SetScaleFactor(0.1);
  glypher2->SetSourceConnection(squad->GetOutputPort());
  glyphActor2->SetMapper(glypher2);

  // Standard rendering classes
  vtkNew<vtkRenderer> rendererNew;
  renderer = rendererNew;
  vtkNew<vtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  renWin->SetMultiSamples(0);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // set up the view
  renderer->SetBackground(0.2, 0.2, 0.2);
  renWin->SetSize(300, 300);

  // use the rubber band pick interactor style
  vtkRenderWindowInteractor* rwi = renWin->GetInteractor();
  vtkNew<vtkInteractorStyleRubberBandPick> rbp;
  rwi->SetInteractorStyle(rbp);

  vtkNew<vtkRenderedAreaPicker> areaPicker;
  rwi->SetPicker(areaPicker);

  renderer->AddActor(glyphActor1);
  renderer->AddActor(glyphActor2);
  glyphActor2->SetPosition(2, 0, 0);

  // pass pick events to the VisibleGlyphSelector
  auto* cbc = new MyEndPickCommand;
  cbc->SetRenderer(renderer);
  cbc->SetMask(selectionMask);
  cbc->SetDataSet(selection);
  rwi->AddObserver(vtkCommand::EndPickEvent, cbc);
  cbc->Delete();
  ////////////////////////////////////////////////////////////

  // run the test

  renderer->ResetCamera();

  renWin->Render();
  areaPicker->AreaPick(53, 78, 82, 273, renderer);
  cbc->Execute(nullptr, 0, nullptr);
  renWin->Render();

  int retVal = vtkRegressionTestImage(renWin);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
