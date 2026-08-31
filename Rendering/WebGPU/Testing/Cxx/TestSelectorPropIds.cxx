// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

// The prop a selection names must be the prop that was drawn there.
//
// The ids written to the ids attachment are indices into the renderer's list of
// props to render, and that list is not the renderer's list of visible props: a
// skybox is held separately as the background prop and never enters it. A
// selector that numbers the visible props itself is therefore off by one for
// every prop behind a skybox - it reports the neighbour.
//
// Two spheres far apart, a skybox behind them, and a pick on each: each pick has
// to name the sphere it landed on.

#include "vtkActor.h"
#include "vtkDataObject.h"
#include "vtkHardwareSelector.h"
#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProp.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSkybox.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkTexture.h"

#include <algorithm>
#include <cstdlib>
#include <iostream>

namespace
{
vtkSmartPointer<vtkActor> MakeSphere(double x, const double rgb[3])
{
  vtkNew<vtkSphereSource> source;
  source->SetCenter(x, 0.0, 0.0);
  source->SetRadius(0.8);
  source->SetThetaResolution(16);
  source->SetPhiResolution(16);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(source->GetOutputPort());

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(rgb[0], rgb[1], rgb[2]);
  return actor;
}

// Returns the prop the selector reports at a display position, or nullptr.
vtkProp* PickAt(vtkRenderer* renderer, unsigned int x, unsigned int y)
{
  vtkNew<vtkHardwareSelector> selector;
  selector->SetRenderer(renderer);
  selector->SetFieldAssociation(vtkDataObject::FIELD_ASSOCIATION_CELLS);
  selector->SetArea(x, y, x, y);
  vtkSelection* selection = selector->Select();
  vtkProp* prop = nullptr;
  if (selection != nullptr)
  {
    if (selection->GetNumberOfNodes() > 0)
    {
      prop = vtkProp::SafeDownCast(
        selection->GetNode(0)->GetProperties()->Get(vtkSelectionNode::PROP()));
    }
    selection->Delete();
  }
  return prop;
}
}

int TestSelectorPropIds(int, char*[])
{
  const double red[3] = { 1.0, 0.0, 0.0 };
  const double blue[3] = { 0.2, 0.4, 1.0 };
  vtkSmartPointer<vtkActor> left = MakeSphere(-1.4, red);
  vtkSmartPointer<vtkActor> right = MakeSphere(1.4, blue);

  vtkNew<vtkRenderer> renderer;

  // The skybox goes in first, and blurred, which is what makes the two prop
  // lists disagree: vtkRenderer diverts a blurred skybox to BackgroundProp, out
  // of the list the ids index into, while it stays among the renderer's view
  // props. Everything drawn after it therefore carries an id one lower than its
  // position in that list.
  vtkNew<vtkImageData> skyImage;
  skyImage->SetDimensions(4, 4, 1);
  skyImage->AllocateScalars(VTK_UNSIGNED_CHAR, 3);
  auto* skyPixels = static_cast<unsigned char*>(skyImage->GetScalarPointer());
  std::fill_n(skyPixels, 4 * 4 * 3, static_cast<unsigned char>(64));
  vtkNew<vtkTexture> texture;
  texture->SetInputData(skyImage);
  vtkNew<vtkSkybox> skybox;
  skybox->SetProjection(vtkSkybox::Sphere);
  skybox->SetTexture(texture);
  renderer->SkyboxBlurEnabledOn();
  renderer->AddActor(skybox);

  renderer->AddActor(left);
  renderer->AddActor(right);

  vtkNew<vtkRenderWindow> window;
  window->SetSize(400, 200);
  window->AddRenderer(renderer);
  // Framed on the spheres alone: a skybox reports unbounded extents, which would
  // pull the camera away from them.
  double bounds[6];
  left->GetBounds(bounds);
  double rightBounds[6];
  right->GetBounds(rightBounds);
  for (int i = 0; i < 3; ++i)
  {
    bounds[2 * i] = std::min(bounds[2 * i], rightBounds[2 * i]);
    bounds[2 * i + 1] = std::max(bounds[2 * i + 1], rightBounds[2 * i + 1]);
  }
  renderer->ResetCamera(bounds);
  window->Render();

  struct Case
  {
    const char* name;
    vtkActor* expected;
    unsigned int x;
  };
  const Case cases[] = { { "left sphere", left, 120 }, { "right sphere", right, 280 } };

  // Where did anything land at all?
  for (unsigned int x = 20; x < 400; x += 20)
  {
    vtkProp* p = PickAt(renderer, x, 100);
    std::cout << "  probe x=" << x << " -> "
              << (p == left.Get() ? "left" : (p == right.Get() ? "right" : (p ? "other" : "none")))
              << "\n";
  }

  bool ok = true;
  for (const auto& testCase : cases)
  {
    vtkProp* picked = PickAt(renderer, testCase.x, 100);
    if (picked == nullptr)
    {
      std::cerr << "ERROR: nothing was picked at x=" << testCase.x << ", where the "
                << testCase.name << " is drawn\n";
      ok = false;
    }
    else if (picked != testCase.expected)
    {
      std::cerr << "ERROR: the pick at x=" << testCase.x << " named " << picked->GetClassName()
                << " at " << picked << " instead of the " << testCase.name << " at "
                << testCase.expected << "; the prop ids and the selector's prop list disagree\n";
      ok = false;
    }
    else
    {
      std::cout << "pick at x=" << testCase.x << " named the " << testCase.name << "\n";
    }
  }
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
