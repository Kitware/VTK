/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkViewport.cxx
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
#include <stdlib.h>
#include <string.h>

#include "vtkViewport.h"
#include "vtkWindow.h"
#include "vtkActor2DCollection.h"
#include "vtkPropCollection.h"
#include "vtkProp.h"
#include "vtkCommand.h"

// Create a vtkViewport with a black background, a white ambient light, 
// two-sided lighting turned on, a viewport of (0,0,1,1), and backface culling
// turned off.
vtkViewport::vtkViewport()
{
  this->VTKWindow = NULL;
  
  this->Background[0] = 0;
  this->Background[1] = 0;
  this->Background[2] = 0;

  this->Viewport[0] = 0;
  this->Viewport[1] = 0;
  this->Viewport[2] = 1;
  this->Viewport[3] = 1;

  this->WorldPoint[0] = 0;
  this->WorldPoint[1] = 0;
  this->WorldPoint[2] = 0;
  this->WorldPoint[3] = 0;

  this->DisplayPoint[0] = 0;
  this->DisplayPoint[1] = 0;
  this->DisplayPoint[2] = 0;

  this->ViewPoint[0] = 0;
  this->ViewPoint[1] = 0;
  this->ViewPoint[2] = 0;

  this->Aspect[0] = this->Aspect[1] = 1.0;
  this->PixelAspect[0] = this->PixelAspect[1] = 1.0;

  this->StartTag = 0;
  this->EndTag = 0;

  this->Size[0] = 0;
  this->Size[1] = 0;

  this->Origin[0] = 0;
  this->Origin[1] = 0;
  
  this->PickedProp = NULL;
  this->PickFromProps = NULL;
  this->IsPicking = 0;
  this->CurrentPickId = 0;
  this->PickX = -1;
  this->PickY = -1;


  this->Props = vtkPropCollection::New();
  this->Actors2D = vtkActor2DCollection::New();
}

vtkViewport::~vtkViewport()
{
  this->Actors2D->Delete();
  this->Actors2D = NULL;
  this->Props->Delete();
  this->Props = NULL;
  
  if (this->VTKWindow != NULL)
    {
    // renderer never reference counted the window.
    // loop is too hard to detect.
    // this->VTKWindow->UnRegister(this);
    this->VTKWindow = NULL;
    }
  
  if ( this->PickedProp != NULL )
    {
    this->PickedProp->UnRegister(this);
    }
}

void vtkViewport::RemoveActor2D(vtkProp* p)
{
  this->Actors2D->RemoveItem(p);
  this->RemoveProp(p);
}
void vtkViewport::AddProp(vtkProp *p)
{
  this->Props->AddItem(p);
}
void vtkViewport::RemoveProp(vtkProp *p)
{
  this->Props->RemoveItem(p);
}

// look through the props and get all the actors
vtkActor2DCollection *vtkViewport::GetActors2D()
{
  vtkProp *aProp;
  
  // clear the collection first
  this->Actors2D->RemoveAllItems();
  
  for (this->Props->InitTraversal(); 
       (aProp = this->Props->GetNextProp()); )
    {
    aProp->GetActors2D(this->Actors2D);
    }
  return this->Actors2D;
}

// Convert display coordinates to view coordinates.
void vtkViewport::DisplayToView()
{
  if ( this->VTKWindow )
    {
    float vx,vy,vz;
    int sizex,sizey;
    int *size;

    /* get physical window dimensions */
    size = this->VTKWindow->GetSize();
    sizex = size[0];
    sizey = size[1];

    vx = 2.0 * (this->DisplayPoint[0] - sizex*this->Viewport[0])/ 
      (sizex*(this->Viewport[2]-this->Viewport[0])) - 1.0;
    vy = 2.0 * (this->DisplayPoint[1] - sizey*this->Viewport[1])/ 
      (sizey*(this->Viewport[3]-this->Viewport[1])) - 1.0;
    vz = this->DisplayPoint[2];

    this->SetViewPoint(vx*this->Aspect[0],vy*this->Aspect[1],vz);
    }
}

