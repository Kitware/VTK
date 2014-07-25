/*
 * Copyright (C) 2010 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

//BEGIN_INCLUDE(all)
#include <jni.h>
#include <errno.h>

#include "vtkNew.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkConeSource.h"
#include "vtkDebugLeaks.h"
#include "vtkGlyph3D.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"

#include "vtkAndroidRenderWindowInteractor.h"

#include <android/log.h>

#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "NativeVTK", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "NativeVTK", __VA_ARGS__))

extern "C" {
    JNIEXPORT jlong JNICALL Java_com_kitware_JavaVTK_JavaVTKLib_init(JNIEnv * env, jobject obj,  jint width, jint height);
    JNIEXPORT void JNICALL Java_com_kitware_JavaVTK_JavaVTKLib_step(JNIEnv * env, jobject obj, jlong renWinP);
};

JNIEXPORT jlong JNICALL Java_com_kitware_JavaVTK_JavaVTKLib_init(JNIEnv * env, jobject obj,  jint width, jint height)
{
  vtkRenderWindow *renWin = vtkRenderWindow::New();
  renWin->SetWindowInfo("jni"); // tel the system that jni owns the window not us
  vtkNew<vtkRenderer> renderer;
  renWin->AddRenderer(renderer.Get());

  vtkNew<vtkAndroidRenderWindowInteractor> iren;
  //iren->SetAndroidApplication(state);
  iren->SetRenderWindow(renWin);

  vtkNew<vtkSphereSource> sphere;
  sphere->SetThetaResolution(8);
  sphere->SetPhiResolution(8);

  vtkNew<vtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphere->GetOutputPort());
  vtkNew<vtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper.Get());

  vtkNew<vtkConeSource> cone;
  cone->SetResolution(6);

  vtkNew<vtkGlyph3D> glyph;
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSourceConnection(cone->GetOutputPort());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);

  vtkNew<vtkPolyDataMapper> spikeMapper;
  spikeMapper->SetInputConnection(glyph->GetOutputPort());

  vtkNew<vtkActor> spikeActor;
  spikeActor->SetMapper(spikeMapper.Get());

  renderer->AddActor(sphereActor.Get());
  renderer->AddActor(spikeActor.Get());
  renderer->SetBackground(0.4,0.5,0.6);

  renWin->Render();
  return (jlong)renWin;
}

JNIEXPORT void JNICALL Java_com_kitware_JavaVTK_JavaVTKLib_step(JNIEnv * env, jobject obj, jlong renWinP)
{
  vtkRenderWindow *renWin = (vtkRenderWindow *)(renWinP);
  renWin->Render();
}


//END_INCLUDE(all)
