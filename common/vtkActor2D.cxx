/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkActor2D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Matt Turek who developed this class.

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

#include "vtkActor2D.h"
#include "vtkProperty2D.h"
#include "vtkMapper2D.h"

// Description:
// Creates an actor2D with the following defaults: 
// position -1, -1 (view coordinates)
// orientation 0, scale (1,1), layer 0, visibility on
vtkActor2D::vtkActor2D()
{
  this->Orientation = 0.0;
  this->Scale[0] = 1.0;
  this->Scale[1] = 1.0;
  this->LayerNumber = 0;
  this->Visibility = 1;  // ON
  this->SelfCreatedProperty = 0;
  this->Property = (vtkProperty2D*) NULL;
  this->PositionType = VTK_VIEW_COORD;
  this->DisplayPosition[0] = 0;
  this->DisplayPosition[1] = 0;
  this->ViewPosition[0] = -1.0;
  this->ViewPosition[1] = -1.0;
  this->WorldPosition[0] = 0;
  this->WorldPosition[1] = 0;
  this->WorldPosition[2] = 0;
  this->Mapper = (vtkMapper2D*) NULL;
}

// Description:
// Destroy an actor2D.  If the actor2D created it's own
// property, that property is deleted.
vtkActor2D::~vtkActor2D()
{
  if (this->SelfCreatedProperty) this->Property->Delete();
}

void vtkActor2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkObject::PrintSelf(os,indent);
  os << indent << "Orientation: " << this->Orientation << "\n";
  os << indent << "Scale: (" << this->Scale[0] << ", " << this->Scale[1] << ")\n";
  os << indent << "Layer Number: " << this->LayerNumber << "\n";
  os << indent << "Visibility: " << (this->Visibility ? "On\n" : "Off\n");

  char posString[64];
  switch (this->PositionType)
    {
    case VTK_VIEW_COORD:
 	strcpy(posString, "VTK_VIEW_COORD\0");
	break;
    case VTK_DISPLAY_COORD:
	strcpy(posString, "VTK_DISPLAY_COORD\0");
	break;
    case VTK_WORLD_COORD:
	strcpy(posString, "VTK_WORLD_COORD\0");
 	break; 
    default:
	strcpy(posString, "UNKNOWN!\0");
	break;
    }

  os << indent << "Position Type: " << posString << "\n";
  os << indent << "Display Position: (" << this->DisplayPosition[0] << "," 
     << this->DisplayPosition[1] << ")\n";
  os << indent << "View Position: (" << this->ViewPosition[0] << "," 
     << this->ViewPosition[1] << ")\n";
  os << indent << "World Position: (" << this->WorldPosition[0] << "," 
     << this->WorldPosition[1] << "," << this->WorldPosition[2] << ")\n";
  os << indent << "Self Created Property: " << (this->SelfCreatedProperty ? "Yes\n" : "No\n");
  os << indent << "Property: " << this->Property << "\n";
  if (this->Property) this->Property->PrintSelf(os, indent.GetNextIndent());
  os << indent << "Mapper: " << this->Mapper << "\n";
  if (this->Mapper) this->Mapper->PrintSelf(os, indent.GetNextIndent());

}

// Description:
// Sets the actor2D's position in view coordinates.  Updates
// the PositionType.
void vtkActor2D::SetViewPosition(float XPos, float YPos)
{

  if ((XPos != this->ViewPosition[0]) || (YPos != this->ViewPosition[1]))
    {
    this->ViewPosition[0] = XPos;
    this->ViewPosition[1] = YPos;
    this->Modified();
    }
  this->PositionType = VTK_VIEW_COORD;
}

// Description:
// Set the actor2D's position in display coordinates.  Updates
// the PositionType.
void vtkActor2D::SetDisplayPosition(int XPos, int YPos)
{
  if ((XPos != this->DisplayPosition[0]) || (YPos != this->DisplayPosition[1]))
    {
    this->DisplayPosition[0] = XPos;
    this->DisplayPosition[1] = YPos;
    this->Modified();
    }
  this->PositionType = VTK_DISPLAY_COORD;
}

// Description:
// Set the actor2D's position in world coordinates.  Updates 
// the PositionType.  To have an actor2D follow a regular actor,
// just set the 2D actor's world position to the position of the
// regular actor.  
void vtkActor2D::SetWorldPosition(float XPos, float YPos, float ZPos)
{
  if (  (XPos != this->WorldPosition[0]) || 
 	(YPos != this->WorldPosition[1]) ||
	(ZPos != this->WorldPosition[2]))
    {
    this->WorldPosition[0] = XPos;
    this->WorldPosition[1] = YPos;
    this->WorldPosition[2] = ZPos;
    this->Modified();
    }
  this->PositionType = VTK_WORLD_COORD;
}

