
#include "vtkActor2D.h"
#include "vtkProperty2D.h"
#include "vtkMapper2D.h"

// VTK_VIEW_COORD    0
// VTK_DISPLAY_COORD 1
// VTK_WORLD_COORD   2

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

  this->ComputedDisplayPosition[0] = actorPos[0] + 0.5;

  // X and Win32 drawing origins have 0,0 at top left
  this->ComputedDisplayPosition[1] = winSize[1] - (actorPos[1] + 0.5);

  return this->ComputedDisplayPosition;
}

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
}


vtkActor2D::~vtkActor2D()
{
  if (this->SelfCreatedProperty) delete this->Property;
}

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






