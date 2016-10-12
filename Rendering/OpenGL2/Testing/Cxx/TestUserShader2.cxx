/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkActor.h"
#include "vtkCamera.h"
#include "vtkCommand.h"
#include "vtkMath.h"
#include "vtkNew.h"
#include "vtkOpenGLPolyDataMapper.h"
#include "vtkPLYReader.h"
#include "vtkPolyDataNormals.h"
#include "vtkProperty.h"
#include "vtkRegressionTestImage.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkRenderer.h"
#include "vtkShaderProgram.h"
#include "vtkTestUtilities.h"
#include "vtkTimerLog.h"


#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

// -----------------------------------------------------------------------
// Update a uniform in the shader for each render. We do this with a
// callback for the UpdateShaderEvent
class vtkShaderCallback : public vtkCommand
{
public:
  static vtkShaderCallback *New()
    { return new vtkShaderCallback; }
  vtkRenderer *Renderer;
  void Execute(vtkObject *, unsigned long, void*cbo) VTK_OVERRIDE
  {
    vtkOpenGLHelper *cellBO = reinterpret_cast<vtkOpenGLHelper*>(cbo);

    float diffuseColor[3];

#if 0  // trippy mode
    float inputHSV[3];
    double theTime = vtkTimerLog::GetUniversalTime();
    double twopi = 2.0*3.1415926;

    inputHSV[0] = sin(twopi*fmod(theTime,3.0)/3.0)/4.0 + 0.25;
    inputHSV[1] = sin(twopi*fmod(theTime,4.0)/4.0)/2.0 + 0.5;
    inputHSV[2] = 0.7*(sin(twopi*fmod(theTime,19.0)/19.0)/2.0 + 0.5);
    vtkMath::HSVToRGB(inputHSV,diffuseColor);
    cellBO->Program->SetUniform3f("diffuseColorUniform", diffuseColor);

    if (this->Renderer)
    {
      inputHSV[0] = sin(twopi*fmod(theTime,5.0)/5.0)/4.0 + 0.75;
      inputHSV[1] = sin(twopi*fmod(theTime,7.0)/7.0)/2.0 + 0.5;
      inputHSV[2] = 0.5*(sin(twopi*fmod(theTime,17.0)/17.0)/2.0 + 0.5);
      vtkMath::HSVToRGB(inputHSV,diffuseColor);
      this->Renderer->SetBackground(diffuseColor[0], diffuseColor[1], diffuseColor[2]);

      inputHSV[0] = sin(twopi*fmod(theTime,11.0)/11.0)/2.0+0.5;
      inputHSV[1] = sin(twopi*fmod(theTime,13.0)/13.0)/2.0 + 0.5;
      inputHSV[2] = 0.5*(sin(twopi*fmod(theTime,17.0)/17.0)/2.0 + 0.5);
      vtkMath::HSVToRGB(inputHSV,diffuseColor);
      this->Renderer->SetBackground2(diffuseColor[0], diffuseColor[1], diffuseColor[2]);
    }
#else
    diffuseColor[0] = 0.4;
    diffuseColor[1] = 0.7;
    diffuseColor[2] = 0.6;
    cellBO->Program->SetUniform3f("diffuseColorUniform", diffuseColor);
#endif
  }

  vtkShaderCallback() { this->Renderer = 0; }
};