// Description:
// Returns the actor's pixel position relative to the viewports
// lower left corner
int *vtkActor2D::GetComputedViewportPixelPosition(vtkViewport* viewport)
{
  float* actorPos;
  float  temp[2];
  // Get the actor's position in viewport coordinates
  switch (this->PositionType) 
    {
    case VTK_VIEW_COORD:
	viewport->SetViewPoint(this->ViewPosition[0], this->ViewPosition[1], 0);
	viewport->ViewToDisplay();
        actorPos = viewport->GetDisplayPoint();
      	break;
    case VTK_DISPLAY_COORD:
        // Put the int DisplayPosition into a float array 
        // and point actorPos at that array
        temp[0] = this->DisplayPosition[0];
	temp[1] = this->DisplayPosition[1];
	actorPos = temp;
	break;
    case VTK_WORLD_COORD:
	viewport->SetWorldPoint(this->WorldPosition[0], this->WorldPosition[1], 
				this->WorldPosition[2], 1);
	viewport->WorldToDisplay();
        actorPos = viewport->GetDisplayPoint();
	break;
    default:
	vtkErrorMacro(<< "Unknown position type: " << this->PositionType);
	break;
    }

  int* winSize = viewport->GetVTKWindow()->GetSize();
  float* vpt = viewport->GetViewport(); 

  this->ComputedDisplayPosition[0] = 
    (int)(actorPos[0] + 0.5 - vpt[0]*winSize[0]);
  this->ComputedDisplayPosition[1] = 
    (int)(actorPos[1] + 0.5 - vpt[1]*winSize[1]);
  
  return this->ComputedDisplayPosition;
}

// Description:
// Returns the actor's pixel position in a window with origin
// in the upper left (drawing origin for both X and Win32).
int *vtkActor2D::GetComputedDisplayPosition(vtkViewport* viewport)
{
  float* actorPos;
  float  temp[2];

  // Get the actor's position in viewport coordinates
  switch (this->PositionType) 
    {
    case VTK_VIEW_COORD:
	viewport->SetViewPoint(this->ViewPosition[0], this->ViewPosition[1], 0);
	viewport->ViewToDisplay();
        actorPos = viewport->GetDisplayPoint();
      	break;
    case VTK_DISPLAY_COORD:
        // Put the int DisplayPosition into a float array 
        // and point actorPos at that array
        temp[0] = this->DisplayPosition[0];
	temp[1] = this->DisplayPosition[1];
	actorPos = temp;
	break;
    case VTK_WORLD_COORD:
	viewport->SetWorldPoint(this->WorldPosition[0], this->WorldPosition[1], 
				this->WorldPosition[2], 1);
	viewport->WorldToDisplay();
        actorPos = viewport->GetDisplayPoint();
	break;
    default:
	vtkErrorMacro(<< "Unknown position type: " << this->PositionType);
	break;
    }

  int* winSize = viewport->GetVTKWindow()->GetSize();

  this->ComputedDisplayPosition[0] = (int)(actorPos[0] + 0.5);

  // X and Win32 drawing origins have 0,0 at top left
  this->ComputedDisplayPosition[1] = (int)(winSize[1] - (actorPos[1] + 0.5));

  return this->ComputedDisplayPosition;
}

float *vtkActor2D::GetComputedWorldPosition(vtkViewport* viewport)
{
  float* actorPos;
  static float worldPos[3];

  // Get the actor's position in viewport coordinates
  switch (this->PositionType) 
    {
    case VTK_VIEW_COORD:
	viewport->SetViewPoint(this->ViewPosition[0], this->ViewPosition[1], 0);
	viewport->ViewToWorld();
        actorPos = viewport->GetWorldPoint();
	worldPos[0] = actorPos[0];
	worldPos[1] = actorPos[1];
	worldPos[2] = actorPos[2];
      	break;
    case VTK_DISPLAY_COORD:
        viewport->SetDisplayPoint(this->DisplayPosition[0], this->DisplayPosition[1], 0);
	viewport->DisplayToWorld();
        actorPos = viewport->GetWorldPoint();
	worldPos[0] = actorPos[0];
	worldPos[1] = actorPos[1];
	worldPos[2] = actorPos[2];
	break;
    case VTK_WORLD_COORD:
        worldPos[0] = this->WorldPosition[0];
	worldPos[1] = this->WorldPosition[1];
	worldPos[2] = this->WorldPosition[2];
	break;
    default:
	vtkErrorMacro(<< "Unknown position type: " << this->PositionType);
	break;
    }

  return worldPos;    

}

// Description:
// Renders an actor2D's property and then it's mapper.
void vtkActor2D::Render (vtkViewport* viewport)
{
  vtkDebugMacro(<< "vtkActor2D::Render");

  if (!this->Property)
    {
    vtkDebugMacro(<< "vtkActor2D::Render - Creating Property2D");
    // Force creation of default property
    this->GetProperty();
    }

  this->Property->Render(viewport);

  if (!this->Mapper) 
    {
    vtkErrorMacro(<< "vtkActor2D::Render - No mapper set");
    return;
    }

  vtkDebugMacro(<<"vtkActor2D::Render - Rendering mapper");
  this->Mapper->Render(viewport, this); 

}

// Description:
// Returns an actor2D's property2D.  Creates a property if one
// doesn't already exist.
vtkProperty2D *vtkActor2D::GetProperty()
{
  if (this->Property == NULL)
    {
    this->Property = vtkProperty2D::New();
    this->SelfCreatedProperty = 1;
    this->Modified();
    }
  return this->Property;
}






