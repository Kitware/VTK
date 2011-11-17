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
#include "vtkOpenGL2ContextDevice2D.h"
#include "vtkPoints2D.h"

#include "vtkShaderProgram2.h"
#include "vtkShader2.h"
#include "vtkShader2Collection.h"
#include "vtkUniformVariables.h"
#include "vtkOpenGLRenderWindow.h"
#include "vtkNew.h"

#include "vtkRegressionTestImage.h"

const char* SimpleVertexShader =
"#version 120\n"
"void main(void)\n"
"{\n"
"  gl_FrontColor = gl_Color;\n"
"  gl_Position = ftransform();\n"
"}\n";

const char* SimpleFragmentShader =
"#version 120\n"
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
"#version 120\n"
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

//----------------------------------------------------------------------------
class GLSLTestItem : public vtkContextItem
{
public:
  static GLSLTestItem *New();
  vtkTypeMacro(GLSLTestItem, vtkContextItem);
  // Paint event for the test.
  virtual bool Paint(vtkContext2D *painter);
  // Required for the shader programs - ensure they release their resources.
  virtual void ReleaseGraphicsResources();

  void BuildShader(vtkOpenGLRenderWindow*);

  vtkSmartPointer<vtkShaderProgram2> program, program2;
  bool IsCompiled;
};

//----------------------------------------------------------------------------
int TestGLSL( int, char * [] )
{
  // Set up a 2D context view, context test object and add it to the scene
  vtkNew<vtkContextView> view;
  view->GetRenderer()->SetBackground(1.0, 1.0, 1.0);
  view->GetRenderWindow()->SetSize(200, 200);
  vtkNew<GLSLTestItem> test;
  view->GetScene()->AddItem(test.GetPointer());

  // Ensure that there is a valid OpenGL context - Mac inconsistent behavior.
  view->GetRenderWindow()->SetMultiSamples(0);
  // Need to attempt at least one render, to see if the GLSL can compile.
  view->Render();

  if (test->IsCompiled)
    {
    view->GetInteractor()->Initialize();
    view->GetInteractor()->Start();
    }
  else
    {
    cout << "GLSL 1.20 required, shader failed to compile." << endl;
    }
  return EXIT_SUCCESS;
}

// Make our new derived class to draw a diagram
vtkStandardNewMacro(GLSLTestItem);
// This function aims to test the primitives provided by the 2D API.
bool GLSLTestItem::Paint(vtkContext2D *painter)
{
  // Build and link our shader if necessary
  vtkOpenGL2ContextDevice2D *device =
    vtkOpenGL2ContextDevice2D::SafeDownCast(painter->GetDevice());
  if (device)
    {
    this->BuildShader(device->GetRenderWindow());
    if (!this->IsCompiled)
      {
      return false;
      }
    }
  else
    {
    this->IsCompiled = false;
    return false;
    }

  // Draw points without our shader code
  for (int i = 0; i < 8; ++i)
    {
    float pos[] = { 50.0f, static_cast<float>(i)*25.0f+5.0f };
    painter->GetPen()->SetColor(255,
                                static_cast<unsigned char>(float(i)*35.0),
                                0);
    painter->GetPen()->SetWidth(static_cast<float>(i)*5.0f+1.0f);
    painter->DrawPointSprites(0, pos, 1);
    }


  // Draw the points using the first shader program
  this->program->Use();
  for (int i = 0; i < 8; ++i)
    {
    float pos[] = { 100.0f, static_cast<float>(i)*25.0f+5.0f };
    painter->GetPen()->SetColor(255,
                                0,
                                static_cast<unsigned char>(float(i)*35.0));
    painter->GetPen()->SetWidth(static_cast<float>(i)*5.0f+1.0f);
    painter->DrawPointSprites(0, pos, 1);
    }
  this->program->Restore();

  // Draw the points using the second shader program
  this->program2->Use();
  for (int i = 0; i < 8; ++i)
    {
    float pos[] = { 150.0f, static_cast<float>(i)*25.0f+5.0f };
    painter->GetPen()->SetColor(static_cast<unsigned char>(float(i)*35.0),
                                255,
                                0);
    painter->GetPen()->SetWidth(static_cast<float>(i)*5.0f+1.0f);
    painter->DrawPointSprites(0, pos, 1);
    }
  this->program2->Restore();

  return true;
}

void GLSLTestItem::ReleaseGraphicsResources()
{
  if (this->program)
    {
    this->program->ReleaseGraphicsResources();
    }
  if (this->program2)
    {
    this->program2->ReleaseGraphicsResources();
    }
}

void GLSLTestItem::BuildShader(vtkOpenGLRenderWindow* glContext)
{
  if (this->program)
    {
    return;
    }

  // Check if GLSL is supported
  if (!vtkShaderProgram2::IsSupported(glContext))
    {
    vtkErrorMacro("GLSL is not supported on this system.");
    this->IsCompiled = false;
    return;
    }

  this->program = vtkSmartPointer<vtkShaderProgram2>::New();
  this->program->SetContext(glContext);
  this->program2 = vtkSmartPointer<vtkShaderProgram2>::New();
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
    this->IsCompiled = false;
    return;
    }

  this->program2->Build();
  if(this->program2->GetLastBuildStatus() != VTK_SHADER_PROGRAM2_LINK_SUCCEEDED)
    {
    vtkErrorMacro("Couldn't build the shader program. It could be an error in a shader, or a driver bug.");
    this->IsCompiled = false;
    return;
    }
  this->IsCompiled = true;
}
