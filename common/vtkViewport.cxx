/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkViewport.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include <stdlib.h>
#include <string.h>

#include "vtkViewport.h"
#include "vtkWindow.h"

// Description:
// Create a vtkViewport with a black background, a white ambient light, 
// two-sided lighting turned on, a viewport of (0,0,1,1), and backface culling
// turned off.
vtkViewport::vtkViewport()
{
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

  this->StartRenderMethod = NULL;
  this->StartRenderMethodArgDelete = NULL;
  this->StartRenderMethodArg = NULL;
  this->EndRenderMethod = NULL;
  this->EndRenderMethodArgDelete = NULL;
  this->EndRenderMethodArg = NULL;

  this->Size[0] = 0;
  this->Size[1] = 0;

  this->Origin[0] = 0;
  this->Origin[1] = 0;

  this->Actors2D = vtkActor2DCollection::New();
}

vtkViewport::~vtkViewport()
{
  this->Actors2D->Delete();

  // delete the current arg if there is one and a delete meth
  if ((this->StartRenderMethodArg)&&(this->StartRenderMethodArgDelete))
    {
    (*this->StartRenderMethodArgDelete)(this->StartRenderMethodArg);
    }
  if ((this->EndRenderMethodArg)&&(this->EndRenderMethodArgDelete))
    {
    (*this->EndRenderMethodArgDelete)(this->EndRenderMethodArg);
    }
}

void vtkViewport::AddActor2D(vtkActor2D* actor)
{
  vtkDebugMacro (<< "vtkViewport::AddActor2D");
  this->Actors2D->AddItem(actor);
}

void vtkViewport::RemoveActor2D(vtkActor2D* actor)
{
  vtkDebugMacro (<< "vtkViewport::RemoveActor2D");

  this->Actors2D->RemoveItem(actor);
}




// Description:
// Convert display coordinates to view coordinates.
void vtkViewport::DisplayToView()
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

// Description:
// Convert view coordinates to display coordinates.
void vtkViewport::ViewToDisplay()
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

// Description:
// Convert view point coordinates to world coordinates.
void vtkViewport::ViewToWorld()
{   
  this->SetWorldPoint(this->ViewPoint[0], this->ViewPoint[1],
		      this->ViewPoint[2], 1);
}

// Description:
// Convert world point coordinates to view coordinates.
void vtkViewport::WorldToView()
{

  this->SetViewPoint(this->WorldPoint[0], this->WorldPoint[1],
		     this->WorldPoint[2]);

}


// Description:
// Return the size of the viewport in display coordinates.
int *vtkViewport::GetSize()
{  
  // Get the window size
  int* winSize = this->VTKWindow->GetSize();

  // Calculate a width and height for the viewport 
  float vptWidth = this->Viewport[2] - this->Viewport[0];
  float vptHeight = this->Viewport[3] - this->Viewport[1];

  // Round the size up
  this->Size[0] = (int) (vptWidth * (float) winSize[0] + 0.5);
  this->Size[1] = (int) (vptHeight * (float) winSize[1] + 0.5);

  return this->Size;
}

// Description:
// Return the origin of the viewport in display coordinates.
int *vtkViewport::GetOrigin()
{
  int* winSize = this->VTKWindow->GetSize();

  // Round the origin up a pixel
  this->Origin[0] = (int) (this->Viewport[0] * (float) winSize[0] + 0.5);
  this->Origin[1] = (int) (this->Viewport[1] * (float) winSize[1] + 0.5);

  return this->Origin;
}

  
// Description:
// Return the center of this Viewport in display coordinates.
float *vtkViewport::GetCenter()
{
  int *size;
  
  // get physical window dimensions 
  size = this->GetVTKWindow()->GetSize();

  this->Center[0] = ((this->Viewport[2]+this->Viewport[0])
		     /2.0*(float)size[0]);
  this->Center[1] = ((this->Viewport[3]+this->Viewport[1])
		     /2.0*(float)size[1]);

  return this->Center;
}

// Description:
// Is a given display point in this Viewport's viewport.
int vtkViewport::IsInViewport(int x,int y)
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

  return 0;
}

// Description:
// Specify a function to be called before rendering process begins.
// Function will be called with argument provided.
void vtkViewport::SetStartRenderMethod(void (*f)(void *), void *arg)
{
  if ( f != this->StartRenderMethod || arg != this->StartRenderMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->StartRenderMethodArg)&&(this->StartRenderMethodArgDelete))
      {
      (*this->StartRenderMethodArgDelete)(this->StartRenderMethodArg);
      }
    this->StartRenderMethod = f;
    this->StartRenderMethodArg = arg;
    this->Modified();
    }
}

// Description:
// Set the arg delete method. This is used to free user memory.
void vtkViewport::SetStartRenderMethodArgDelete(void (*f)(void *))
{
  if ( f != this->StartRenderMethodArgDelete)
    {
    this->StartRenderMethodArgDelete = f;
    this->Modified();
    }
}

