/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBalloonRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBalloonRepresentation.h"
#include "vtkRenderer.h"
#include "vtkTextProperty.h"
#include "vtkTextProperty.h"
#include "vtkProperty2D.h"
#include "vtkTextMapper.h"
#include "vtkTextActor.h"
#include "vtkPoints.h"
#include "vtkCellArray.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkActor2D.h"
#include "vtkImageData.h"
#include "vtkTexture.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkActor2D.h"
#include "vtkFloatArray.h"
#include "vtkPointData.h"
#include "vtkWindow.h"
#include "vtkObjectFactory.h"
#include "vtkInteractorObserver.h"

vtkStandardNewMacro(vtkBalloonRepresentation);

vtkCxxSetObjectMacro(vtkBalloonRepresentation, TextProperty, vtkTextProperty);
vtkCxxSetObjectMacro(vtkBalloonRepresentation, FrameProperty, vtkProperty2D);
vtkCxxSetObjectMacro(vtkBalloonRepresentation, BalloonImage, vtkImageData);
vtkCxxSetObjectMacro(vtkBalloonRepresentation, ImageProperty, vtkProperty2D);


//----------------------------------------------------------------------
vtkBalloonRepresentation::vtkBalloonRepresentation()
{
  // Initially we are not visible
  this->Visibility = 0;
  this->TextVisible = 0;
  this->ImageVisible = 0;

  // Balloon related
  this->BalloonText = NULL;
  this->BalloonImage = NULL;
  this->BalloonLayout = ImageTop;

  // Displaying the image in the balloon using texture. Create a quad polygon
  // and apply the texture on top of it.
  this->ImageSize[0] = 50;
  this->ImageSize[1] = 50;
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
  this->TextureActor = vtkActor2D::New();
  this->TextureActor->SetMapper(this->TextureMapper);
  this->ImageProperty = vtkProperty2D::New();
  this->ImageProperty->SetOpacity(1.0);
  this->TextureActor->SetProperty(this->ImageProperty);

  // Controlling layout
  this->Padding = 5;
  this->Offset[0] =  15;
  this->Offset[1] = -30;

  // The text actor
  this->TextMapper = vtkTextMapper::New();
  this->TextActor = vtkActor2D::New();
  this->TextActor->SetMapper(this->TextMapper);
  this->TextProperty = vtkTextProperty::New();
  this->TextProperty->SetColor(0,0,0);
  this->TextProperty->SetFontSize(14);
  this->TextProperty->BoldOn();
  this->TextMapper->SetTextProperty(this->TextProperty);

  // The frame
  this->FramePoints = vtkPoints::New();
  this->FramePoints->SetNumberOfPoints(4);
  this->FramePolygon = vtkCellArray::New();
  this->FramePolygon->Allocate(this->FramePolygon->EstimateSize(1,5));
  this->FramePolygon->InsertNextCell(4);
  this->FramePolygon->InsertCellPoint(0);
  this->FramePolygon->InsertCellPoint(1);
  this->FramePolygon->InsertCellPoint(2);
  this->FramePolygon->InsertCellPoint(3);
  this->FramePolyData = vtkPolyData::New();
  this->FramePolyData->SetPoints(this->FramePoints);
  this->FramePolyData->SetPolys(this->FramePolygon);
  this->FrameMapper = vtkPolyDataMapper2D::New();
  this->FrameMapper->SetInputData(this->FramePolyData);
  this->FrameActor = vtkActor2D::New();
  this->FrameActor->SetMapper(this->FrameMapper);
  this->FrameProperty = vtkProperty2D::New();
  this->FrameProperty->SetColor(1,1,.882);
  this->FrameProperty->SetOpacity(0.5);
  this->FrameActor->SetProperty(this->FrameProperty);
}

