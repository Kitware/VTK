/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPropPicker.cxx
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
#include "vtkPropPicker.h"
#include "vtkObjectFactory.h"


vtkPropPicker::vtkPropPicker()
{
  this->PickFromProps = NULL;
}

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


// set up for a pick
void vtkPropPicker::Initialize()
{
  this->PickedProp = 0;
  this->vtkPicker::Initialize();
}

// Pick from the given collection
int vtkPropPicker::Pick(float selectionX, float selectionY, float vtkNotUsed(z),
			vtkRenderer *renderer)
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
int vtkPropPicker::PickProp(float selectionX, float selectionY, vtkRenderer *renderer)
{
  // Invoke start pick method if defined
  if ( this->StartPickMethod ) 
    {
    (*this->StartPickMethod)(this->StartPickMethodArg);
    } 
  //  Initialize picking process
  this->Renderer = renderer;
  this->SelectionPoint[0] = selectionX;
  this->SelectionPoint[1] = selectionY;
  this->SelectionPoint[2] = 0;
  this->Initialize();

  // Have the renderer do the hardware pick
  this->PickedProp = renderer->PickPropFrom(selectionX, selectionY, this->PickFromProps);
  // If there was a pick then find the world x,y,z for the pick
  if(this->PickedProp)
    {
    // save the start and end methods, so that 
    // vtkWorldPointPicker will not call them
    void (*SaveStartPickMethod)(void *) = this->StartPickMethod;
    void (*SaveEndPickMethod)(void *) = this->EndPickMethod;
    this->StartPickMethod = 0;
    this->EndPickMethod = 0;
    vtkWorldPointPicker::Pick(selectionX, selectionY, 0, 
			      renderer);
    this->StartPickMethod = SaveStartPickMethod;
    this->EndPickMethod = SaveEndPickMethod;
    } 
  if(this->EndPickMethod)
    {
    (*this->EndPickMethod)(this->EndPickMethodArg);
    }
  // Call Pick on the Prop that was picked, and return 1 for sucess
  if(this->PickedProp)
    {
    this->PickedProp->Pick();
    if ( this->PickMethod )
      {
      (*this->PickMethod)(this->PickMethodArg);
      }
    return 1;
    }
  return 0;
}

void vtkPropPicker::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkWorldPointPicker::PrintSelf(os, indent);
  os << indent << "PickedProp:    " << this->PickedProp << endl;
  os << indent << "PickFrom List: " << this->PickFromProps << endl;
}
