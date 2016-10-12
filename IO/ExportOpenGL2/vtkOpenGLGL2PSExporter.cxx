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

#include "vtkImageData.h"
#include "vtkImageShiftScale.h"
#include "vtkNew.h"
#include "vtkObjectFactory.h"
#include "vtkOpenGLGL2PSHelper.h"
#include "vtkRenderWindow.h"
#include "vtkWindowToImageFilter.h"

#include "vtk_gl2ps.h"

#include <algorithm>
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
  vtkNew<vtkOpenGLGL2PSHelper> gl2ps;
  gl2ps->SetInstance(gl2ps.GetPointer());
  gl2ps->SetTextAsPath(this->TextAsPath);
  gl2ps->SetRenderWindow(this->RenderWindow);

  // Grab the image background:
  vtkNew<vtkImageData> background;
  if (!this->RasterizeBackground(background.GetPointer()))
  {
    vtkErrorMacro("Error rasterizing background image. Exported image may be "
                  "incorrect.");
    background->Initialize();
    // Continue with export.
  }

  // Fixup options for no-opengl-context GL2PS rendering (we don't use the
  // context since we inject all geometry manually):
  options |= GL2PS_NO_OPENGL_CONTEXT | GL2PS_NO_BLENDING;
  // Print warning if the user requested no background -- we always draw it.
  if ((options & GL2PS_DRAW_BACKGROUND) == GL2PS_NONE)
  {
    vtkWarningMacro("Ignoring DrawBackground=false setting. The background is "
                    "always drawn on the OpenGL2 backend for GL2PS exports.");
  }
  // Turn the option off -- we don't want GL2PS drawing the background, it
  // comes from the raster image we draw in the image.
  options &= ~GL2PS_DRAW_BACKGROUND;

  // Export file. No worries about buffersize, since we're manually adding
  // geometry through vtkOpenGLGL2PSHelper::ProcessTransformFeedback.
  GLint err = gl2psBeginPage(title.c_str(), "VTK", viewport, format, sort,
                             options, GL_RGBA, 0, NULL, 0, 0, 0, 0, file,
                             fname.str().c_str());
  if (err != GL2PS_SUCCESS)
  {
    vtkErrorMacro("Error calling gl2psBeginPage. Error code: " << err);
    gl2ps->SetInstance(NULL);
    fclose(file);
    return;
  }

  if (background->GetNumberOfPoints() > 0)
  {
    int dims[3];
    background->GetDimensions(dims);
    GL2PSvertex rasterPos;
    rasterPos.xyz[0] = 0.f;
    rasterPos.xyz[1] = 0.f;
    rasterPos.xyz[2] = 1.f;
    std::fill(rasterPos.rgba, rasterPos.rgba + 4, 0.f);

    gl2psForceRasterPos(&rasterPos);
    gl2psDrawPixels(dims[0], dims[1], 0, 0, GL_RGB, GL_FLOAT,
                    background->GetScalarPointer());
    background->ReleaseData();
  }

  // Render the scene:
  if (!this->CaptureVectorProps())
  {
    vtkErrorMacro("Error capturing vectorizable props. Resulting image "
                  "may be incorrect.");
  }

  // Cleanup
  err = gl2psEndPage();
  gl2ps->SetInstance(NULL);
  fclose(file);

  switch (err)
  {
    case GL2PS_SUCCESS:
      break;
    case GL2PS_NO_FEEDBACK:
      vtkErrorMacro("No data captured by GL2PS for vector graphics export.");
      break;
    default:
      vtkErrorMacro("Error calling gl2psEndPage. Error code: " << err);
      break;
  }

  // Re-render the window to remove any lingering after-effects...
  this->RenderWindow->Render();
}

bool vtkOpenGLGL2PSExporter::RasterizeBackground(vtkImageData *image)
{
  vtkNew<vtkWindowToImageFilter> windowToImage;
  windowToImage->SetInput(this->RenderWindow);
  windowToImage->SetInputBufferTypeToRGB();
  windowToImage->SetReadFrontBuffer(false);

  vtkNew<vtkImageShiftScale> byteToFloat;
  byteToFloat->SetOutputScalarTypeToFloat();
  byteToFloat->SetScale(1.0 / 255.0);
  byteToFloat->SetInputConnection(windowToImage->GetOutputPort());

  vtkOpenGLGL2PSHelper *gl2ps = vtkOpenGLGL2PSHelper::GetInstance();
  gl2ps->SetActiveState(vtkOpenGLGL2PSHelper::Background);
  // Render twice to set the backbuffer:
  this->RenderWindow->Render();
  this->RenderWindow->Render();
  byteToFloat->Update();
  gl2ps->SetActiveState(vtkOpenGLGL2PSHelper::Inactive);

  image->ShallowCopy(byteToFloat->GetOutput());

  return true;
}

bool vtkOpenGLGL2PSExporter::CaptureVectorProps()
{
  vtkOpenGLGL2PSHelper *gl2ps = vtkOpenGLGL2PSHelper::GetInstance();
  gl2ps->SetActiveState(vtkOpenGLGL2PSHelper::Capture);
  this->RenderWindow->Render();
  gl2ps->SetActiveState(vtkOpenGLGL2PSHelper::Inactive);

  return true;
}
