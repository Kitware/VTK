/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkActor.cxx
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
#include <math.h>

#include "vtkActor.h"
#include "vtkRenderWindow.h"

// Description:
// Creates an actor with the following defaults: origin(0,0,0) 
// position=(0,0,0) scale=(1,1,1) visibility=1 pickable=1 dragable=1
// orientation=(0,0,0). No user defined matrix and no texture map.
vtkActor::vtkActor()
{
  this->UserMatrix = NULL;
  this->Mapper = NULL;
  this->Property = NULL;
  this->BackfaceProperty = NULL;
  this->Texture = NULL;

  this->Scale[0] = 1.0;
  this->Scale[1] = 1.0;
  this->Scale[2] = 1.0;

  this->SelfCreatedProperty = 0;
  this->TraversalLocation = 0;
}

vtkActor::~vtkActor()
{
  if ( this->SelfCreatedProperty && this->Property != NULL) 
    this->Property->Delete();

}

// Description:
// Shallow copy of an actor.
vtkActor& vtkActor::operator=(const vtkActor& actor)
{
  int i;

  this->UserMatrix = actor.UserMatrix;
  this->Mapper = actor.Mapper;
  this->Property = actor.Property;
  this->BackfaceProperty = actor.BackfaceProperty;
  this->Texture = actor.Texture;

  *((vtkProp *)this) = actor;

  for (i=0; i < 3; i++) 
    {
    this->Scale[i] = actor.Scale[i];
    }

  return *this;
}


#ifdef VTK_USE_GLR
#include "vtkGLActor.h"
#endif
#ifdef VTK_USE_OGLR
#include "vtkOpenGLActor.h"
#endif
#ifdef VTK_USE_SBR
#include "vtkStarbaseActor.h"
#endif
#ifdef VTK_USE_XGLR
#include "vtkXGLActor.h"
#endif
#ifdef _WIN32
#include "vtkOpenGLActor.h"
#endif
// return the correct type of Actor 
vtkActor *vtkActor::New()
{
  char *temp = vtkRenderWindow::GetRenderLibrary();
  
#ifdef VTK_USE_SBR
  if (!strcmp("Starbase",temp)) return vtkStarbaseActor::New();
#endif
#ifdef VTK_USE_GLR
  if (!strcmp("GL",temp)) return vtkGLActor::New();
#endif
#ifdef VTK_USE_OGLR
  if (!strcmp("OpenGL",temp)) return vtkOpenGLActor::New();
#endif
#ifdef _WIN32
  if (!strcmp("Win32OpenGL",temp)) return vtkOpenGLActor::New();
#endif
#ifdef VTK_USE_XGLR
  if (!strcmp("XGL",temp)) return vtkXGLActor::New();
#endif
  
  return new vtkActor;
}

// Description:
// This causes the actor to be rendered. It in turn will render the actor's
// property, texture map and then mapper. If a property hasn't been 
// assigned, then the actor will create one automatically. Note that a 
// side effect of this method is that the visualization network is updated.
void vtkActor::Render(vtkRenderer *ren)
{
  if ( ! this->Mapper ) return;

  // render the property
  if (!this->Property)
    {
    // force creation of a property
    this->GetProperty();
    }
  this->Property->Render(this, ren);

  // render the backface property
  if (this->BackfaceProperty)
    {
    this->BackfaceProperty->BackfaceRender(this, ren);
    }

  // render the texture */
  if (this->Texture) this->Texture->Render(ren);

  this->Render(ren,this->Mapper);
}

void vtkActor::SetProperty(vtkProperty *lut)
{
  if ( this->Property != lut ) 
    {
    if ( this->SelfCreatedProperty ) this->Property->Delete();
    this->SelfCreatedProperty = 0;
    this->Property = lut;
    this->Modified();
    }
}

vtkProperty *vtkActor::GetProperty()
{
  if ( this->Property == NULL )
    {
    this->Property = vtkProperty::New();
    this->SelfCreatedProperty = 1;
    }
  return this->Property;
}

void vtkActor::SetBackfaceProperty(vtkProperty *lut)
{
  if ( this->BackfaceProperty != lut ) 
    {
    this->BackfaceProperty = lut;
    this->Modified();
    }
}

vtkProperty *vtkActor::GetBackfaceProperty()
{
  return this->BackfaceProperty;
}

// Description:
// Copy the actor's composite 4x4 matrix into the matrix provided.
void vtkActor::GetMatrix(vtkMatrix4x4& result)
{
  this->GetOrientation();
  this->Transform.Push();  
  this->Transform.Identity();  
  this->Transform.PostMultiply();  

  // shift back to actor's origin
  this->Transform.Translate(-this->Origin[0],
			    -this->Origin[1],
			    -this->Origin[2]);

  // scale
  this->Transform.Scale(this->Scale[0],
			this->Scale[1],
			this->Scale[2]);

  // rotate
  this->Transform.RotateY(this->Orientation[1]);
  this->Transform.RotateX(this->Orientation[0]);
  this->Transform.RotateZ(this->Orientation[2]);

  // move back from origin and translate
  this->Transform.Translate(this->Origin[0] + this->Position[0],
			    this->Origin[1] + this->Position[1],
			    this->Origin[2] + this->Position[2]);
   
  // apply user defined matrix last if there is one 
  if (this->UserMatrix)
    {
    this->Transform.Concatenate(*this->UserMatrix);
    }

  this->Transform.PreMultiply();  
  result = this->Transform.GetMatrix();

  this->Transform.Pop();  
} 