//----------------------------------------------------------------------
vtkBalloonRepresentation::~vtkBalloonRepresentation()
{
  if ( this->BalloonText )
    {
    delete [] this->BalloonText;
    }
  if ( this->BalloonImage )
    {
    this->BalloonImage->Delete();
    }

  this->Texture->Delete();
  this->TexturePolyData->Delete();
  this->TexturePoints->Delete();
  this->TextureMapper->Delete();
  this->TextureActor->Delete();
  this->ImageProperty->Delete();

  this->TextMapper->Delete();
  this->TextActor->Delete();
  this->TextProperty->Delete();

  // The frame
  this->FramePoints->Delete();
  this->FramePolygon->Delete();
  this->FramePolyData->Delete();
  this->FrameMapper->Delete();
  this->FrameActor->Delete();
  this->FrameProperty->Delete();
}

//----------------------------------------------------------------------
void vtkBalloonRepresentation::StartWidgetInteraction(double e[2])
{
  this->StartEventPosition[0] = e[0];
  this->StartEventPosition[1] = e[1];
  this->VisibilityOn();
}


//----------------------------------------------------------------------
void vtkBalloonRepresentation::EndWidgetInteraction(double vtkNotUsed(e)[2])
{
  this->VisibilityOff();
}


//----------------------------------------------------------------------
inline void vtkBalloonRepresentation::AdjustImageSize(double imageSize[2])
{
  double r0 = this->ImageSize[0]/imageSize[0];
  double r1 = this->ImageSize[1]/imageSize[1];
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
}


//----------------------------------------------------------------------
inline void vtkBalloonRepresentation::ScaleImage(double imageSize[2],double scale)
{
  imageSize[0] *= scale;
  imageSize[1] *= scale;
}


