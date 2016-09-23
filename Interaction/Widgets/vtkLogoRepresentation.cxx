/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLogoRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLogoRepresentation.h"
#include "vtkCallbackCommand.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty2D.h"
#include "vtkTexturedActor2D.h"
#include "vtkPolyData.h"
#include "vtkTexture.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkFloatArray.h"
#include "vtkPointData.h"
#include "vtkImageData.h"
#include "vtkPropCollection.h"
#include "vtkRenderer.h"
#include "vtkWindow.h"

vtkStandardNewMacro(vtkLogoRepresentation);

vtkCxxSetObjectMacro(vtkLogoRepresentation, Image, vtkImageData);
vtkCxxSetObjectMacro(vtkLogoRepresentation, ImageProperty, vtkProperty2D);


//-------------------------------------------------------------------------
vtkLogoRepresentation::vtkLogoRepresentation()
{
  // Initialize the data members
  this->Image = NULL;
  this->ImageProperty = vtkProperty2D::New();

  // Setup the pipeline
  this->Texture = vtkTexture::New();
  this->TexturePolyData = vtkPolyData::New();
  this->TexturePoints = vtkPoints::New();
  this->TexturePoints->SetNumberOfPoints(4);
  this->TexturePolyData->SetPoints(this->TexturePoints);
  vtkCellArray* polys = vtkCellArray::New();
  polys->InsertNextCell(4);
  polys->InsertCellPoint(0);
  polys->InsertCellPoint(1);
  polys->InsertCellPoint(2);
  polys->InsertCellPoint(3);
  this->TexturePolyData->SetPolys(polys);
  polys->Delete();
  vtkFloatArray* tc = vtkFloatArray::New();
  tc->SetNumberOfComponents(2);
  tc->SetNumberOfTuples(4);
  tc->InsertComponent(0,0, 0.0);  tc->InsertComponent(0,1, 0.0);
  tc->InsertComponent(1,0, 1.0);  tc->InsertComponent(1,1, 0.0);
  tc->InsertComponent(2,0, 1.0);  tc->InsertComponent(2,1, 1.0);
  tc->InsertComponent(3,0, 0.0);  tc->InsertComponent(3,1, 1.0);
  this->TexturePolyData->GetPointData()->SetTCoords(tc);
  tc->Delete();
  this->TextureMapper = vtkPolyDataMapper2D::New();
  this->TextureMapper->SetInputData(this->TexturePolyData);
  this->TextureActor = vtkTexturedActor2D::New();
  this->TextureActor->SetMapper(this->TextureMapper);
  this->TextureActor->SetTexture(this->Texture);
  this->ImageProperty->SetOpacity(0.25);
  this->TextureActor->SetProperty(this->ImageProperty);

  // Set up parameters from thw superclass
  double size[2];
  this->GetSize(size);
  this->Position2Coordinate->SetValue(0.04*size[0], 0.04*size[1]);
  this->ProportionalResize = 1;
  this->Moving = 1;
  this->SetShowBorder(vtkBorderRepresentation::BORDER_ACTIVE);
  this->PositionCoordinate->SetValue(0.9, 0.025);
  this->Position2Coordinate->SetValue(0.075, 0.075);
}

//-------------------------------------------------------------------------
vtkLogoRepresentation::~vtkLogoRepresentation()
{
  if ( this->Image )
  {
    this->Image->Delete();
  }
  this->ImageProperty->Delete();
  this->Texture->Delete();
  this->TexturePoints->Delete();
  this->TexturePolyData->Delete();
  this->TextureMapper->Delete();
  this->TextureActor->Delete();
}

