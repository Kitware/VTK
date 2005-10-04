/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLScalarsToColorsPainter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkOpenGLScalarsToColorsPainter.h"

#include "vtkActor.h"
#include "vtkCellData.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkMapper.h" //for VTK_MATERIALMODE_*
#include "vtkObjectFactory.h"
#include "vtkOpenGLTexture.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkProperty.h"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"


#ifndef VTK_IMPLEMENT_MESA_CXX
#  include "vtkOpenGL.h"
#endif

#ifndef VTK_IMPLEMENT_MESA_CXX
vtkStandardNewMacro(vtkOpenGLScalarsToColorsPainter);
vtkCxxRevisionMacro(vtkOpenGLScalarsToColorsPainter, "1.1.2.2");
#endif
//-----------------------------------------------------------------------------
vtkOpenGLScalarsToColorsPainter::vtkOpenGLScalarsToColorsPainter()
{
  this->InternalColorTexture = 0;
}

//-----------------------------------------------------------------------------
vtkOpenGLScalarsToColorsPainter::~vtkOpenGLScalarsToColorsPainter()
{
  if (this->InternalColorTexture)
    {
    this->InternalColorTexture->Delete();
    this->InternalColorTexture = 0;
    }
}

//-----------------------------------------------------------------------------
void vtkOpenGLScalarsToColorsPainter::ReleaseGraphicsResources(vtkWindow* win)
{
  if (this->InternalColorTexture)
    {
    this->InternalColorTexture->ReleaseGraphicsResources(win);
    }
  this->Superclass::ReleaseGraphicsResources(win);
}

//-----------------------------------------------------------------------------
void vtkOpenGLScalarsToColorsPainter::RenderInternal(vtkRenderer* renderer, 
  vtkActor* actor, unsigned long typeflags)
{
  vtkProperty* prop = actor->GetProperty();

  // If we are coloring by texture, then load the texture map.
  if (this->ColorTextureMap)
    {
    if (this->InternalColorTexture == 0)
      {
      this->InternalColorTexture = vtkOpenGLTexture::New();
      this->InternalColorTexture->RepeatOff();
      }
    this->InternalColorTexture->SetInput(this->ColorTextureMap);
    // Keep color from interacting with texture.
    float info[4];
    info[0] = info[1] = info[2] = info[3] = 1.0;
    glMaterialfv( GL_FRONT_AND_BACK, GL_DIFFUSE, info );
    
    this->LastWindow = renderer->GetRenderWindow();
    }
  else if (this->LastWindow)
    {
    // release the texture.
    this->ReleaseGraphicsResources(this->LastWindow);
    this->LastWindow = 0;
    }


  // if we are doing vertex colors then set lmcolor to adjust 
  // the current materials ambient and diffuse values using   
  // vertex color commands otherwise tell it not to.          
  glDisable( GL_COLOR_MATERIAL );
  vtkDataArray* c = this->OutputData->GetPointData()->GetScalars();
  c = (c)? c : this->OutputData->GetCellData()->GetScalars();
  c = (c)? c : this->OutputData->GetFieldData()->GetArray("Color");
  if (c)
    {
    GLenum lmcolorMode;
    if (this->ScalarMaterialMode == VTK_MATERIALMODE_DEFAULT)
      {
      if (prop->GetAmbient() > prop->GetDiffuse())
        {
        lmcolorMode = GL_AMBIENT;
        }
      else
        {
        lmcolorMode = GL_DIFFUSE;
        }
      }
    else if (this->ScalarMaterialMode == VTK_MATERIALMODE_AMBIENT_AND_DIFFUSE)
      {
      lmcolorMode = GL_AMBIENT_AND_DIFFUSE;
      }
    else if (this->ScalarMaterialMode == VTK_MATERIALMODE_AMBIENT)
      {
      lmcolorMode = GL_AMBIENT;
      }
    else // if (this->ScalarMaterialMode == VTK_MATERIALMODE_DIFFUSE)
      {
      lmcolorMode = GL_DIFFUSE;
      } 
    glColorMaterial( GL_FRONT_AND_BACK, lmcolorMode);
    glEnable( GL_COLOR_MATERIAL );
    }
  if (this->ColorTextureMap)
    {
    this->InternalColorTexture->Load(renderer);
    }
  this->Superclass::RenderInternal(renderer, actor, typeflags);
}

//-----------------------------------------------------------------------------
void vtkOpenGLScalarsToColorsPainter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
