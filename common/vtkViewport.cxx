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

  this->Aspect[0] = this->Aspect[1] = 1.0;

  this->StartRenderMethod = NULL;
  this->StartRenderMethodArgDelete = NULL;
  this->StartRenderMethodArg = NULL;
  this->EndRenderMethod = NULL;
  this->EndRenderMethodArgDelete = NULL;
  this->EndRenderMethodArg = NULL;
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