// Description:
// Get the bounds for this Actor as (Xmin,Xmax,Ymin,Ymax,Zmin,Zmax).
float *vtkActor::GetBounds()
{
  int i,n;
  float *bounds, bbox[24], *fptr;
  float *result;
  vtkMatrix4x4 matrix;
  
  // get the bounds of the Mapper if we have one
  if (!this->Mapper)
    {
    return this->Bounds;
    }

  bounds = this->Mapper->GetBounds();

  // fill out vertices of a bounding box
  bbox[ 0] = bounds[1]; bbox[ 1] = bounds[3]; bbox[ 2] = bounds[5];
  bbox[ 3] = bounds[1]; bbox[ 4] = bounds[2]; bbox[ 5] = bounds[5];
  bbox[ 6] = bounds[0]; bbox[ 7] = bounds[2]; bbox[ 8] = bounds[5];
  bbox[ 9] = bounds[0]; bbox[10] = bounds[3]; bbox[11] = bounds[5];
  bbox[12] = bounds[1]; bbox[13] = bounds[3]; bbox[14] = bounds[4];
  bbox[15] = bounds[1]; bbox[16] = bounds[2]; bbox[17] = bounds[4];
  bbox[18] = bounds[0]; bbox[19] = bounds[2]; bbox[20] = bounds[4];
  bbox[21] = bounds[0]; bbox[22] = bounds[3]; bbox[23] = bounds[4];
  
  // save the old transform
  this->GetMatrix(matrix);
  this->Transform.Push(); 
  this->Transform.PostMultiply ();
  this->Transform.Identity();
  this->Transform.Concatenate(matrix);

  // and transform into actors coordinates
  fptr = bbox;
  for (n = 0; n < 8; n++) 
    {
    this->Transform.SetPoint(fptr[0],fptr[1],fptr[2],1.0);
  
    // now store the result
    result = this->Transform.GetPoint();
    fptr[0] = result[0] / result[3];
    fptr[1] = result[1] / result[3];
    fptr[2] = result[2] / result[3];
    fptr += 3;
    }
  
  this->Transform.PreMultiply ();
  this->Transform.Pop();  
  
  // now calc the new bounds
  this->Bounds[0] = this->Bounds[2] = this->Bounds[4] = VTK_LARGE_FLOAT;
  this->Bounds[1] = this->Bounds[3] = this->Bounds[5] = -VTK_LARGE_FLOAT;
  for (i = 0; i < 8; i++)
    {
    for (n = 0; n < 3; n++)
      {
      if (bbox[i*3+n] < this->Bounds[n*2]) this->Bounds[n*2] = bbox[i*3+n];
      if (bbox[i*3+n] > this->Bounds[n*2+1]) this->Bounds[n*2+1] = bbox[i*3+n];
      }
    }

  return this->Bounds;
}

vtkActor *vtkActor::GetNextPart()
{
  if ( this->TraversalLocation++ == 0 ) return this;
  else return NULL;
}

unsigned long int vtkActor::GetMTime()
{
  unsigned long mTime=this->vtkObject::GetMTime();
  unsigned long time;

  if ( this->Property != NULL )
    {
    time = this->Property->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  if ( this->UserMatrix != NULL )
    {
    time = this->UserMatrix->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  if ( this->Texture != NULL )
    {
    time = this->Texture->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

// Description:
// Update visualization pipeline and any other parts of actor that are
// necessary.
void vtkActor::Update()
{
  if ( this->Mapper )
    {
    this->Mapper->Update();
    }
}

// This method is used in conjunction with the assembly object to build a copy
// of the assembly hierarchy. This hierarchy can then be traversed for 
// rendering or other operations.
void vtkActor::BuildPaths(vtkAssemblyPaths *vtkNotUsed(paths), 
                          vtkActorCollection *path)
{
  vtkActor *copy= vtkActor::New();
  vtkActor *previous;

  *copy = *this;

  previous = path->GetLastItem();

  vtkMatrix4x4 *matrix = vtkMatrix4x4::New();
  *matrix = previous->vtkProp::GetMatrix();
  copy->SetUserMatrix(matrix);

  path->AddItem(copy);
}

void vtkActor::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkProp::PrintSelf(os,indent);

  // make sure our bounds are up to date
  if ( this->Mapper )
    {
    this->GetBounds();
    os << indent << "Bounds: \n";
    os << indent << "  Xmin,Xmax: (" << this->Bounds[0] << ", " << this->Bounds[1] << ")\n";
    os << indent << "  Ymin,Ymax: (" << this->Bounds[2] << ", " << this->Bounds[3] << ")\n";
    os << indent << "  Zmin,Zmax: (" << this->Bounds[4] << ", " << this->Bounds[5] << ")\n";
    }
  else
    {
    os << indent << "Bounds: (not defined)\n";
    }

  if ( this->Mapper )
    {
    os << indent << "Mapper:\n";
    this->Mapper->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Mapper: (none)\n";
    }
  if ( this->Property )
    {
    os << indent << "Property:\n";
    this->Property->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Property: (none)\n";
    }
  os << indent << "Scale: (" << this->Scale[0] << ", " 
     << this->Scale[1] << ", " << this->Scale[2] << ")\n";
}

