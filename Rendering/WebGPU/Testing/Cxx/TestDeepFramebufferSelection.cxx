// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor.h"
#include "vtkDataObject.h"
#include "vtkHardwareSelector.h"
#include "vtkIdTypeArray.h"
#include "vtkInformation.h"
#include "vtkLogger.h"
#include "vtkNew.h"
#include "vtkPolyDataMapper.h"
#include "vtkProp.h"
#include "vtkProperty.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkSmartPointer.h"
#include "vtkSphereSource.h"
#include "vtkWebGPURenderer.h"

#include <cstdlib>
#include <map>
#include <string>

namespace
{
vtkSmartPointer<vtkActor> MakeSphere(double z, double opacity, const double rgb[3])
{
  vtkNew<vtkSphereSource> source;
  source->SetCenter(0.0, 0.0, z);
  source->SetRadius(1.0);
  source->SetThetaResolution(16);
  source->SetPhiResolution(16);

  vtkNew<vtkPolyDataMapper> mapper;
  mapper->SetInputConnection(source->GetOutputPort());

  vtkSmartPointer<vtkActor> actor = vtkSmartPointer<vtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetColor(rgb[0], rgb[1], rgb[2]);
  actor->GetProperty()->SetOpacity(opacity);
  return actor;
}

std::map<std::string, vtkIdType> Select(
  bool deepFramebuffer, int association, bool interactive = false)
{
  const double red[3] = { 1.0, 0.0, 0.0 };
  const double green[3] = { 0.0, 1.0, 0.0 };
  const double blue[3] = { 0.0, 0.0, 1.0 };

  vtkSmartPointer<vtkActor> opaqueFar = MakeSphere(-2.0, 1.0, blue);
  vtkSmartPointer<vtkActor> translucentMid = MakeSphere(0.0, 0.4, green);
  vtkSmartPointer<vtkActor> translucentNear = MakeSphere(2.0, 0.4, red);

  vtkNew<vtkRenderer> renderer;
  auto* wgpuRenderer = vtkWebGPURenderer::SafeDownCast(renderer);
  if (wgpuRenderer == nullptr)
  {
    vtkLog(ERROR, "renderer is not a vtkWebGPURenderer");
    return {};
  }
  wgpuRenderer->SetUseDeepFramebufferTransparency(deepFramebuffer);

  renderer->AddActor(opaqueFar);
  renderer->AddActor(translucentMid);
  renderer->AddActor(translucentNear);
  renderer->SetBackground(0.1, 0.1, 0.1);

  vtkNew<vtkRenderWindow> window;
  window->SetSize(300, 300);
  window->AddRenderer(renderer);
  renderer->ResetCamera();
  window->Render();
  if (interactive)
  {
    vtkNew<vtkRenderWindowInteractor> iren;
    iren->SetRenderWindow(window);
    iren->Initialize();
    iren->Start();
  }

  std::map<vtkProp*, std::string> names = { { opaqueFar, "far" }, { translucentMid, "mid" },
    { translucentNear, "near" } };

  vtkNew<vtkHardwareSelector> selector;
  selector->SetRenderer(renderer);
  selector->SetFieldAssociation(association);
  selector->SetArea(145, 145, 155, 155);

  std::map<std::string, vtkIdType> picked;
  vtkSelection* selection = selector->Select();
  if (selection == nullptr)
  {
    vtkLog(ERROR, "the selector returned no selection");
    return picked;
  }
  for (unsigned int node = 0; node < selection->GetNumberOfNodes(); ++node)
  {
    auto* selectionNode = selection->GetNode(node);
    auto* prop =
      vtkProp::SafeDownCast(selectionNode->GetProperties()->Get(vtkSelectionNode::PROP()));
    auto* ids = vtkArrayDownCast<vtkIdTypeArray>(selectionNode->GetSelectionList());
    const auto name = names.count(prop) ? names[prop] : std::string("unknown");
    picked[name] += ids ? ids->GetNumberOfTuples() : 0;
  }
  selection->Delete();
  return picked;
}

std::string Describe(const std::map<std::string, vtkIdType>& picked)
{
  if (picked.empty())
  {
    return "nothing";
  }
  std::string text;
  for (const auto& entry : picked)
  {
    text += entry.first + "=" + std::to_string(entry.second) + " ";
  }
  return text;
}

bool Compare(const char* what, int association)
{
  const auto plain = Select(false, association);
  const auto deep = Select(true, association);
  if (plain.empty())
  {
    vtkLog(ERROR,
      "" << what
         << ": the ordinary translucent path selected nothing, so there is nothing to compare "
            "against");
    return false;
  }
  if (plain != deep)
  {
    vtkLog(ERROR,
      "" << what << ": the deep framebuffer changed what was selected. Without it: "
         << Describe(plain) << "- with it: " << Describe(deep));
    return false;
  }
  vtkLog(INFO, "" << what << ": " << Describe(plain) << "with and without the deep framebuffer");
  return true;
}
}

int TestDeepFramebufferSelection(int argc, char* argv[])
{
  bool interactive = false;
  for (int i = 0; i < argc; ++i)
  {
    if (std::string(argv[i]) == "-I")
    {
      interactive = true;
      break;
    }
  }

  if (interactive)
  {
    Select(true, vtkDataObject::FIELD_ASSOCIATION_CELLS, true);
    return EXIT_SUCCESS;
  }

  bool ok = true;
  ok &= Compare("cell selection", vtkDataObject::FIELD_ASSOCIATION_CELLS);
  ok &= Compare("point selection", vtkDataObject::FIELD_ASSOCIATION_POINTS);
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
