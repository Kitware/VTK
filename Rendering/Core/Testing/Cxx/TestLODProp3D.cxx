// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// Run with -I for interactive mode. Keys:
//   l          toggle automatic LOD selection
//   n          cycle the manually selected LOD (used when automatic selection is off)
//   Up/Down    double / halve the desired update rate, i.e. shrink / grow the render
//              time budget the automatic selection has to fit into

#include "vtkAlgorithmOutput.h"
#include "vtkAppendPolyData.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkCoordinate.h"
#include "vtkCubeSource.h"
#include "vtkDecimatePro.h"
#include "vtkLODProp3D.h"
#include "vtkNew.h"
#include "vtkPLYReader.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSmartPointer.h"
#include "vtkTestUtilities.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkTransform.h"
#include "vtkTransformFilter.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

namespace
{
// Number of copies of the input each LOD is made of. Stacking many jittered copies of
// the same mesh raises the render cost without changing the shape that is drawn, and it
// keeps the relative cost of the LODs proportional to their triangle counts.
constexpr int NumberOfXCopies = 10;
constexpr int NumberOfYCopies = 10;

// Appends NumberOfCopies slightly translated copies of the geometry produced by `port`.
vtkSmartPointer<vtkAppendPolyData> StackCopies(vtkAlgorithmOutput* port, double spacing)
{
  auto append = vtkSmartPointer<vtkAppendPolyData>::New();
  for (int j = 0; j < NumberOfYCopies; ++j)
  {
    for (int i = 0; i < NumberOfXCopies; ++i)
    {
      vtkNew<vtkTransform> transform;
      transform->Translate(spacing * i, spacing * j, 0.0);
      vtkNew<vtkTransformFilter> transformFilter;
      transformFilter->SetInputConnection(port);
      transformFilter->SetTransform(transform);
      append->AddInputConnection(transformFilter->GetOutputPort());
    }
  }
  return append;
}

enum LODType
{
  FINE,
  MEDIUM_COARSE,
  COARSE,
  BBOX
};

std::tuple<vtkSmartPointer<vtkPolyDataMapper>, vtkSmartPointer<vtkProperty>> MakeLOD(
  LODType lodType, vtkPLYReader* reader)
{
  auto mapper = vtk::TakeSmartPointer(vtkPolyDataMapper::New());
  auto property = vtk::TakeSmartPointer(vtkProperty::New());
  property->SetLighting(false);

  vtkNew<vtkDecimatePro> decimator;
  decimator->SetInputConnection(reader->GetOutputPort());

  double bounds[6];
  reader->GetOutput()->GetBounds(bounds);
  const double spacing = 1 * (bounds[1] - bounds[0]);
  switch (lodType)
  {
    case FINE:
    {
      property->SetDiffuseColor(1.0, 0.0, 0.0);
      auto append = StackCopies(reader->GetOutputPort(), spacing);
      mapper->SetInputConnection(append->GetOutputPort());
      break;
    }
    case MEDIUM_COARSE:
    {
      property->SetDiffuseColor(0.0, 1.0, 0.0);
      decimator->SetTargetReduction(0.2);
      auto append = StackCopies(decimator->GetOutputPort(), spacing);
      mapper->SetInputConnection(append->GetOutputPort());
      break;
    }
    case COARSE:
    {
      property->SetDiffuseColor(0.0, 0.0, 1.0);
      property->SetLighting(false);
      decimator->SetTargetReduction(0.5);
      auto append = StackCopies(decimator->GetOutputPort(), spacing);
      mapper->SetInputConnection(append->GetOutputPort());
      break;
    }
    case BBOX:
    {
      property->SetDiffuseColor(1.0, 1.0, 1.0);
      vtkNew<vtkCubeSource> box;
      box->SetBounds(bounds);
      auto append = StackCopies(box->GetOutputPort(), spacing);
      mapper->SetInputConnection(append->GetOutputPort());
    }
    break;
  }

  return std::make_tuple(mapper, property);
}

// Everything the key callback and the status overlay need.
struct LODDemo
{
  vtkLODProp3D* Prop = nullptr;
  vtkRenderWindow* Window = nullptr;
  vtkTextActor* Text = nullptr;
  std::vector<int> Ids;
  std::vector<std::string> Names;
  int Count = 0;