//----------------------------------------------------------------------
void vtkBalloonRepresentation::BuildRepresentation()
{
  if ( this->GetMTime() > this->BuildTime ||
       (this->Renderer && this->Renderer->GetVTKWindow() &&
        this->Renderer->GetVTKWindow()->GetMTime() > this->BuildTime) )
    {
    this->TextVisible = 0;
    this->ImageVisible = 0;

    int size[2];
    size[0] = (this->Renderer->GetSize())[0];
    size[1] = (this->Renderer->GetSize())[1];
    int stringSize[2]; stringSize[0] = stringSize[1] = 0;
    double imageSize[2]; imageSize[0] = imageSize[1] = 0.0;
    double frameSize[2]; frameSize[0] = frameSize[1] = 0.0;
    double io[2], so[2], fo[2];
    io[0] = 0.0;
    io[1] = 0.0;
    so[0] = 0.0;
    so[1] = 0.0;
    fo[0] = 0.0;
    fo[1] = 0.0;
    double e[2];
    e[0] = static_cast<double>(this->StartEventPosition[0] + this->Offset[0]);
    e[1] = static_cast<double>(this->StartEventPosition[1] + this->Offset[1]);

    // Determine the size of the text
    if ( this->BalloonText )
      {
      // Start by getting the size of the text
      this->TextMapper->SetInput(this->BalloonText);
      this->TextMapper->GetSize(this->Renderer, stringSize);
      this->TextVisible = ( (stringSize[0] > 0 && stringSize[1] > 0) ? 1 : 0 );
      }

    // Determine the size of the image
    if ( this->BalloonImage )
      {
      //this->BalloonImage->Update();
      if ( this->BalloonImage->GetDataDimension() == 2 )
        {
        int dims[3];
        this->BalloonImage->GetDimensions(dims);
        imageSize[0] = static_cast<double>(dims[0]);
        imageSize[1] = static_cast<double>(dims[1]);
        this->ImageVisible = ( (imageSize[0] > 0.0 && imageSize[1] > 0.0) ? 1 : 0 );
        }
      }

    // Layout the text and image
    if ( this->TextVisible || this->ImageVisible )
      {
      if ( this->TextVisible && !this->ImageVisible ) //just text
        {
        frameSize[0] = static_cast<double>(stringSize[0] + 2*this->Padding);
        frameSize[1] = static_cast<double>(stringSize[1] + 2*this->Padding);
        fo[0] = 0.0;
        fo[1] = 0.0;
        so[0] = static_cast<double>(this->Padding);
        so[1] = static_cast<double>(this->Padding);
        }
      else if ( this->ImageVisible && !this->TextVisible ) //just image
        {
        this->AdjustImageSize(imageSize);
        io[0] = 0.0;
        io[1] = 0.0;
        }

      else //both image and text
        {
        this->AdjustImageSize(imageSize);
        if ( this->BalloonLayout == ImageTop )
          {
          frameSize[1] = stringSize[1] + 2*this->Padding;
          double length = (imageSize[0] > (stringSize[0]+2*this->Padding) ?
                           imageSize[0] : (stringSize[0]+2*this->Padding));
          frameSize[0] = length;
          double scale = length/imageSize[0];
          this->ScaleImage(imageSize,scale);
          io[0] = 0.0;
          io[1] = frameSize[1];
          fo[0] = 0.0;
          fo[1] = 0.0;
          so[0] = length/2.0 - stringSize[0]/2.0;
          so[1] = this->Padding;
          }
        else if ( this->BalloonLayout == ImageBottom )
          {
          frameSize[1] = stringSize[1] + 2*this->Padding;
          double length = (imageSize[0] > (stringSize[0]+2*this->Padding) ?
                           imageSize[0] : (stringSize[0]+2*this->Padding));
          frameSize[0] = length;
          double scale = length/imageSize[0];
          this->ScaleImage(imageSize,scale);
          io[0] = 0.0;
          io[1] = 0.0;
          fo[0] = 0.0;
          fo[1] = imageSize[1];
          so[0] = length/2.0 - stringSize[0]/2.0;
          so[1] = imageSize[1] + this->Padding;
          }
        else if ( this->BalloonLayout == ImageLeft )
          {
          frameSize[0] = stringSize[0] + 2*this->Padding;
          double length = (imageSize[1] > (stringSize[1]+2*this->Padding) ?
                           imageSize[1] : (stringSize[1]+2*this->Padding));
          frameSize[1] = length;
          double scale = length/imageSize[1];
          this->ScaleImage(imageSize,scale);
          io[0] = 0.0;
          io[1] = 0.0;
          fo[0] = imageSize[0];
          fo[1] = 0.0;
          so[0] = imageSize[0] + this->Padding;
          so[1] = length/2.0 - stringSize[1]/2.0;
          }
        else if ( this->BalloonLayout == ImageRight )
          {
          frameSize[0] = stringSize[0] + 2*this->Padding;
          double length = (imageSize[1] > (stringSize[1]+2*this->Padding) ?
                           imageSize[1] : (stringSize[1]+2*this->Padding));
          frameSize[1] = length;
          double scale = length/imageSize[1];
          this->ScaleImage(imageSize,scale);
          io[0] = frameSize[0];
          io[1] = 0.0;
          fo[0] = 0.0;
          fo[1] = 0.0;
          so[0] = this->Padding;
          so[1] = length/2.0 - stringSize[1]/2.0;
          }
        }

      // Reposition the origin of the balloon if it's off the renderer
      if ( e[0] < 0 )
        {
        e[0] = 0.0;
        }
      if ( e[1] < 0 )
        {
        e[1] = 0.0;
        }
      if ( (e[0]+frameSize[0]+imageSize[0]) > size[0] )
        {
        e[0] = size[0] - (frameSize[0]+imageSize[0]);
        }
      if ( (e[1]+frameSize[1]+imageSize[1]) > size[1] )
        {
        e[1] = size[1] - (frameSize[1]+imageSize[1]);
        }

      // Draw the text if visible
      if ( this->TextVisible )
        {
        this->FramePoints->SetPoint(0, e[0]+fo[0],e[1]+fo[1],0.0);
        this->FramePoints->SetPoint(1, e[0]+fo[0]+frameSize[0],e[1]+fo[1],0.0);
        this->FramePoints->SetPoint(2, e[0]+fo[0]+frameSize[0],e[1]+fo[1]+frameSize[1],0.0);
        this->FramePoints->SetPoint(3, e[0]+fo[0],e[1]+fo[1]+frameSize[1],0.0);

        this->TextActor->SetPosition(e[0]+so[0], e[1]+so[1]);
        }

      // Place the texture
      if ( this->ImageVisible )
        {
        this->Texture->SetInputData(this->BalloonImage);
        this->TexturePoints->SetPoint(0, e[0]+io[0],e[1]+io[1],0.0);
        this->TexturePoints->SetPoint(1, e[0]+io[0]+imageSize[0],e[1]+io[1],0.0);
        this->TexturePoints->SetPoint(2, e[0]+io[0]+imageSize[0],e[1]+io[1]+imageSize[1],0.0);
        this->TexturePoints->SetPoint(3, e[0]+io[0],e[1]+io[1]+imageSize[1],0.0);
        }
      }//if something visible

    // Update the properties
    this->TextureActor->SetProperty(this->ImageProperty);
    this->FrameActor->SetProperty(this->FrameProperty);
    this->TextMapper->SetTextProperty(this->TextProperty);

    this->BuildTime.Modified();
    }
}

