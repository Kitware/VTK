/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLGL2PSExporter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLGL2PSExporter.h"

#include "vtkObjectFactory.h"
#include "vtkOpenGLGL2PSHelper.h"
#include "vtkRenderWindow.h"

#include "vtk_gl2ps.h"

#include <cstdio>
#include <sstream>
#include <string>

vtkStandardNewMacro(vtkOpenGLGL2PSExporter)

//------------------------------------------------------------------------------
void vtkOpenGLGL2PSExporter::PrintSelf(std::ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
vtkOpenGLGL2PSExporter::vtkOpenGLGL2PSExporter()
{
}

//------------------------------------------------------------------------------
vtkOpenGLGL2PSExporter::~vtkOpenGLGL2PSExporter()
{
}

//------------------------------------------------------------------------------
void vtkOpenGLGL2PSExporter::WriteData()
{
  // Open file:
  if (this->FilePrefix == NULL)
    {
    vtkErrorMacro(<< "Please specify a file prefix to use");
    return;
    }

  std::ostringstream fname;
  fname << this->FilePrefix << "." << this->GetFileExtension();
  if (this->Compress)
    {
    fname << ".gz";
    }
  FILE *file = fopen(fname.str().c_str(), "wb");
  if (!file)
    {
    vtkErrorMacro("Unable to open file: " << fname.str());
    return;
    }

  // Setup information that GL2PS will need to export the scene:
  std::string title = (this->Title && this->Title[0]) ? this->Title
                                                      : "VTK GL2PS Export";
  GLint options = static_cast<GLint>(this->GetGL2PSOptions());
  GLint sort = static_cast<GLint>(this->GetGL2PSSort());
  GLint format = static_cast<GLint>(this->GetGL2PSFormat());
  int *winsize = this->RenderWindow->GetSize();
  GLint viewport[4] = {0, 0, static_cast<GLint>(winsize[0]),
                       static_cast<GLint>(winsize[1])};

  // Setup helper class:
  vtkNew<vtkOpenGLGL2PSHelper> helper;
  helper->SetInstance(helper.GetPointer());
  helper->SetCapturing(true);
  helper->SetTextAsPath(this->TextAsPath);
  helper->SetRenderWindow(this->RenderWindow);

  // Export file. No worries about buffersize, since we're manually adding
  // geometry through vtkOpenGLGL2PSHelper::ProcessTransformFeedback.
  GLint err = gl2psBeginPage(title.c_str(), "VTK", viewport, format, sort,
                             options, GL_RGBA, 0, NULL, 0, 0, 0, 0, file,
                             fname.str().c_str());
  if (err != GL2PS_SUCCESS)
    {
    vtkErrorMacro("Error calling gl2psBeginPage. Error code: " << err);
    helper->SetCapturing(false);
    helper->SetInstance(NULL);
    fclose(file);
    return;
    }

  // Render the scene:
  this->RenderWindow->Render();

  err = gl2psEndPage();
  fclose(file);

  // Cleanup helper.
  helper->SetCapturing(false);
  helper->SetInstance(NULL);

  if (err != GL2PS_SUCCESS)
    {
    vtkErrorMacro("Error calling gl2psEndPage. Error code: " << err);
    return;
    }
}