// Convert view coordinates to display coordinates.
void vtkViewport::ViewToDisplay()
{
  if ( this->VTKWindow )
    {
    float dx,dy;
    int sizex,sizey;
    int *size;

    /* get physical window dimensions */
    size = this->VTKWindow->GetSize();
    sizex = size[0];
    sizey = size[1];

    dx = (this->ViewPoint[0]/this->Aspect[0] + 1.0) * 
      (sizex*(this->Viewport[2]-this->Viewport[0])) / 2.0 +
        sizex*this->Viewport[0];
    dy = (this->ViewPoint[1]/this->Aspect[1] + 1.0) * 
      (sizey*(this->Viewport[3]-this->Viewport[1])) / 2.0 +
        sizey*this->Viewport[1];

    this->SetDisplayPoint(dx,dy,this->ViewPoint[2]);
    }
}

// Convert view point coordinates to world coordinates.
void vtkViewport::ViewToWorld()
{   
  this->SetWorldPoint(this->ViewPoint[0], this->ViewPoint[1],
		      this->ViewPoint[2], 1);
}

// Convert world point coordinates to view coordinates.
void vtkViewport::WorldToView()
{

  this->SetViewPoint(this->WorldPoint[0], this->WorldPoint[1],
		     this->WorldPoint[2]);

}


// Return the size of the viewport in display coordinates.
int *vtkViewport::GetSize()
{  
  if ( this->VTKWindow )
    {
    int  lowerLeft[2];
    float *vport = this->GetViewport();
    
    float vpu, vpv;
    vpu = vport[0];
    vpv = vport[1];  
    this->NormalizedDisplayToDisplay(vpu,vpv);
    lowerLeft[0] = (int)(vpu+0.5);
    lowerLeft[1] = (int)(vpv+0.5);
    float vpu2, vpv2;
    vpu2 = vport[2];
    vpv2 = vport[3];  
    this->NormalizedDisplayToDisplay(vpu2,vpv2);
    this->Size[0] = (int)(vpu2 + 0.5) - lowerLeft[0];
    this->Size[1] = (int)(vpv2 + 0.5) - lowerLeft[1];  
    }
  else
    {
    this->Size[0] = this->Size[1] = 0;
    }

  return this->Size;
}

// Return the origin of the viewport in display coordinates.
int *vtkViewport::GetOrigin()
{
  if ( this->VTKWindow )
    {
    int* winSize = this->VTKWindow->GetSize();

    // Round the origin up a pixel
    this->Origin[0] = (int) (this->Viewport[0] * (float) winSize[0] + 0.5);
    this->Origin[1] = (int) (this->Viewport[1] * (float) winSize[1] + 0.5);
    }
  else
    {
    this->Origin[0] = this->Origin[1] = 0;
    }

  return this->Origin;
}

  
// Return the center of this Viewport in display coordinates.
float *vtkViewport::GetCenter()
{
  if ( this->VTKWindow )
    {
    int *size;
  
    // get physical window dimensions 
    size = this->GetVTKWindow()->GetSize();

    this->Center[0] = ((this->Viewport[2]+this->Viewport[0])
                       /2.0*(float)size[0]);
    this->Center[1] = ((this->Viewport[3]+this->Viewport[1])
                       /2.0*(float)size[1]);
    }
  else
    {
    this->Center[0] = this->Center[1] = 0;
    }

  return this->Center;
}

// Is a given display point in this Viewport's viewport.
int vtkViewport::IsInViewport(int x,int y)
{
  if ( this->VTKWindow )
    {
    int *size;
  
    // get physical window dimensions 
    size = this->GetVTKWindow()->GetSize();

    if ((this->Viewport[0]*size[0] <= x)&&
        (this->Viewport[2]*size[0] >= x)&&
        (this->Viewport[1]*size[1] <= y)&&
        (this->Viewport[3]*size[1] >= y))
      {
      return 1;
      }
    }

  return 0;
}

// Specify a function to be called before rendering process begins.
// Function will be called with argument provided.
void vtkViewport::SetStartRenderMethod(void (*f)(void *), void *arg)
{
  vtkOldStyleCallbackCommand *cbc = new vtkOldStyleCallbackCommand;
  cbc->Callback = f;
  cbc->ClientData = arg;
  this->RemoveObserver(this->StartTag);
  this->StartTag = this->AddObserver(vtkCommand::StartEvent,cbc);
}

