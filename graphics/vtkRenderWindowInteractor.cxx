/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRenderWindowInteractor.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1996 Ken Martin, Will Schroeder, Bill Lorensen.

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
#ifdef _WIN32
#include "vtkWin32RenderWindowInteractor.h"
#else
#include "vtkXRenderWindowInteractor.h"
#endif
#include "vtkActor.h"
#include "vtkCellPicker.h"

// Description:
// Construct object so that light follows camera motion.
vtkRenderWindowInteractor::vtkRenderWindowInteractor()
{
  this->RenderWindow    = NULL;
  this->CurrentCamera   = NULL;
  this->CurrentLight    = NULL;
  this->CurrentRenderer = NULL;

  this->LightFollowCamera = 1;
  this->Initialized = 0;
  this->DesiredUpdateRate = 15;
  // default limit is 3 hours per frame
  this->StillUpdateRate = 0.0001;
  
  this->SelfCreatedPicker = 0;
  this->Picker = this->CreateDefaultPicker();
  this->OutlineActor = NULL;
  this->OutlineMapper = vtkPolyMapper::New();  
  this->OutlineMapper->SetInput(this->Outline.GetOutput());
  this->PickedRenderer = NULL;
  this->CurrentActor = NULL;

  this->StartPickMethod = NULL;
  this->StartPickMethodArgDelete = NULL;
  this->StartPickMethodArg = NULL;
  this->EndPickMethod = NULL;
  this->EndPickMethodArgDelete = NULL;
  this->EndPickMethodArg = NULL;
  this->UserMethod = NULL;
  this->UserMethodArgDelete = NULL;
  this->UserMethodArg = NULL;
}

vtkRenderWindowInteractor::~vtkRenderWindowInteractor()
{
  if ( this->OutlineActor ) this->OutlineActor->Delete();
  if ( this->SelfCreatedPicker && this->Picker) this->Picker->Delete();
}

vtkRenderWindowInteractor *vtkRenderWindowInteractor::New()
{
#ifdef _WIN32
  return vtkWin32RenderWindowInteractor::New();
#else
  return vtkXRenderWindowInteractor::New();
#endif  
  return new vtkRenderWindowInteractor;
}

void vtkRenderWindowInteractor::SetRenderWindow(vtkRenderWindow *aren)
{
  this->RenderWindow = aren;
  if (this->RenderWindow->GetInteractor() != this)
    {
    this->RenderWindow->SetInteractor(this);
    }
}
void vtkRenderWindowInteractor::FindPokedRenderer(int x,int y)
{
  vtkRendererCollection *rc;
  vtkRenderer *aren;

  this->CurrentRenderer = NULL;

  rc = this->RenderWindow->GetRenderers();
  
  for (rc->InitTraversal(); 
       ((aren = rc->GetNextItem())&&(!this->CurrentRenderer));)
    {
    if (aren->IsInViewport(x,y))
      {
      this->CurrentRenderer = aren;
      }
    }
  
  // we must have a value 
  if (this->CurrentRenderer == NULL)
    {
    rc->InitTraversal();
    aren = rc->GetNextItem();
    this->CurrentRenderer = aren;
    }
}

void  vtkRenderWindowInteractor::FindPokedCamera(int x,int y)
{
  float *vp;
  vtkLightCollection *lc;

  this->FindPokedRenderer(x,y);
  vp = this->CurrentRenderer->GetViewport();

  this->CurrentCamera = this->CurrentRenderer->GetActiveCamera();  
  memcpy(this->Center,this->CurrentRenderer->GetCenter(),sizeof(int)*2);
  this->DeltaElevation = -20.0/((vp[3] - vp[1])*this->Size[1]);
  this->DeltaAzimuth = -20.0/((vp[2] - vp[0])*this->Size[0]);

  // as a side effect also set the light 
  // in case they are using light follow camera 
  lc = this->CurrentRenderer->GetLights();
  lc->InitTraversal();
  this->CurrentLight = lc->GetNextItem();
}

