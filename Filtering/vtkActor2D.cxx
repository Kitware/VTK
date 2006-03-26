/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkActor2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkActor2D.h"

#include "vtkProperty2D.h"
#include "vtkMapper2D.h"
#include "vtkPropCollection.h"
#include "vtkObjectFactory.h"
#include "vtkTexture.h"
#include "vtkRenderer.h"

vtkCxxRevisionMacro(vtkActor2D, "1.2");
vtkStandardNewMacro(vtkActor2D);

vtkCxxSetObjectMacro(vtkActor2D,Property, vtkProperty2D);
vtkCxxSetObjectMacro(vtkActor2D,Texture,vtkTexture);

// Creates an actor2D with the following defaults:
// position -1, -1 (view coordinates)
// orientation 0, scale (1,1), layer 0, visibility on
vtkActor2D::vtkActor2D()
{
  this->Mapper = (vtkMapper2D*) NULL;
  this->LayerNumber = 0;
  this->Property = (vtkProperty2D*) NULL;
  this->Texture = NULL;
  //
  this->PositionCoordinate = vtkCoordinate::New();
  this->PositionCoordinate->SetCoordinateSystem(VTK_VIEWPORT);
  //
  this->Position2Coordinate = vtkCoordinate::New();
  this->Position2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Position2Coordinate->SetValue(0.5, 0.5);
  this->Position2Coordinate->SetReferenceCoordinate(this->PositionCoordinate);
}

// Destroy an actor2D.
vtkActor2D::~vtkActor2D()
{
  if (this->Property)
    {
    this->Property->UnRegister(this);
    this->Property = NULL;
    }
  if (this->PositionCoordinate)
    {
    this->PositionCoordinate->Delete();
    this->PositionCoordinate = NULL;
    }
  if (this->Position2Coordinate)
    {
    this->Position2Coordinate->Delete();
    this->Position2Coordinate = NULL;
    }
  if (this->Mapper != NULL)
    {
    this->Mapper->UnRegister(this);
    this->Mapper = NULL;
    }
  this->SetTexture(NULL);
}

void vtkActor2D::ReleaseGraphicsResources(vtkWindow *win)
{
  // pass this information onto the mapper
  if (this->Mapper)
    {
    this->Mapper->ReleaseGraphicsResources(win);
    }
}

// Renders an actor2D's property and then it's mapper.
int vtkActor2D::RenderOverlay(vtkViewport* viewport)
{
  vtkDebugMacro(<< "vtkActor2D::RenderOverlay");

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
    return 0;
    }

  // render the texture 
  if (this->Texture)
    {
    vtkRenderer* ren = vtkRenderer::SafeDownCast(viewport);
    if (ren)
      {
      this->Texture->Render(ren);
      }
    }
    
  this->Mapper->RenderOverlay(viewport, this); 

  return 1;
}

// Renders an actor2D's property and then it's mapper.
int vtkActor2D::RenderOpaqueGeometry(vtkViewport* viewport)
{
  vtkDebugMacro(<< "vtkActor2D::RenderOpaqueGeometry");

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
    return 0;
    }

  this->Mapper->RenderOpaqueGeometry(viewport, this);

  return 1;
}

// Renders an actor2D's property and then it's mapper.
int vtkActor2D::RenderTranslucentGeometry(vtkViewport* viewport)
{
  vtkDebugMacro(<< "vtkActor2D::RenderTranslucentGeometry");

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
    return 0;
    }

  this->Mapper->RenderTranslucentGeometry(viewport, this);

  return 1;
}

void vtkActor2D::SetMapper(vtkMapper2D *mapper)
{
  if (this->Mapper != mapper)
    {
    if (this->Mapper != NULL) 
      {
      this->Mapper->UnRegister(this);
      }
    this->Mapper = mapper;
    if (this->Mapper != NULL) 
      {
      this->Mapper->Register(this);
      }
    this->Modified();
    }
}


unsigned long int vtkActor2D::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long time;

  time  = this->PositionCoordinate->GetMTime();
  mTime = ( time > mTime ? time : mTime );
  time  = this->Position2Coordinate->GetMTime();
  mTime = ( time > mTime ? time : mTime );

  if ( this->Property != NULL )
    {
    time = this->Property->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

// Set the Prop2D's position in display coordinates.
void vtkActor2D::SetDisplayPosition(int XPos, int YPos)
{
  this->PositionCoordinate->SetCoordinateSystem(VTK_DISPLAY);
  this->PositionCoordinate->SetValue((float)XPos,(float)YPos,0.0);
}

void vtkActor2D::SetWidth(double w)
{
  double *pos;

  pos = this->Position2Coordinate->GetValue();
  this->Position2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Position2Coordinate->SetValue(w,pos[1]);
}

void vtkActor2D::SetHeight(double w)
{
  double *pos;

  pos = this->Position2Coordinate->GetValue();
  this->Position2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Position2Coordinate->SetValue(pos[0],w);
}
    
double vtkActor2D::GetWidth()
{
  return this->Position2Coordinate->GetValue()[0];
}

double vtkActor2D::GetHeight()
{
  return this->Position2Coordinate->GetValue()[1];
}

// Returns an Prop2D's property2D.  Creates a property if one
// doesn't already exist.
vtkProperty2D *vtkActor2D::GetProperty()
{
  if (this->Property == NULL)
    {
    this->Property = vtkProperty2D::New();
    this->Property->Register(this);
    this->Property->Delete();
    this->Modified();
    }
  return this->Property;
}

void vtkActor2D::GetActors2D(vtkPropCollection *ac)
{
  ac->AddItem(this);
}

void vtkActor2D::ShallowCopy(vtkProp *prop)
{
  vtkActor2D *a = vtkActor2D::SafeDownCast(prop);
  if ( a != NULL )
    {
    this->SetMapper(a->GetMapper());
    this->SetLayerNumber(a->GetLayerNumber());
    this->SetProperty(a->GetProperty());
    this->SetPosition(a->GetPosition());
    this->SetPosition2(a->GetPosition2());
    this->SetTexture(a->GetTexture());
    }

  // Now do superclass
  this->vtkProp::ShallowCopy(prop);
}

void vtkActor2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Layer Number: " << this->LayerNumber << "\n";
  os << indent << "PositionCoordinate: " << this->PositionCoordinate << "\n";
  this->PositionCoordinate->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Position2 Coordinate: " << this->Position2Coordinate << "\n";
  this->Position2Coordinate->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Property: " << this->Property << "\n";
  if (this->Property)
    {
    this->Property->PrintSelf(os, indent.GetNextIndent());
    }
  os << indent << "Mapper: " << this->Mapper << "\n";
  if (this->Mapper)
    {
    this->Mapper->PrintSelf(os, indent.GetNextIndent());
    }
  
  os << indent << "Texture: " << this->Texture << "\n";
  if (this->Texture)
    {
    this->Texture->PrintSelf(os, indent.GetNextIndent());
    }
  
  
}