// Set the arg delete method. This is used to free user memory.
void vtkViewport::SetStartRenderMethodArgDelete(void (*f)(void *))
{
  vtkOldStyleCallbackCommand *cmd = 
    (vtkOldStyleCallbackCommand *)this->GetCommand(this->StartTag);
  if (cmd)
    {
    cmd->SetClientDataDeleteCallback(f);
    }
}

// Set the arg delete method. This is used to free user memory.
void vtkViewport::SetEndRenderMethodArgDelete(void (*f)(void *))
{
  vtkOldStyleCallbackCommand *cmd = 
    (vtkOldStyleCallbackCommand *)this->GetCommand(this->EndTag);
  if (cmd)
    {
    cmd->SetClientDataDeleteCallback(f);
    }
}

// Specify a function to be called when rendering process completes.
// Function will be called with argument provided.
void vtkViewport::SetEndRenderMethod(void (*f)(void *), void *arg)
{
  vtkOldStyleCallbackCommand *cbc = new vtkOldStyleCallbackCommand;
  cbc->Callback = f;
  cbc->ClientData = arg;
  this->RemoveObserver(this->EndTag);
  this->EndTag = this->AddObserver(vtkCommand::EndEvent,cbc);
}

void vtkViewport::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os,indent);

  os << indent << "Aspect: (" << this->Aspect[0] << ", " 
    << this->Aspect[1] << ")\n";

  os << indent << "PixelAspect: (" << this->PixelAspect[0] << ", " 
    << this->PixelAspect[1] << ")\n";

  os << indent << "Background: (" << this->Background[0] << ", " 
    << this->Background[1] << ", "  << this->Background[2] << ")\n";

  os << indent << "Viewport: (" << this->Viewport[0] << ", " 
    << this->Viewport[1] << ", " << this->Viewport[2] << ", " 
      << this->Viewport[3] << ")\n";

  os << indent << "Displaypoint: (" << this->DisplayPoint[0] << ", " 
    << this->DisplayPoint[1] << ", " << this->DisplayPoint[2] << ")\n";

  os << indent << "Viewpoint: (" << this->ViewPoint[0] << ", " 
    << this->ViewPoint[1] << ", " << this->ViewPoint[2] << ")\n";

  os << indent << "Worldpoint: (" << this->WorldPoint[0] << ", " 
    << this->WorldPoint[1] << ", " << this->WorldPoint[2] << ", " 
      << this->WorldPoint[3] << ")\n";

  if ( this->StartTag)
    {
    os << indent << "Start Render method defined.\n";
    }
  else
    {
    os << indent << "No Start Render method.\n";
    }

  if ( this->EndTag )
    {
    os << indent << "End Render method defined.\n";
    }
  else
    {
    os << indent << "No End Render method.\n";
    }
  os << indent << "Pick Position X Y: " << this->PickX 
     << " " << this->PickY << endl;
  os << indent << "IsPicking boolean: " << this->IsPicking << endl;
  os << indent << "Props:\n";
  this->Props->PrintSelf(os,indent.GetNextIndent());

}

void vtkViewport::LocalDisplayToDisplay(float &vtkNotUsed(u), float &v)
{
  if ( this->VTKWindow )
    {
    int *size;
  
    /* get physical window dimensions */
    size = this->VTKWindow->GetSize();
    
    v = size[1] - v - 1;
    }
}

void vtkViewport::DisplayToLocalDisplay(float &vtkNotUsed(u), float &v)
{
  if ( this->VTKWindow )
    {
    int *size;
  
    /* get physical window dimensions */
    size = this->VTKWindow->GetSize();
  
    v = size[1] - v - 1;
    }
}

void vtkViewport::DisplayToNormalizedDisplay(float &u, float &v)
{
  if ( this->VTKWindow )
    {
    int *size;
  
    /* get physical window dimensions */
    size = this->VTKWindow->GetSize();
    
    u = u/size[0];
    v = v/size[1];
    }
}

void vtkViewport::NormalizedDisplayToViewport(float &u, float &v)
{
  if ( this->VTKWindow )
    {
    // get the pixel value for the viewport origin
    float vpou, vpov;    
    vpou = this->Viewport[0];
    vpov = this->Viewport[1];
    this->NormalizedDisplayToDisplay(vpou,vpov);
    
    // get the pixel value for the coordinate
    this->NormalizedDisplayToDisplay(u,v);
    
    // subtract the vpo
    u = u - vpou - 0.5;
    v = v - vpov - 0.5;
    }
}