// Description:
// Set the arg delete method. This is used to free user memory.
void vtkViewport::SetEndRenderMethodArgDelete(void (*f)(void *))
{
  if ( f != this->EndRenderMethodArgDelete)
    {
    this->EndRenderMethodArgDelete = f;
    this->Modified();
    }
}

// Description:
// Specify a function to be called when rendering process completes.
// Function will be called with argument provided.
void vtkViewport::SetEndRenderMethod(void (*f)(void *), void *arg)
{
  if ( f != this->EndRenderMethod || arg != EndRenderMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->EndRenderMethodArg)&&(this->EndRenderMethodArgDelete))
      {
      (*this->EndRenderMethodArgDelete)(this->EndRenderMethodArg);
      }
    this->EndRenderMethod = f;
    this->EndRenderMethodArg = arg;
    this->Modified();
    }
}

void vtkViewport::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os,indent);

  os << indent << "Aspect: (" << this->Aspect[0] << ", " 
    << this->Aspect[1] << ")\n";
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

  if ( this->StartRenderMethod )
    {
    os << indent << "Start Render method defined.\n";
    }
  else
    {
    os << indent << "No Start Render method.\n";
    }

  if ( this->EndRenderMethod )
    {
    os << indent << "End Render method defined.\n";
    }
  else
    {
    os << indent << "No End Render method.\n";
    }

}

void vtkViewport::LocalDisplayToDisplay(float &u, float &v)
{
  int *size;
  
  /* get physical window dimensions */
  size = this->VTKWindow->GetSize();
  
  v = size[1] - v - 1;
}

void vtkViewport::DisplayToLocalDisplay(float &u, float &v)
{
  int *size;
  
  /* get physical window dimensions */
  size = this->VTKWindow->GetSize();
  
  v = size[1] - v - 1;
}

void vtkViewport::DisplayToNormalizedDisplay(float &u, float &v)
{
  int *size;
  
  /* get physical window dimensions */
  size = this->VTKWindow->GetSize();
  
  if (size[0] > 1)
    {
    u = u / (size[0] - 1);
    }
  else 
    {
    u = 0.0;
    }
  
  if (size[1] > 1)
    {
    v = v / (size[1] - 1);
    }
  else
    {
    v = 0.0;
    }
}

void vtkViewport::NormalizedDisplayToViewport(float &u, float &v)
{
  int *size;
  
  /* get physical window dimensions */
  size = this->VTKWindow->GetSize();
  
  u = u - this->Viewport[0];
  v = v - this->Viewport[1];

  u = u * (size[0] - 1);
  v = v * (size[1] - 1);
}

void vtkViewport::ViewportToNormalizedViewport(float &u, float &v)
{
  int *size;
  
  /* get physical window dimensions */
  size = this->VTKWindow->GetSize();

  if (size[0] > 1)
    {
    u = u / (size[0] - 1);
    }
  else 
    {
    u = 0.0;
    }
  
  if (size[1] > 1)
    {
    v = v / (size[1] - 1);
    }
  else
    {
    v = 0.0;
    }

  u = u / (this->Viewport[2] - this->Viewport[0]);
  v = v / (this->Viewport[3] - this->Viewport[1]);
}

void vtkViewport::NormalizedViewportToView(float &x, float &y, float &z)
{
  x = (2.0*x - 1.0)*this->Aspect[0];
  y = (2.0*y - 1.0)*this->Aspect[1];
}

void vtkViewport::NormalizedDisplayToDisplay(float &u, float &v)
{
  int *size;
  
  /* get physical window dimensions */
  size = this->VTKWindow->GetSize();
  
  u = u * (size[0] - 1);
  v = v * (size[1] - 1);
}

  
void vtkViewport::ViewportToNormalizedDisplay(float &u, float &v)
{
  int *size;
  
  /* get physical window dimensions */
  size = this->VTKWindow->GetSize();
  
  if (size[0] > 1)
    {
    u = u / (size[0] - 1);
    }
  else 
    {
    u = 0.0;
    }
  
  if (size[1] > 1)
    {
    v = v / (size[1] - 1);
    }
  else
    {
    v = 0.0;
    }
  
  u = u + this->Viewport[0];
  v = v + this->Viewport[1];
}

void vtkViewport::NormalizedViewportToViewport(float &u, float &v)
{
  int *size;
  
  /* get physical window dimensions */
  size = this->VTKWindow->GetSize();
  
  u = u * (this->Viewport[2] - this->Viewport[0]);
  v = v * (this->Viewport[3] - this->Viewport[1]);
  
  u = u * (size[0] - 1);
  v = v * (size[1] - 1);
}

void vtkViewport::ViewToNormalizedViewport(float &x, float &y, float &z)
{
  x =  (x / this->Aspect[0] + 1.0) / 2.0;
  y =  (y / this->Aspect[1] + 1.0) / 2.0;
}