  const char* NameOfID(int id) const
  {
    for (int i = 0; i < this->Count; ++i)
    {
      if (this->Ids[i] == id)
      {
        return this->Names[i].c_str();
      }
    }
    return "<none>";
  }

  int IndexOfID(int id) const
  {
    for (int i = 0; i < this->Count; ++i)
    {
      if (this->Ids[i] == id)
      {
        return i;
      }
    }
    return 0;
  }
};

// Rebuilds the overlay that reports the automatic/manual selection state, the current
// render time budget and the LOD that was actually drawn.
void UpdateStatusText(LODDemo* demo)
{
  vtkLODProp3D* prop = demo->Prop;
  const double rate = demo->Window->GetInteractor()->GetDesiredUpdateRate();
  const int manualID = prop->GetSelectedLODID();
  const int renderedID = prop->GetLastRenderedLODID();

  std::ostringstream text;
  text << std::fixed << std::setprecision(2);
  text << "[l] automatic LOD selection: " << (prop->GetAutomaticLODSelection() ? "ON" : "OFF")
       << "\n";
  text << "[n] manual LOD: " << demo->NameOfID(manualID) << " (id " << manualID << ")";
  if (prop->GetAutomaticLODSelection())
  {
    text << " [ignored while automatic is ON]";
  }
  text << "\n";
  text << "[Up/Down] desired update rate: " << rate << " fps -> budget " << 1000.0 / rate
       << " ms\n";
  text << "rendered LOD: " << demo->NameOfID(renderedID) << " (id " << renderedID << ", level "
       << prop->GetLODLevel(renderedID) << ")\n";
  text << "measured render times (ms):";
  for (int i = 0; i < demo->Count; ++i)
  {
    text << "\n    " << demo->Names[i] << ": "
         << 1000.0 * prop->GetLODEstimatedRenderTime(demo->Ids[i]);
  }

  // Only touch the actor when the string really changed, otherwise every frame would
  // rebuild the text texture.
  const std::string value = text.str();
  if (!demo->Text->GetInput() || value != demo->Text->GetInput())
  {
    demo->Text->SetInput(value.c_str());
  }
}

void KeyPressCallback(vtkObject* caller, unsigned long, void* clientData, void*)
{
  auto* interactor = static_cast<vtkRenderWindowInteractor*>(caller);
  auto* demo = static_cast<LODDemo*>(clientData);
  const char* keySym = interactor->GetKeySym();
  if (!keySym)
  {
    return;
  }

  vtkLODProp3D* prop = demo->Prop;
  if (strcmp(keySym, "l") == 0)
  {
    prop->SetAutomaticLODSelection(!prop->GetAutomaticLODSelection());
  }
  else if (strcmp(keySym, "n") == 0)
  {
    // Cycle the manual pick. It only drives the rendered LOD while automatic selection
    // is off, so 'l' is what makes this visible.
    const int next = (demo->IndexOfID(prop->GetSelectedLODID()) + 1) % demo->Count;
    prop->SetSelectedLODID(demo->Ids[next]);
  }
  else if (strstr(keySym, "Up") != nullptr || strstr(keySym, "Down") != nullptr)
  {
    // A higher update rate means less time per frame, i.e. a tighter budget for the
    // automatic LOD selection to fit into.
    double rate = demo->Window->GetInteractor()->GetDesiredUpdateRate();
    rate = (strstr(keySym, "Up") != nullptr) ? rate * 1.1 : rate * 0.9090909090909091;
    rate = std::min(std::max(rate, 1.0e-4), 1.0e6);
    demo->Window->GetInteractor()->SetDesiredUpdateRate(rate);
  }
  else
  {
    return;
  }

  UpdateStatusText(demo);
  demo->Window->Render();
}

// The rendered LOD is only known once the frame is done, so refresh after each render.
// The updated string shows up on the next frame.
void RenderEndCallback(vtkObject*, unsigned long, void* clientData, void*)
{
  UpdateStatusText(static_cast<LODDemo*>(clientData));
}
}

