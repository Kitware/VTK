/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLRenderer.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include <math.h>
#include "vtkOpenGLRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkOpenGLProperty.h"
#include "vtkOpenGLCamera.h"
#include "vtkOpenGLLight.h"
#include "vtkCuller.h"

// if this .cxx file is being used to implement
// the mesa renderer, then do not include GL/gl.h
// because with the mesa renderer we need to include
// mesagl.h instead
#ifndef VTK_IMPLEMENT_MESA_CXX
#ifdef __APPLE__
#include <OpenGL/gl.h>
#else
#include <GL/gl.h>
#endif
#endif
#include "vtkObjectFactory.h"

class vtkGLPickInfo
{
public:
  GLuint* PickBuffer;
  GLuint PickedId;
};

#ifndef VTK_IMPLEMENT_MESA_CXX
//---------------------------------------------------------------------------
vtkOpenGLRenderer* vtkOpenGLRenderer::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkOpenGLRenderer");
  if(ret)
    {
    return (vtkOpenGLRenderer*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkOpenGLRenderer;
}
#endif

#define VTK_MAX_LIGHTS 8

vtkOpenGLRenderer::vtkOpenGLRenderer()
{
  this->PickInfo = new vtkGLPickInfo;
  this->NumberOfLightsBound = 0;
  this->PickInfo->PickBuffer = 0;
  this->PickInfo->PickedId = 0;
  this->PickedZ = 0;
}

// Internal method temporarily removes lights before reloading them
// into graphics pipeline.
void vtkOpenGLRenderer::ClearLights (void)
{
  short curLight;
  float Info[4];

  // define a lighting model and set up the ambient light.
  // use index 11 for the heck of it. Doesn't matter except for 0.
   
  // update the ambient light 
  Info[0] = this->Ambient[0];
  Info[1] = this->Ambient[1];
  Info[2] = this->Ambient[2];
  Info[3] = 1.0;
  glLightModelfv(GL_LIGHT_MODEL_AMBIENT, Info);

  if ( this->TwoSidedLighting )
    {
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, GL_TRUE);
    }
  else
    {
    glLightModeli(GL_LIGHT_MODEL_TWO_SIDE, 0);
    }

  // now delete all the old lights 
  for (curLight = GL_LIGHT0; curLight < GL_LIGHT0 + VTK_MAX_LIGHTS; curLight++)
    {
    glDisable((GLenum)curLight);
    }

  this->NumberOfLightsBound = 0;
}

// Ask lights to load themselves into graphics pipeline.
int vtkOpenGLRenderer::UpdateLights ()
{
  vtkLight *light;
  short curLight;
  float status;
  int count;

  // Check if a light is on. If not then make a new light.
  count = 0;
  curLight= this->NumberOfLightsBound + GL_LIGHT0;

  for(this->Lights->InitTraversal(); 
      (light = this->Lights->GetNextItem()); )
    {
    status = light->GetSwitch();
    if ((status > 0.0)&& (curLight < (GL_LIGHT0+VTK_MAX_LIGHTS)))
      {
      curLight++;
      count++;
      }
    }

  if( !count )
    {
    vtkDebugMacro(<<"No lights are on, creating one.");
    this->CreateLight();
    }

  count = 0;
  curLight= this->NumberOfLightsBound + GL_LIGHT0;

  // set the matrix mode for lighting. ident matrix on viewing stack  
  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();

  for(this->Lights->InitTraversal(); 
      (light = this->Lights->GetNextItem()); )
    {

    status = light->GetSwitch();

    // if the light is on then define it and bind it. 
    // also make sure we still have room.             
    if ((status > 0.0)&& (curLight < (GL_LIGHT0+VTK_MAX_LIGHTS)))
      {
      light->Render((vtkRenderer *)this,curLight);
      glEnable((GLenum)curLight);
      // increment the current light by one 
      curLight++;
      count++;
      }
    }
  
  this->NumberOfLightsBound = curLight - GL_LIGHT0;
  
  glPopMatrix();
  glEnable(GL_LIGHTING);
  return count;
}

// Concrete open gl render method.
void vtkOpenGLRenderer::DeviceRender(void)
{
  // Do not remove this MakeCurrent! Due to Start / End methods on
  // some objects which get executed during a pipeline update, 
  // other windows might get rendered since the last time
  // a MakeCurrent was called.
  this->RenderWindow->MakeCurrent();

  // standard render method 
  this->ClearLights();

  this->UpdateCamera();
  this->UpdateLightGeometry();
  this->UpdateLights();

  // set matrix mode for actors 
  glMatrixMode(GL_MODELVIEW);

  this->UpdateGeometry();

  // clean up the model view matrix set up by the camera 
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}


