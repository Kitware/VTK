/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestContext.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkRenderWindowInteractor.h"
#include "vtkSmartPointer.h"
#include "vtkObjectFactory.h"
#include "vtkContext2D.h"
#include "vtkContextItem.h"
#include "vtkContextView.h"
#include "vtkContextScene.h"
#include "vtkPen.h"
#include "vtkBrush.h"
#include "vtkOpenGLContextDevice2D.h"
#include "vtkPoints2D.h"

#include "vtkShaderProgram2.h"
#include "vtkShader2.h"
#include "vtkShader2Collection.h"
#include "vtkUniformVariables.h"
#include "vtkOpenGLRenderWindow.h"

#include "vtkRegressionTestImage.h"

const char* SimpleVertexShader =
"void main(void)\n"
"{\n"
"  gl_FrontColor = gl_Color;\n"
"  gl_Position = gl_Position = ftransform();\n"
"}\n";

const char* SimpleFragmentShader =
"void main()\n"
"{\n"
"  vec2 location = gl_PointCoord - vec2(0.5, 0.5);\n"
"  float length = dot(location, location);\n"
"  if (length < 0.20)\n"
"    gl_FragColor = gl_Color;\n"
"  else\n"
"    gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
"}\n";

const char* SimpleFragmentShader2 =
"void main()\n"
"{\n"
"  vec2 location = gl_PointCoord - vec2(0.5, 0.5);\n"
"  float length = dot(location, location);\n"
"  if(length > 0.25)\n"
"    discard;\n"
"  if (length < 0.20)\n"
"    gl_FragColor = gl_Color;\n"
"  else\n"
"    gl_FragColor = vec4(0.0, 0.0, 0.0, 1.0);\n"
"}\n";

#define VTK_CREATE(type, name) \
  vtkSmartPointer<type> name = vtkSmartPointer<type>::New()

//----------------------------------------------------------------------------
class GLSLTestItem : public vtkContextItem
{
public:
  GLSLTestItem() : program(0), program2(0)
  {
  }

  static GLSLTestItem *New();
  vtkTypeMacro(GLSLTestItem, vtkContextItem);
  // Paint event for the chart, called whenever the chart needs to be drawn
  virtual bool Paint(vtkContext2D *painter);

  void BuildShader(vtkOpenGLRenderWindow*);

  vtkShaderProgram2 *program, *program2;
};

//----------------------------------------------------------------------------
int TestGLSL( int argc, char * argv [] )
{
  // Set up a 2D context view, context test object and add it to the scene
  VTK_CREATE(vtkContextView, view);
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(200, 200);
  VTK_CREATE(GLSLTestItem, test);
  view->GetScene()->AddItem(test);

  // Check if GLSL is supported
  if (!vtkShaderProgram2::IsSupported(dynamic_cast<vtkOpenGLRenderWindow*>(
                                      view->GetRenderWindow())))
    {
      cout << "GLSL not supported." << endl;
      return 1;
    }

  view->GetRenderWindow()->SetMultiSamples(0);

  int retVal = vtkRegressionTestImage(view->GetRenderWindow());
  if(retVal == vtkRegressionTester::DO_INTERACTOR)
    {
    view->GetInteractor()->Initialize();
    view->GetInteractor()->Start();
    }

  return !retVal;
}

// Make our new derived class to draw a diagram
vtkStandardNewMacro(GLSLTestItem);
// This function aims to test the primitives provided by the 2D API.
bool GLSLTestItem::Paint(vtkContext2D *painter)
{
  // Build and link our shader if necessary
  vtkOpenGLContextDevice2D* device =
    vtkOpenGLContextDevice2D::SafeDownCast(painter->GetDevice());
  if (device)
    {
    this->BuildShader(device->GetRenderWindow());
    }
  else
    {
    return false;
    }

  // Draw points without our shader code
  for (int i = 0; i < 8; ++i)
    {
    float pos[] = { 50, i*25+5 };
    painter->GetPen()->SetColor(255,
                                static_cast<unsigned char>(float(i)*35.0),
                                0);
    painter->GetPen()->SetWidth(i*5+1);
    painter->DrawPointSprites(0, pos, 1);
    }


  // Draw the points using the first shader program
  this->program->Use();
  for (int i = 0; i < 8; ++i)
    {
    float pos[] = { 100, i*25+5 };
    painter->GetPen()->SetColor(255,
                                0,
                                static_cast<unsigned char>(float(i)*35.0));
    painter->GetPen()->SetWidth(i*5+1);
    painter->DrawPointSprites(0, pos, 1);
    }
  this->program->Restore();

  // Draw the points using the second shader program
  this->program2->Use();
  for (int i = 0; i < 8; ++i)
    {
    float pos[] = { 150, i*25+5 };
    painter->GetPen()->SetColor(static_cast<unsigned char>(float(i)*35.0),
                                255,
                                0);
    painter->GetPen()->SetWidth(i*5+1);
    painter->DrawPointSprites(0, pos, 1);
    }
  this->program2->Restore();

  return true;
}

void GLSLTestItem::BuildShader(vtkOpenGLRenderWindow* glContext)
{
  if (this->program)
    {
    return;
    }
  this->program = vtkShaderProgram2::New();
  this->program->SetContext(glContext);
  this->program2 = vtkShaderProgram2::New();
  this->program2->SetContext(glContext);

  // The vertext shader
  vtkShader2 *shader = vtkShader2::New();
  shader->SetType(VTK_SHADER_TYPE_VERTEX);
  shader->SetSourceCode(SimpleVertexShader);
  shader->SetContext(this->program->GetContext());
  this->program->GetShaders()->AddItem(shader);
  this->program2->GetShaders()->AddItem(shader);
  shader->Delete();

  // The fragment shader
  shader = vtkShader2::New();
  shader->SetType(VTK_SHADER_TYPE_FRAGMENT);
  shader->SetSourceCode(SimpleFragmentShader);
  shader->SetContext(this->program->GetContext());
  this->program->GetShaders()->AddItem(shader);
  shader->Delete();

  // The 2nd fragment shader
  shader = vtkShader2::New();
  shader->SetType(VTK_SHADER_TYPE_FRAGMENT);
  shader->SetSourceCode(SimpleFragmentShader2);
  shader->SetContext(this->program->GetContext());
  this->program2->GetShaders()->AddItem(shader);
  shader->Delete();

  // Build the shader programs
  this->program->Build();
  if(this->program->GetLastBuildStatus() != VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
    {
    vtkErrorMacro("Couldn't build the shader program. It could be an error in a shader, or a driver bug.");
    return;
    }

  this->program2->Build();
  if(this->program2->GetLastBuildStatus() != VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
    {
    vtkErrorMacro("Couldn't build the shader program. It could be an error in a shader, or a driver bug.");
    return;
    }
}
