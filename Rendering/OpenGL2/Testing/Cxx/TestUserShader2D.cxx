// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

#include "vtkActor2D.h"
#include "vtkCallbackCommand.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkNew.h"
#include "vtkOpenGLPolyDataMapper2D.h"
#include "vtkPlaneSource.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkShaderProperty.h"
#include "vtkUniforms.h"

namespace
{
struct TestUserShader2D_TimerData
{
  vtkShaderProperty* shaderProperty;
  float time;
  int id;
};

void TestUserShader2D_OnTimerCallback(
  vtkObject* obj, unsigned long, void* clientData, void* callData)
{
  auto* iren = vtkRenderWindowInteractor::SafeDownCast(obj);
  const int firedTimerId = *reinterpret_cast<int*>(callData);
  auto* timerData = static_cast<TestUserShader2D_TimerData*>(clientData);
  if (timerData == nullptr)
  {
    return;
  }
  if (timerData->id != firedTimerId)
  {
    return;
  }
  timerData->time += iren->GetTimerDuration(firedTimerId);
  timerData->shaderProperty->GetFragmentCustomUniforms()->SetUniformf("time", timerData->time);
  iren->GetRenderWindow()->Render();
}
}

//------------------------------------------------------------------------------
int TestUserShader2D(int argc, char* argv[])
{
  vtkNew<vtkActor2D> actor;
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkOpenGLPolyDataMapper2D> mapper;
  renderer->SetBackground(0.0, 0.0, 0.0);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(400, 400);
  renderWindow->AddRenderer(renderer);
  renderer->AddActor2D(actor);
  vtkNew<vtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);

  vtkNew<vtkPlaneSource> plane;
  mapper->SetInputConnection(plane->GetOutputPort());
  actor->SetMapper(mapper);

  vtkNew<vtkCoordinate> pCoord;
  pCoord->SetCoordinateSystemToWorld();

  vtkNew<vtkCoordinate> coord;
  coord->SetCoordinateSystemToNormalizedViewport();
  coord->SetReferenceCoordinate(pCoord);
  mapper->SetTransformCoordinate(coord);

  // render white noise (like a TV that displays noise signal)
  vtkShaderProperty* sp = actor->GetShaderProperty();
  sp->AddFragmentShaderReplacement(
    "//VTK::CustomUniforms::Dec", // replace the custom uniforms block
    true,                         // before the standard replacements
    R"(
uniform float time;
float generateRandom (vec2 st) { return fract(sin(dot(st.xy, vec2(12.9898,78.233) + sin(time))) * 43758.5453123); }
)",
    false // only do it once
  );
  sp->AddFragmentShaderReplacement("//VTK::Color::Impl", // replace the color block
    true,                                                // before the standard replacements
    R"(
float noise = mix(-1.0f, 1.0f, generateRandom(tcoordVCVSOutput));
gl_FragData[0] = vec4(noise, noise, noise, 1.0);
)",                                                      // but we add this
    false                                                // only do it once
  );

  // Test enumerating shader replacements
  int nbReplacements = sp->GetNumberOfShaderReplacements();
  if (nbReplacements != 2)
  {
    return EXIT_FAILURE;
  }
  if (sp->GetNthShaderReplacementTypeAsString(0) != std::string("Fragment") ||
    sp->GetNthShaderReplacementTypeAsString(1) != std::string("Fragment"))
  {
    return EXIT_FAILURE;
  }

  TestUserShader2D_TimerData timerData;
  timerData.shaderProperty = sp;
  timerData.time = 0;
  timerData.id = iren->CreateRepeatingTimer(10);

  vtkNew<vtkCallbackCommand> timerCmd;
  timerCmd->SetClientData(&timerData);
  timerCmd->SetCallback(::TestUserShader2D_OnTimerCallback);
  iren->AddObserver(vtkCommand::TimerEvent, timerCmd);

  renderWindow->Render();

  int retVal = vtkRegressionTestImage(renderWindow);
  if (retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