int TestLODProp3D(int argc, char* argv[])
{
  bool interactive = false;
  for (int i = 1; i < argc; ++i)
  {
    if (strcmp(argv[i], "-I") == 0)
    {
      interactive = true;
    }
  }

  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(300, 300);
  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(renderWindow);
  vtkNew<vtkRenderer> renderer;
  renderWindow->AddRenderer(renderer);

  char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv, "Data/bunny.ply");
  vtkNew<vtkPLYReader> reader;
  reader->SetFileName(fileName);
  delete[] fileName;
  reader->Update();

  auto [fine, fineProperty] = MakeLOD(LODType::FINE, reader);
  auto [medium, mediumProperty] = MakeLOD(LODType::MEDIUM_COARSE, reader);
  auto [coarse, coarseProperty] = MakeLOD(LODType::COARSE, reader);
  auto [bbox, bboxProperty] = MakeLOD(LODType::BBOX, reader);
  fine->GetInputAlgorithm()->Update();
  medium->GetInputAlgorithm()->Update();
  coarse->GetInputAlgorithm()->Update();
  bbox->GetInputAlgorithm()->Update();

  std::cout << "cells per LOD: fine=" << fine->GetInput()->GetNumberOfCells()
            << ", medium=" << medium->GetInput()->GetNumberOfCells()
            << ", coarse=" << coarse->GetInput()->GetNumberOfCells()
            << ", bbox=" << bbox->GetInput()->GetNumberOfCells() << std::endl;

  vtkNew<vtkLODProp3D> lodProp3D;
  // The initial estimated render time is 0.0 for every LOD: vtkLODProp3D then renders
  // each one at least once to measure how expensive it actually is.
  const int fineID = lodProp3D->AddLOD(fine, fineProperty, 0.0);
  const int mediumID = lodProp3D->AddLOD(medium, mediumProperty, 0.0);
  const int coarseID = lodProp3D->AddLOD(coarse, coarseProperty, 0.0);
  const int boxID = lodProp3D->AddLOD(bbox, bboxProperty, 0.0);

  // Lower level means better resolution.
  lodProp3D->SetLODLevel(fineID, 0.0);
  lodProp3D->SetLODLevel(mediumID, 1.0);
  lodProp3D->SetLODLevel(coarseID, 2.0);
  lodProp3D->SetLODLevel(boxID, 3.0);

  const std::vector<int> ids = { fineID, mediumID, coarseID, boxID };
  const std::vector<std::string> names = { "fine", "medium", "coarse", "box" };
  const std::vector<vtkSmartPointer<vtkProperty>> properties = { fineProperty, mediumProperty,
    coarseProperty, bboxProperty };

  renderer->AddActor(lodProp3D);
  renderer->ResetCamera();

  lodProp3D->AutomaticLODSelectionOff();
  bool success = true;
  for (std::size_t i = 0; i < 4; ++i)
  {
    lodProp3D->SetSelectedLODID(ids[i]);
    // warmup frames so the LOD's estimated render time has settled to a good stable value.
    for (std::size_t j = 0; j < 5; ++j)
    {
      renderer->GetActiveCamera()->Azimuth(36);
      renderer->ResetCameraClippingRange();
      renderWindow->Render();
    }
    int probe[2] = { 140, 58 };
    float* values =
      renderWindow->GetRGBAPixelData(probe[0], probe[1], probe[0], probe[1], /*front= */ 1);
    double expectedColor[3];
    properties[i]->GetDiffuseColor(expectedColor);
    if (expectedColor[0] != values[0] || expectedColor[1] != values[1] ||
      expectedColor[2] != values[2])
    {
      std::cout << names[i] << ": probeColor @ pixel " << probe[0] << ',' << probe[1] << "("
                << values[0] << "," << values[1] << "," << values[2] << ") != expectedColor ("
                << expectedColor[0] << "," << expectedColor[1] << "," << expectedColor[2] << ")\n";
      success &= false;
    }
  }
  lodProp3D->AutomaticLODSelectionOn();

  const auto originalUpdateRate = renderWindow->GetDesiredUpdateRate();
  // Give lots of time and we should see that fine LOD was rendered.
  renderWindow->SetDesiredUpdateRate(1e-6);
  renderWindow->Render();
  if (lodProp3D->GetLastRenderedLODID() != fineID)
  {
    std::cerr << "With a low desired update rate, fine LOD should have been rendered.\n";
    success &= false;
  }
  std::cout << "lod: " << lodProp3D->GetLastRenderedLODID() << " took "
            << lodProp3D->GetLODEstimatedRenderTime(lodProp3D->GetLastRenderedLODID()) << " ms\n";
  // Give very very little time, and we should see that the bbox LOD was rendered.
  renderWindow->SetDesiredUpdateRate(1e9);
  renderWindow->Render();
  if (lodProp3D->GetLastRenderedLODID() == fineID)
  {
    std::cerr << "With a low desired update rate, fine LOD should not have been rendered.\n";
    success &= false;
  }
  std::cout << "lod: " << lodProp3D->GetLastRenderedLODID() << " took "
            << lodProp3D->GetLODEstimatedRenderTime(lodProp3D->GetLastRenderedLODID()) << " ms\n";
  renderWindow->SetDesiredUpdateRate(originalUpdateRate);

  if (interactive)
  {
    // Status overlay, middle left.
    vtkNew<vtkTextActor> statusActor;
    statusActor->GetTextProperty()->SetFontSize(16);
    statusActor->GetTextProperty()->SetColor(1.0, 1.0, 1.0);
    statusActor->GetTextProperty()->SetJustificationToLeft();
    statusActor->GetTextProperty()->SetVerticalJustificationToCentered();
    statusActor->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
    statusActor->GetPositionCoordinate()->SetValue(0.02, 0.5);
    renderer->AddViewProp(statusActor);

    auto* demo = new LODDemo();
    demo->Prop = lodProp3D;
    demo->Window = renderWindow;
    demo->Text = statusActor;
    demo->Ids = ids;
    demo->Names = names;
    demo->Count = 4;

    vtkNew<vtkCallbackCommand> keyPressCallback;
    keyPressCallback->SetCallback(KeyPressCallback);
    keyPressCallback->SetClientData(demo);
    interactor->AddObserver(vtkCommand::KeyPressEvent, keyPressCallback);

    vtkNew<vtkCallbackCommand> renderEndCallback;
    renderEndCallback->SetCallback(RenderEndCallback);
    renderEndCallback->SetClientData(demo);
    renderEndCallback->SetClientDataDeleteCallback(
      [](void* ptr)
      {
        auto* demoPtr = static_cast<LODDemo*>(ptr);
        delete demoPtr;
      });
    renderWindow->AddObserver(vtkCommand::EndEvent, renderEndCallback);

    std::cout << "Keys: 'l' toggles automatic LOD selection, 'n' cycles the manual LOD, "
                 "Up/Down change the desired update rate."
              << std::endl;
    interactor->SetDesiredUpdateRate(60);
    UpdateStatusText(demo);
    renderWindow->SetSize(1920, 1080);
    renderWindow->Render();
    interactor->Start();
  }

  return success ? EXIT_SUCCESS : EXIT_FAILURE;
}