void vtkViewport::ViewportToNormalizedViewport(float &u, float &v)
{
  if ( this->VTKWindow )
    {
    int *size;
    
    /* get physical window dimensions */
/*
  float vpsizeu, vpsizev;
  size = this->VTKWindow->GetSize();
  vpsizeu = size[0]*(this->Viewport[2] - this->Viewport[0]);
  vpsizev = size[1]*(this->Viewport[3] - this->Viewport[1]);
  
  u = u/(vpsizeu - 1.0);
  v = v/(vpsizev - 1.0);
*/
    size = this->GetSize();
    u = u/(size[0] - 1.0);
    v = v/(size[1] - 1.0);
    }
}

void vtkViewport::NormalizedViewportToView(float &x, float &y, 
                                           float &vtkNotUsed(z))
{
  x = (2.0*x - 1.0)*this->Aspect[0];
  y = (2.0*y - 1.0)*this->Aspect[1];
}

void vtkViewport::NormalizedDisplayToDisplay(float &u, float &v)
{
  if ( this->VTKWindow )
    {
    int *size;

    /* get physical window dimensions */
    size = this->VTKWindow->GetSize();

    u = u*size[0];
    v = v*size[1];
    }
}

  
void vtkViewport::ViewportToNormalizedDisplay(float &u, float &v)
{
  if ( this->VTKWindow )
    {
    // get the pixel value for the viewport origin
    float vpou, vpov;    
    vpou = this->Viewport[0];
    vpov = this->Viewport[1];
    this->NormalizedDisplayToDisplay(vpou,vpov);

    // add the vpo
    // the 0.5 offset is here because the viewport uses pixel centers
    // while the display uses pixel edges. 
    u = u + vpou + 0.5;
    v = v + vpov + 0.5;

    // get the pixel value for the coordinate
    this->DisplayToNormalizedDisplay(u,v);
    }
}

void vtkViewport::NormalizedViewportToViewport(float &u, float &v)
{
  if ( this->VTKWindow )
    {
    int *size;
    
    /* get physical window dimensions */
/*
  float vpsizeu, vpsizev;
  size = this->VTKWindow->GetSize();
  vpsizeu = size[0]*(this->Viewport[2] - this->Viewport[0]);
  vpsizev = size[1]*(this->Viewport[3] - this->Viewport[1]);
  u = u * (vpsizeu - 1.0);
  v = v * (vpsizev - 1.0);
*/
    size = this->GetSize();
    u = u * (size[0] - 1.0);
    v = v * (size[1] - 1.0);    
    }
}

void vtkViewport::ViewToNormalizedViewport(float &x, float &y, 
                                           float &vtkNotUsed(z))
{
  x =  (x / this->Aspect[0] + 1.0) / 2.0;
  y =  (y / this->Aspect[1] + 1.0) / 2.0;
}

void vtkViewport::ComputeAspect()
{
  if ( this->VTKWindow )
    {
    float aspect[2];
    float *vport;
    int  *size, lowerLeft[2], upperRight[2];

    // get the bounds of the window 
    size = this->VTKWindow->GetSize();

    vport = this->GetViewport();

    lowerLeft[0] = (int)(vport[0]*size[0] + 0.5);
    lowerLeft[1] = (int)(vport[1]*size[1] + 0.5);
    upperRight[0] = (int)(vport[2]*size[0] + 0.5);
    upperRight[1] = (int)(vport[3]*size[1] + 0.5);
    upperRight[0]--;
    upperRight[1]--;

    aspect[0] = (float)(upperRight[0]-lowerLeft[0]+1)/
      (float)(upperRight[1]-lowerLeft[1]+1)*this->PixelAspect[0];
    aspect[1] = 1.0*this->PixelAspect[1];

    this->SetAspect(aspect);
    }
}

vtkAssemblyPath* vtkViewport::PickPropFrom(float selectionX, float selectionY, 
                                          vtkPropCollection* pickfrom)
{
  this->PickFromProps = pickfrom;
  return this->PickProp(selectionX, selectionY);
}