// Description:
// When pick action successfully selects actor, this method highlights the 
// actor appropriately. Currently this is done by placing a bounding box
// around the actor.
void vtkRenderWindowInteractor::HighlightActor(vtkActor *actor)
{
  if ( ! this->OutlineActor )
    {
    // have to defer creation to get right type
    this->OutlineActor = vtkActor::New();
    this->OutlineActor->PickableOff();
    this->OutlineActor->DragableOff();
    this->OutlineActor->SetMapper(this->OutlineMapper);
    this->OutlineActor->GetProperty()->SetColor(1.0,1.0,1.0);
    this->OutlineActor->GetProperty()->SetAmbient(1.0);
    this->OutlineActor->GetProperty()->SetDiffuse(0.0);
    }

  if ( this->PickedRenderer ) 
    this->PickedRenderer->RemoveActors(OutlineActor);

  if ( ! actor )
    {
    this->PickedRenderer = NULL;
    }
  else 
    {
    this->PickedRenderer = this->CurrentRenderer;
    this->CurrentRenderer->AddActors(OutlineActor);
    this->Outline.SetBounds(actor->GetBounds());
    this->CurrentActor = actor;
    }
  this->RenderWindow->Render();
}

// Description:
// Specify a method to be executed prior to the pick operation.
void vtkRenderWindowInteractor::SetStartPickMethod(void (*f)(void *), void *arg)
{
  if ( f != this->StartPickMethod || arg != this->StartPickMethodArg )
    {
    this->StartPickMethod = f;
    this->StartPickMethodArg = arg;
    this->Modified();
    }
}

// Description:
// Specify a method to be executed after the pick operation.
void vtkRenderWindowInteractor::SetEndPickMethod(void (*f)(void *), void *arg)
{
  if ( f != this->EndPickMethod || arg != this->EndPickMethodArg )
    {
    this->EndPickMethod = f;
    this->EndPickMethodArg = arg;
    this->Modified();
    }
}

// Description:
// Set the object used to perform pick operations. You can use this to 
// control what type of data is picked.
void vtkRenderWindowInteractor::SetPicker(vtkPicker *picker)
{
  if ( this->Picker != picker ) 
    {
    if ( this->SelfCreatedPicker ) this->Picker->Delete();
    this->SelfCreatedPicker = 0;
    this->Picker = picker;
    this->Modified();
    }
}

vtkPicker *vtkRenderWindowInteractor::CreateDefaultPicker()
{
  if ( this->SelfCreatedPicker ) this->Picker->Delete();
  this->SelfCreatedPicker = 1;
  return new vtkCellPicker;
}

// Description:
// Set the user method. This method is invoked on a <u> keypress.
void vtkRenderWindowInteractor::SetUserMethod(void (*f)(void *), void *arg)
{
  if ( f != this->UserMethod || arg != this->UserMethodArg )
    {
    // delete the current arg if there is one and a delete meth
    if ((this->UserMethodArg)&&(this->UserMethodArgDelete))
      {
      (*this->UserMethodArgDelete)(this->UserMethodArg);
      }
    this->UserMethod = f;
    this->UserMethodArg = arg;
    this->Modified();
    }
}

// Description:
// Called when a void* argument is being discarded.  Lets the user free it.
void vtkRenderWindowInteractor::SetUserMethodArgDelete(void (*f)(void *))
{
  if ( f != this->UserMethodArgDelete)
    {
    this->UserMethodArgDelete = f;
    this->Modified();
    }
}

// Description:
// Called when a void* argument is being discarded.  Lets the user free it.
void vtkRenderWindowInteractor::SetStartPickMethodArgDelete(void (*f)(void *))
{
  if ( f != this->StartPickMethodArgDelete)
    {
    this->StartPickMethodArgDelete = f;
    this->Modified();
    }
}
// Description:
// Called when a void* argument is being discarded.  Lets the user free it.
void vtkRenderWindowInteractor::SetEndPickMethodArgDelete(void (*f)(void *))
{
  if ( f != this->EndPickMethodArgDelete)
    {
    this->EndPickMethodArgDelete = f;
    this->Modified();
    }
}

void vtkRenderWindowInteractor::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  os << indent << "RenderWindow:    " << this->RenderWindow << "\n";
  os << indent << "CurrentCamera:   " << this->CurrentCamera << "\n";
  os << indent << "CurrentLight:    " << this->CurrentLight << "\n";
  os << indent << "CurrentRenderer: " << this->CurrentRenderer << "\n";
  if ( this->Picker )
    {
    os << indent << "Picker: " << this->Picker << "\n";
    }
  else
    {
    os << indent << "Picker: (none)\n";
    }
  os << indent << "LightFollowCamera: " << (this->LightFollowCamera ? "On\n" : "Off\n");
  os << indent << "DesiredUpdateRate: " << this->DesiredUpdateRate << "\n";
  os << indent << "StillUpdateRate: " << this->StillUpdateRate << "\n";
}