//----------------------------------------------------------------------
void vtkBalloonRepresentation::ReleaseGraphicsResources(vtkWindow *w)
{
  this->Texture->ReleaseGraphicsResources(w);
  this->TextActor->ReleaseGraphicsResources(w);
  this->FrameActor->ReleaseGraphicsResources(w);
  this->TextureActor->ReleaseGraphicsResources(w);
}


//----------------------------------------------------------------------
int vtkBalloonRepresentation::RenderOverlay(vtkViewport *v)
{
  int count = 0;
  this->BuildRepresentation();

  if ( this->TextVisible )
    {
    count += this->FrameActor->RenderOverlay(v);
    count += this->TextActor->RenderOverlay(v);
    }

  if ( this->ImageVisible )
    {
    vtkRenderer* ren = vtkRenderer::SafeDownCast(v);
    if (ren)
      {
      this->Texture->Render(ren);
      count += this->TextureActor->RenderOverlay(v);
      }
    }

  return count;
}

//----------------------------------------------------------------------
int vtkBalloonRepresentation::
ComputeInteractionState(int X, int Y, int)
{
  // Is it in the text region or the image region?
  double x0[3], x2[3];
  if ( this->ImageVisible )
    {
    this->TexturePoints->GetPoint(0,x0);
    this->TexturePoints->GetPoint(2,x2);
    if ( (x0[0] <= X && X <= x2[0]) && (x0[1] <= Y && Y <= x2[1]) )
      {
      return vtkBalloonRepresentation::OnImage;
      }
    }

  if ( this->TextVisible )
    {
    this->FramePoints->GetPoint(0,x0);
    this->FramePoints->GetPoint(2,x2);
    if ( (x0[0] <= X && X <= x2[0]) && (x0[1] <= Y && Y <= x2[1]) )
      {
      return vtkBalloonRepresentation::OnText;
      }
    }

  return vtkBalloonRepresentation::Outside;
}

//----------------------------------------------------------------------
void vtkBalloonRepresentation::PrintSelf(ostream& os, vtkIndent indent)
{
  //Superclass typedef defined in vtkTypeMacro() found in vtkSetGet.h
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Balloon Text: ";
  if ( this->BalloonText )
    {
    os << this->BalloonText << "\n";
    }
  else
    {
    os << "(none)\n";
    }

  os << indent << "Balloon Image: ";
  if ( this->BalloonImage )
    {
    os << this->BalloonImage << "\n";
    }
  else
    {
    os << "(none)\n";
    }

  os << indent << "Balloon Layout: ";
  switch (this->BalloonLayout)
    {
    case ImageLeft:
      os << "Image Left\n";
      break;
    case ImageRight:
      os << "Image Right\n";
      break;
    case ImageBottom:
      os << "Image Bottom\n";
      break;
    default:
      os << "Image Top\n";
    }

  os << indent << "Image Size: (" << this->ImageSize[0] << ","
     << this->ImageSize[1] << ")\n";
  os << indent << "Padding: " << this->Padding << "\n";
  os << indent << "Offset: (" << this->Offset[0] << ","
     << this->Offset[1] << ")\n";

  if ( this->FrameProperty )
    {
    os << indent << "Frame Property:\n";
    this->FrameProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Frame Property: (none)\n";
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

  if ( this->TextProperty )
    {
    os << indent << "Text Property:\n";
    this->TextProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Text Property: (none)\n";
    }
}