//----------------------------------------------------------------------------
int TestUserShader2(int argc, char *argv[])
{
  vtkNew<vtkActor> actor;
  vtkNew<vtkRenderer> renderer;
  vtkNew<vtkOpenGLPolyDataMapper> mapper;
  renderer->SetBackground(0.0, 0.0, 0.0);
  vtkNew<vtkRenderWindow> renderWindow;
  renderWindow->SetSize(400, 400);
  renderWindow->AddRenderer(renderer.Get());
  renderer->AddActor(actor.Get());
  renderer->GradientBackgroundOn();
  vtkNew<vtkRenderWindowInteractor>  iren;
  iren->SetRenderWindow(renderWindow.Get());

  const char* fileName = vtkTestUtilities::ExpandDataFileName(argc, argv,
                                                               "Data/dragon.ply");
  vtkNew<vtkPLYReader> reader;
  reader->SetFileName(fileName);
  reader->Update();

  vtkNew<vtkPolyDataNormals> norms;
  norms->SetInputConnection(reader->GetOutputPort());
  norms->Update();

  mapper->SetInputConnection(norms->GetOutputPort());
  actor->SetMapper(mapper.Get());
  actor->GetProperty()->SetAmbientColor(0.2, 0.2, 1.0);
  actor->GetProperty()->SetDiffuseColor(1.0, 0.65, 0.7);
  actor->GetProperty()->SetSpecularColor(1.0, 1.0, 1.0);
  actor->GetProperty()->SetSpecular(0.5);
  actor->GetProperty()->SetDiffuse(0.7);
  actor->GetProperty()->SetAmbient(0.5);
  actor->GetProperty()->SetSpecularPower(20.0);
  actor->GetProperty()->SetOpacity(1.0);

  // Use our own hardcoded shader code. Generally this is a bad idea in a
  // general purpose program as there are so many things VTK supports that
  // hardcoded shaders will not handle depth peeling, picking, etc, but if you
  // know what your data will be like it can be very useful. The mapper will set
  // a bunch of uniforms regardless of if you are using them. But feel free to
  // use them :-)
  mapper->SetVertexShaderCode(
    "//VTK::System::Dec\n"  // always start with this line
    "attribute vec4 vertexMC;\n"
    // use the default normal decl as the mapper
    // will then provide the normalMatrix uniform
    // which we use later on
    "//VTK::Normal::Dec\n"
    "uniform mat4 MCDCMatrix;\n"
    "void main () {\n"
    "  normalVCVSOutput = normalMatrix * normalMC;\n"
    // do something weird with the vertex positions
    // this will mess up your head if you keep
    // rotating and looking at it, very trippy
    "  vec4 tmpPos = MCDCMatrix * vertexMC;\n"
    "  gl_Position = tmpPos*vec4(0.2+0.8*abs(tmpPos.x),0.2+0.8*abs(tmpPos.y),1.0,1.0);\n"
    "}\n"
    );
  mapper->SetFragmentShaderCode(
    "//VTK::System::Dec\n"  // always start with this line
    "//VTK::Output::Dec\n"  // always have this line in your FS
    "varying vec3 normalVCVSOutput;\n"
    "uniform vec3 diffuseColorUniform;\n"
    "void main () {\n"
    "  float df = max(0.0, normalVCVSOutput.z);\n"
    "  float sf = pow(df, 20.0);\n"
    "  vec3 diffuse = df * diffuseColorUniform;\n"
    "  vec3 specular = sf * vec3(0.4,0.4,0.4);\n"
    "  gl_FragData[0] = vec4(0.3*abs(normalVCVSOutput) + 0.7*diffuse + specular, 1.0);\n"
    "}\n"
    );

  // Setup a callback to change some uniforms
  VTK_CREATE(vtkShaderCallback, myCallback);
  myCallback->Renderer = renderer.Get();
  mapper->AddObserver(vtkCommand::UpdateShaderEvent,myCallback);

  renderWindow->Render();
  renderer->GetActiveCamera()->SetPosition(-0.2,0.4,1);
  renderer->GetActiveCamera()->SetFocalPoint(0,0,0);
  renderer->GetActiveCamera()->SetViewUp(0,1,0);
  renderer->ResetCamera();
  renderer->GetActiveCamera()->Zoom(2.0);
  renderWindow->Render();

  int retVal = vtkRegressionTestImage( renderWindow.Get() );
  if ( retVal == vtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return EXIT_SUCCESS;
}
