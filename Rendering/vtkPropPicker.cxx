/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPropPicker.cxx
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
#include "vtkPropPicker.h"
#include "vtkWorldPointPicker.h"
#include "vtkAssemblyNode.h"
#include "vtkObjectFactory.h"


vtkPropPicker* vtkPropPicker::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkPropPicker");
  if(ret)
    {
    return (vtkPropPicker*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkPropPicker;
}

vtkPropPicker::vtkPropPicker()
{
  this->PickFromProps = NULL;
  this->WorldPointPicker = vtkWorldPointPicker::New();
}

vtkPropPicker::~vtkPropPicker()
{
  this->WorldPointPicker->Delete();
}

// set up for a pick
void vtkPropPicker::Initialize()
{
  this->vtkAbstractPropPicker::Initialize();
}

// Pick from the given collection
int vtkPropPicker::Pick(float selectionX, float selectionY, 
                        float vtkNotUsed(z), vtkRenderer *renderer)
{
  return this->PickProp(selectionX, selectionY, renderer);
}


// Pick from the given collection
int vtkPropPicker::PickProp(float selectionX, float selectionY,
			    vtkRenderer *renderer, vtkPropCollection* pickfrom)
{
  this->PickFromProps = pickfrom;
  int ret = this->PickProp(selectionX, selectionY, renderer);
  this->PickFromProps = NULL;
  return ret;
}



// Perform pick operation with selection point provided. The z location
// is recovered from the zBuffer. Always returns 0 since no actors are picked.
int vtkPropPicker::PickProp(float selectionX, float selectionY, 
                            vtkRenderer *renderer)
{
  //  Initialize picking process
  this->Initialize();
  this->Renderer = renderer;
  this->SelectionPoint[0] = selectionX;
  this->SelectionPoint[1] = selectionY;
  this->SelectionPoint[2] = 0;

  // Invoke start pick method if defined
  this->InvokeEvent(vtkCommand::StartPickEvent,NULL);

  // Have the renderer do the hardware pick
  this->SetPath(
    renderer->PickPropFrom(selectionX, selectionY, this->PickFromProps));

  // If there was a pick then find the world x,y,z for the pick, and invoke
  // its pick method.
  if ( this->Path )
    {
    this->WorldPointPicker->Pick(selectionX, selectionY, 0, renderer);
    this->WorldPointPicker->GetPickPosition(this->PickPosition);
    this->Path->GetLastNode()->GetProp()->Pick();
    this->InvokeEvent(vtkCommand::PickEvent,NULL);
    } 

  this->InvokeEvent(vtkCommand::EndPickEvent,NULL);

  // Call Pick on the Prop that was picked, and return 1 for success
  if ( this->Path )
    {
    return 1;
    }
  else
    {
    return 0;
    }
}

void vtkPropPicker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkAbstractPropPicker::PrintSelf(os, indent);

  if (this->PickFromProps)
    {
    os << indent << "PickFrom List: " << this->PickFromProps << endl;
    }
  else
    {
    os << indent << "PickFrom List: (none)" << endl;
    }

}