//----------------------------------------------------------------------
void vtkLogoRepresentation::AdjustImageSize(double o[2],
                                            double borderSize[2],
                                            double imageSize[2])
{
  // Scale the image to fit with in the border.
  // Also update the origin so the image is centered.
  double r0 = borderSize[0]/imageSize[0];
  double r1 = borderSize[1]/imageSize[1];
  if ( r0 > r1 )
  {
    imageSize[0] *= r1;
    imageSize[1] *= r1;
  }
  else
  {
    imageSize[0] *= r0;
    imageSize[1] *= r0;
  }

  if ( imageSize[0] < borderSize[0] )
  {
    o[0] += (borderSize[0]-imageSize[0])/2.0;
  }
  if ( imageSize[1] < borderSize[1] )
  {
    o[1] += (borderSize[1]-imageSize[1])/2.0;
  }
}

//-------------------------------------------------------------------------
void vtkLogoRepresentation::BuildRepresentation()
{
  if ( this->GetMTime() > this->BuildTime ||
       (this->Renderer && this->Renderer->GetVTKWindow() &&
        this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime) )
  {

    // Determine and adjust the size of the image
    if ( this->Image )
    {
      double imageSize[2], borderSize[2], o[2];
      imageSize[0] = 0.0;
      imageSize[1] = 0.0;
      //this->Image->Update();
      if ( this->Image->GetDataDimension() == 2 )
      {
        int dims[3];
        this->Image->GetDimensions(dims);
        imageSize[0] = static_cast<double>(dims[0]);
        imageSize[1] = static_cast<double>(dims[1]);
      }
      int *p1 = this->PositionCoordinate->
        GetComputedDisplayValue(this->Renderer);
      int *p2 = this->Position2Coordinate->
        GetComputedDisplayValue(this->Renderer);
      borderSize[0] = p2[0] - p1[0];
      borderSize[1] = p2[1] - p1[1];
      o[0] = static_cast<double>(p1[0]);
      o[1] = static_cast<double>(p1[1]);

      // this preserves the image aspect ratio. The image is
      // centered around the center of the bordered ragion.
      this->AdjustImageSize(o,borderSize,imageSize);

      // Update the points
      this->Texture->SetInputData(this->Image);
      this->TexturePoints->SetPoint(0, o[0],o[1],0.0);
      this->TexturePoints->SetPoint(1, o[0]+imageSize[0],o[1],0.0);
      this->TexturePoints->SetPoint(2, o[0]+imageSize[0],o[1]+imageSize[1],0.0);
      this->TexturePoints->SetPoint(3, o[0],o[1]+imageSize[1],0.0);
      // For GL backend 2 it is important to modify the point array
      this->TexturePoints->Modified();
    }
  }

  // Note that the transform is updated by the superclass
  this->Superclass::BuildRepresentation();
}

//-------------------------------------------------------------------------
void vtkLogoRepresentation::GetActors2D(vtkPropCollection *pc)
{
  pc->AddItem(this->TextureActor);
  this->Superclass::GetActors2D(pc);
}

//-------------------------------------------------------------------------
void vtkLogoRepresentation::ReleaseGraphicsResources(vtkWindow *w)
{
  this->TextureActor->ReleaseGraphicsResources(w);
  this->Superclass::ReleaseGraphicsResources(w);
}

//-------------------------------------------------------------------------
int vtkLogoRepresentation::RenderOverlay(vtkViewport *v)
{
  int count = 0;
  vtkRenderer* ren = vtkRenderer::SafeDownCast(v);
  if (ren)
  {
    count += this->TextureActor->RenderOverlay(v);
  }
  // Display border on top of logo
  count += this->Superclass::RenderOverlay(v);
  return count;
}

//-------------------------------------------------------------------------
void vtkLogoRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->Image )
  {
    os << indent << "Image:\n";
    this->Image->PrintSelf(os,indent.GetNextIndent());
  }
  else
  {
    os << indent << "Image: (none)\n";
  }

  if ( this->ImageProperty )
  {
    os << indent << "Image Property:\n";
    this->ImageProperty->PrintSelf(os,indent.GetNextIndent());
  }
  else
  {
    os << indent << "Image Property: (none)\n";
  }
}
