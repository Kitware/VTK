// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkColorTransferFunction.h"
#include "vtkCommand.h"
#include "vtkCompositeDataDisplayAttributes.h"
#include "vtkCompositePolyDataMapper.h"
#include "vtkElevationFilter.h"
#include "vtkGroupDataSetsFilter.h"
#include "vtkInteractorStyleSwitch.h"
#include "vtkLightKit.h"
#include "vtkPartitionedDataSetCollection.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTesting.h"

#include <cstdlib>
#include <string>

int TestCompositePolyDataMapperToggleScalarVisibilities(int argc, char* argv[])
{
  vtkNew<vtkGroupDataSetsFilter> spheres;
  spheres->SetOutputTypeToPartitionedDataSetCollection();
  double scalarRange[2] = { 0.0, 100.0 };
  for (int x = 0; x < 4; ++x)
  {
    vtkNew<vtkSphereSource> sphere;
    vtkNew<vtkElevationFilter> elev;
    sphere->SetCenter(x, 0, 0);
    sphere->SetRadius(0.5);
    elev->SetLowPoint(x, -0.5, 0.0);
    elev->SetHighPoint(x, 0.5, 0.0);
    elev->SetScalarRange(scalarRange);
    elev->SetInputConnection(sphere->GetOutputPort());
    spheres->AddInputConnection(elev->GetOutputPort());
  }
  vtkNew<vtkCompositePolyDataMapper> mapper;
  vtkNew<vtkCompositeDataDisplayAttributes> cda;
  mapper->SetCompositeDataDisplayAttributes(cda);
  mapper->SetInputConnection(spheres->GetOutputPort());
  mapper->SetScalarRange(scalarRange);

  vtkNew<vtkColorTransferFunction> ctf;
  ctf->AddRGBPoint(scalarRange[0], 0.09, 0.48, 0.97);
  ctf->AddRGBPoint(scalarRange[1], 0.447, 1., 0.384);
  mapper->SetLookupTable(ctf);
  mapper->UseLookupTableScalarRangeOn();

  vtkNew<vtkActor> actor;
  actor->SetMapper(mapper);

  vtkNew<vtkRenderer> renderer;
  renderer->AutomaticLightCreationOff();

  vtkNew<vtkLightKit> lights;
  lights->AddLightsToRenderer(renderer);
  renderer->AddActor(actor);
  actor->GetProperty()->SetPointSize(4);

  vtkNew<vtkRenderWindow> window;
  window->AddRenderer(renderer);
  window->SetSize(1280, 720);
  window->Render();

  static int selectedSphere = -1;

  vtkNew<vtkCallbackCommand> charCallback;
  charCallback->SetClientData(mapper);
  charCallback->SetCallback(
    [](vtkObject* caller, unsigned long, void* clientdata, void*)
    {
      auto istyle = reinterpret_cast<vtkInteractorStyleSwitch*>(caller);
      auto* interactor = istyle->GetCurrentStyle()->GetInteractor();
      if (istyle == nullptr)
      {
        std::cerr << "istyle is null!\n";
        return;
      }
      if (interactor == nullptr)
      {
        std::cerr << "interactor is null!\n";
        return;
      }
      auto inputMapper = reinterpret_cast<vtkCompositePolyDataMapper*>(clientdata);
      if (inputMapper == nullptr)
      {
        std::cerr << "Mapper is null!\n";
        return;
      }
      auto* inputCda = inputMapper->GetCompositeDataDisplayAttributes();
      auto keyCode = interactor->GetKeyCode();
      switch (keyCode)
      {
        case 'n':
        case 'N':
          ++selectedSphere;
          break;
        default:
          istyle->OnChar();
          return;
      }
      if (auto* compositeMesh =
            vtkPartitionedDataSetCollection::SafeDownCast(inputMapper->GetInputDataObject(0, 0)))
      {
        const unsigned int n = compositeMesh->GetNumberOfPartitionedDataSets();
        selectedSphere %= (n + 1);
        std::string text = "Selected sphere: " + std::to_string(selectedSphere);
        if (auto* mesh = compositeMesh->GetPartitionAsDataObject(selectedSphere, 0))
        {
          const auto flatIndex = compositeMesh->GetCompositeIndex(selectedSphere, 0);
          std::cout << "Turn off scalar visibility for sphere at flat index: " << flatIndex << '\n';
          inputCda->RemoveBlockScalarVisibilities();
          inputCda->SetBlockScalarVisibility(mesh, false);
          double color[3] = { 1.0, 1.0, 0.0 };
          inputCda->SetBlockColor(mesh, color);
          interactor->GetRenderWindow()->Render();
        }
        else
        {
          std::cout << "Color all spheres using scalars\n";
          inputCda->RemoveBlockScalarVisibilities();
          inputCda->RemoveBlockColors();
          interactor->GetRenderWindow()->Render();
        }
      }
    });

  vtkNew<vtkRenderWindowInteractor> interactor;
  interactor->SetRenderWindow(window);
  interactor->GetInteractorStyle()->AddObserver(vtkCommand::CharEvent, charCallback);

  vtkNew<vtkTesting> testing;
  testing->AddArguments(argc, argv);
  if (testing->IsInteractiveModeSpecified())
  {
    interactor->Start();
    return EXIT_SUCCESS;
  }
  std::string validImageFileName;
  if (testing->IsValidImageSpecified())
  {
    validImageFileName = testing->GetValidImageFileName();
  }
  else
  {
    std::cerr << "Please specify a valid image filename with -V argument.\n";
    return EXIT_FAILURE;
  }
  // Now test toggling scalar visibility of each block.
  // The last iteration covers the case where scalar visibility override
  // gets removed for all blocks.
  interactor->SetEventPosition(1, 1);
  interactor->SetControlKey(0);
  interactor->SetShiftKey(0);
  interactor->SetAltKey(0);
  interactor->SetKeyCode('n');
  interactor->SetRepeatCount(0);
  interactor->SetKeySym("n");
  testing->SetRenderWindow(window);

  for (int i = 0; i < 5; ++i)
  {
    std::string newValidImageFileName = validImageFileName;
    if (i > 0)
    {
      auto pos = newValidImageFileName.find(".png");
      std::string suffix = "_" + std::to_string(i) + ".png";
      newValidImageFileName.replace(pos, 6, suffix);
    }
    // Replace the -V argument image name with incremented suffix
    testing->CleanArguments();
    for (int j = 1; j < argc; ++j)
    {
      if (std::string(argv[j]) == "-V")
      {
        testing->AddArgument(argv[j]);
        testing->AddArgument(newValidImageFileName.c_str());
        ++j; // skip the next arg.
      }
      else
      {
        testing->AddArgument(argv[j]);
      }
    }
    interactor->InvokeEvent(vtkCommand::KeyPressEvent);
    interactor->InvokeEvent(vtkCommand::CharEvent);
    interactor->InvokeEvent(vtkCommand::KeyReleaseEvent);
    if (testing->RegressionTest(0.05, cout) == vtkTesting::FAILED)
    {
      return EXIT_FAILURE;
    }
  }
  return EXIT_SUCCESS;
}
