// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkObjectManager.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkSession.h"
#include "vtkStringFormatter.h"

// clang-format off
#include "vtk_nlohmannjson.h"            // for json
#include VTK_NLOHMANN_JSON(json.hpp) // for json
// clang-format on

// Initialize serialization for classes from these modules
#include "vtkFiltersCoreModule.h"      // For vtkFiltersCoreModule
#include "vtkImagingCoreModule.h"      // For vtkImagingCoreModule
#include "vtkInteractionStyleModule.h" // For vtkInteractionStyleModule
#include "vtkRenderingCoreModule.h"    // For vtkRenderingCoreModule
#include "vtkRenderingOpenGL2Module.h" // For vtkRenderingOpenGL2Module
#include "vtkRenderingUIModule.h"      // For vtkRenderingUIModule

#include <cstdlib>
#include <iostream>

struct vtkSessionJsonImpl
{
  nlohmann::json json;
};

vtkSessionJsonImpl* ToJson(const std::string& text)
{
  auto* impl = new vtkSessionJsonImpl;
  if (!text.empty())
  {
    impl->json = nlohmann::json::parse(text);
  }
  return impl;
}

std::string ToString(vtkSessionJson json)
{
  auto* impl = static_cast<vtkSessionJsonImpl*>(json);
  return impl->json.dump();
}

int TestSession(int argc, char* argv[])
{
  vtkSessionDescriptor descriptor;
  descriptor.InteractorManagesTheEventLoop = 1;
  descriptor.ParseJson = +[](const char* text) -> vtkSessionJson { return ToJson(text); };
  descriptor.StringifyJson = +[](vtkSessionJson json) -> char*
  {
    std::string s = ToString(json);
    char* result = new char[s.length() + 1];
    vtk::format_to_n(result, s.length() + 1, "{:s}\0", s.c_str());
    return result;
  };

  auto session = vtkCreateSession(&descriptor);
  if (!session)
  {
    std::cerr << "Failed to create session." << std::endl;
    return EXIT_FAILURE;
  }

  if (vtkSessionInitializeObjectManager(session) == vtkSessionResultFailure)
  {
    std::cerr << "Failed to initialize object manager." << std::endl;
    vtkFreeSession(session);
    return EXIT_FAILURE;
  }

  auto coneSource = vtkSessionCreateObject(session, "vtkRTAnalyticSource");
  auto manager =
    vtkObjectManager::SafeDownCast(reinterpret_cast<vtkObjectBase*>(vtkSessionGetManager(session)));

  auto coneOutput = vtkSessionInvoke(session, coneSource, "GetOutputPort", ToJson("[0]"));

  auto contourFilter = vtkSessionCreateObject(session, "vtkContourFilter");
  std::string contourArgs = "[0," + std::string(descriptor.StringifyJson(coneOutput)) + "]";
  vtkSessionInvoke(session, contourFilter, "SetInputConnection", ToJson(contourArgs));
  vtkSessionInvoke(session, contourFilter, "GenerateValues", ToJson("[15, 0, 255]"));
  auto contourOutput = vtkSessionInvoke(session, contourFilter, "GetOutputPort", ToJson("[0]"));

  auto mapper = vtkSessionCreateObject(session, "vtkPolyDataMapper");
  std::string mapperArgs = "[0," + std::string(descriptor.StringifyJson(contourOutput)) + "]";
  vtkSessionInvoke(session, mapper, "SetInputConnection", ToJson(mapperArgs));
  vtkSessionInvoke(session, mapper, "SetScalarRange", ToJson("[0,255]"));

  auto actor = vtkSessionCreateObject(session, "vtkActor");
  vtkSessionInvoke(
    session, actor, "SetMapper", ToJson("[{\"Id\":" + vtk::to_string(mapper) + "}]"));

  auto renderer = vtkSessionCreateObject(session, "vtkRenderer");
  vtkSessionInvoke(
    session, renderer, "AddActor", ToJson("[{\"Id\":" + vtk::to_string(actor) + "}]"));

  auto renderWindow = vtkSessionCreateObject(session, "vtkRenderWindow");
  vtkSessionInvoke(
    session, renderWindow, "AddRenderer", ToJson("[{\"Id\":" + vtk::to_string(renderer) + "}]"));

  auto interactor = vtkSessionCreateObject(session, "vtkRenderWindowInteractor");
  vtkSessionInvoke(session, interactor, "SetRenderWindow",
    ToJson("[{\"Id\":" + vtk::to_string(renderWindow) + "}]"));

  vtkSessionRender(session, renderWindow);

  if (auto renderWindowObject = vtkRenderWindow::SafeDownCast(manager->GetObjectAtId(renderWindow)))
  {
    int retVal = vtkRegressionTestImage(renderWindowObject);
    if (retVal == vtkRegressionTester::DO_INTERACTOR)
    {
      vtkSessionStartEventLoop(session, renderWindow);
    }
    vtkFreeSession(session);
    return retVal == vtkRegressionTester::FAILED ? EXIT_FAILURE : EXIT_SUCCESS;
  }
  else
  {
    std::cerr << "Failed to get render window." << std::endl;
    vtkFreeSession(session);
    return EXIT_FAILURE;
  }
}
