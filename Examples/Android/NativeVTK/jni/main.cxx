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

#include <jni.h>
#include <errno.h>

#include "vtkNew.h"

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkConeSource.h"
#include "vtkDebugLeaks.h"
#include "vtkGlyph3D.h"
#include "vtkImageExtractComponents.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPNGWriter.h"
#include "vtkRenderWindow.h"
#include "vtkRenderer.h"
#include "vtkSphereSource.h"
#include "vtkTesting.h"
#include "vtkTextActor.h"
#include "vtkTextProperty.h"
#include "vtkWindowToImageFilter.h"

#include "vtkAndroidRenderWindowInteractor.h"

#include <android/log.h>
#include <android_native_app_glue.h>
#include <sys/stat.h>


#define LOGI(...) ((void)__android_log_print(ANDROID_LOG_INFO, "NativeVTK", __VA_ARGS__))
#define LOGW(...) ((void)__android_log_print(ANDROID_LOG_WARN, "NativeVTK", __VA_ARGS__))

/**
 * This is the main entry point of a native application that is using
 * android_native_app_glue.  It runs in its own thread, with its own
 * event loop for receiving input events and doing other things.
 */
void android_main(struct android_app* state)
{
  // Make sure glue isn't stripped.
  app_dummy();

  vtkNew<vtkRenderWindow> renWin;
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkAndroidRenderWindowInteractor> iren;

  // this line is key, it provides the android
  // state to VTK
  iren->SetAndroidApplication(state);

  renWin->AddRenderer(renderer.Get());
  iren->SetRenderWindow(renWin.Get());

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

  vtkNew<vtkTextActor> ta;
  ta->SetInput("Droids Rock");
  ta->GetTextProperty()->SetColor( 0.5, 1.0, 0.0 );
  ta->SetDisplayPosition(50,50);
  ta->GetTextProperty()->SetFontSize(32);
  renderer->AddActor(ta.Get());

  iren->Initialize();
  renWin->Render();
  renWin->Render();
  renWin->Render();
  renWin->Render();
  renWin->Render();


  /**************************************************
  *  THIS BLOCK IS JUST FOR VTK's REGRESSIONS TESTING
  *  AND IS NOT NEEDED IN GENERAL
  **************************************************/
  {
  JNIEnv *env;
  state->activity->vm->AttachCurrentThread(&env, 0);

  jobject me = state->activity->clazz;

  jclass acl = env->GetObjectClass(me); //class pointer of NativeActivity
  jmethodID giid = env->GetMethodID(acl, "getIntent", "()Landroid/content/Intent;");
  jobject intent = env->CallObjectMethod(me, giid); //Got our intent

  jclass icl = env->GetObjectClass(intent); //class pointer of Intent
  jmethodID gseid = env->GetMethodID(icl, "getStringExtra", "(Ljava/lang/String;)Ljava/lang/String;");

  jstring jsParam1 = (jstring)env->CallObjectMethod(intent, gseid, env->NewStringUTF("VTKTesting"));
  int testing = 0;
  if (jsParam1)
  {
    const char *Param1 = env->GetStringUTFChars(jsParam1, 0);
    testing = !strcmp(Param1,"Testing");
    //When done with it, or when you've made a copy
    env->ReleaseStringUTFChars(jsParam1, Param1);
  }
  state->activity->vm->DetachCurrentThread();

  if (testing)
  {
    ANativeActivity* nativeActivity = state->activity;
    const char* internalPath = nativeActivity->externalDataPath;
    std::string dataPath(internalPath);

    // sometimes if this is the first time we run the app
    // then we need to create the internal storage "files" directory
    struct stat sb;
    int32_t res = stat(dataPath.c_str(), &sb);
    if (0 == res && sb.st_mode & S_IFDIR)
    {
      LOGW("'files/' dir already in app's internal data storage.");
    }
    else if (ENOENT == errno)
    {
      res = mkdir(dataPath.c_str(), 0774);
    }

    // internalDataPath points directly to the files/ directory
    std::string outputFile = dataPath + "/NativeVTKResult.png";
    LOGW(outputFile.c_str());
    vtkNew<vtkWindowToImageFilter> rtW2if;
    rtW2if->SetInput(renWin.Get());
    rtW2if->ReadFrontBufferOff();
    rtW2if->SetInputBufferTypeToRGBA();

    vtkNew<vtkImageExtractComponents> iec;
    iec->SetInputConnection(rtW2if->GetOutputPort());
    iec->SetComponents(0,1,2);

    vtkNew<vtkPNGWriter> rtPngw;
    rtPngw->SetFileName(outputFile.c_str());
    rtPngw->SetInputConnection(iec->GetOutputPort());
    rtPngw->Write();

    //vtkNew<vtkImageDifference> rtId;

    vtkNew<vtkTesting> tst;
    std::string outputText = dataPath + "/NativeVTKResult.txt";
    std::ofstream ofs;
    ofs.open(outputText.c_str(),std::ofstream::out);
    std::string validFile = dataPath + "/NativeVTKValid.png";
    tst->AddArgument("-V");
    tst->AddArgument(validFile.c_str());
    int result = tst->RegressionTest(outputFile.c_str(),10.0,ofs);
    ofs.close();


    ANativeActivity_finish(state->activity);
  }
  }
  /*********************************************
  *  END OF THE REGRESSION TESTING BLOCK
  *********************************************/

  iren->Start();
}