void vtkOpenGLRenderer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkRenderer::PrintSelf(os,indent);

  os << indent << "Number Of Lights Bound: " << 
    this->NumberOfLightsBound << "\n";
  os << indent << "PickBuffer " << this->PickInfo->PickBuffer << "\n";
  os << indent << "PickedId" << this->PickInfo->PickedId<< "\n";
  os << indent << "PickedZ " << this->PickedZ << "\n";
}


void vtkOpenGLRenderer::Clear(void)
{
  GLbitfield  clear_mask = 0;

  if (! this->Transparent())
    {
    glClearColor( ((GLclampf)(this->Background[0])),
                  ((GLclampf)(this->Background[1])),
                  ((GLclampf)(this->Background[2])),
                  ((GLclampf)(1.0)) );
    clear_mask |= GL_COLOR_BUFFER_BIT;
    }

  glClearDepth( (GLclampd)( 1.0 ) );
  clear_mask |= GL_DEPTH_BUFFER_BIT;

  vtkDebugMacro(<< "glClear\n");
  glClear(clear_mask);
}

void vtkOpenGLRenderer::StartPick(unsigned int pickFromSize)
{
  int bufferSize = pickFromSize * 4;
  this->PickInfo->PickBuffer = new GLuint[bufferSize];
  glSelectBuffer(bufferSize, this->PickInfo->PickBuffer);
  // change to selection mode
  (void)glRenderMode(GL_SELECT);
  // initialize the pick names and add a 0 name, for no pick
  glInitNames();
  glPushName(0);
}



void vtkOpenGLRenderer::UpdatePickId()
{
  glLoadName(this->CurrentPickId++);
}


void vtkOpenGLRenderer::DevicePickRender()
{ 
  // Do not remove this MakeCurrent! Due to Start / End methods on
  // some objects which get executed during a pipeline update, 
  // other windows might get rendered since the last time
  // a MakeCurrent was called.
  this->RenderWindow->MakeCurrent();

  // standard render method 
  this->ClearLights();

  this->UpdateCamera();
  this->UpdateLightGeometry();
  this->UpdateLights();

  // set matrix mode for actors 
  glMatrixMode(GL_MODELVIEW);

  this->PickGeometry();

  // clean up the model view matrix set up by the camera 
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
}


void vtkOpenGLRenderer::DonePick()
{
  glFlush();
  GLuint hits = glRenderMode(GL_RENDER); 
  unsigned int depth = (unsigned int)-1;
  GLuint* ptr = this->PickInfo->PickBuffer;
  this->PickInfo->PickedId = 0;
  for(unsigned int k =0; k < hits; k++)
    {
    int num_names = *ptr;
    int save = 0;
    ptr++; // move to first depth value
    if(*ptr <= depth)      
      {
      depth = *ptr;
      save = 1;
      }
    ptr++; // move to next depth value
    if(*ptr <= depth)      
      {
      depth = *ptr;
      save = 1;
      }
    // move to first name picked
    ptr++;
    if(save)
      {
      this->PickInfo->PickedId = *ptr;
      }
    // skip additonal names
    ptr += num_names;
    }
  // If there was a pick, then get the Z value
  if(this->PickInfo->PickedId)
    {
    // convert from pick depth described as:
    // Returned depth values are mapped such that the largest unsigned 
    // integer value corresponds to window coordinate depth 1.0, 
    // and zero corresponds to window coordinate depth 0.0.
    
    this->PickedZ = ((double)depth / (double)VTK_UNSIGNED_INT_MAX);

    // Clamp to range [0,1]
    this->PickedZ = (this->PickedZ < 0.0) ? 0.0 : this->PickedZ;
    this->PickedZ = (this->PickedZ > 1.0) ? 1.0: this->PickedZ;
    }
  delete [] this->PickInfo->PickBuffer;
  this->PickInfo->PickBuffer = 0;
}

float vtkOpenGLRenderer::GetPickedZ()
{
  return this->PickedZ;
}

unsigned int vtkOpenGLRenderer::GetPickedId()
{
  return (unsigned int)this->PickInfo->PickedId;
}

vtkOpenGLRenderer::~vtkOpenGLRenderer()
{
  if (this->PickInfo->PickBuffer)
    {
    delete [] this->PickInfo->PickBuffer;
    this->PickInfo->PickBuffer = 0;
    }
  delete this->PickInfo;
}

