/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTextActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkTextActor.h"
#include "vtkObjectFactory.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkTextProperty.h"
#include "vtkViewport.h"
#include "vtkWindow.h"
#include "vtkTransform.h"
#include "vtkImageData.h"
#include "vtkFreeTypeUtilities.h"
#include "vtkXMLImageDataWriter.h"
#include "vtkPoints.h"
#include "vtkPointData.h"
#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkFloatArray.h"
#include "vtkTexture.h"
#include "vtkMath.h"

vtkCxxRevisionMacro(vtkTextActor, "1.30");
vtkStandardNewMacro(vtkTextActor);

// ----------------------------------------------------------------------------
vtkTextActor::vtkTextActor()
{
  // To remain compatible with code using vtkActor2D, we must set
  // position coord to Viewport, not Normalized Viewport
  // so...compute equivalent coords for initial position
  this->PositionCoordinate->SetCoordinateSystemToViewport();
  
  this->AdjustedPositionCoordinate = vtkCoordinate::New();
  this->AdjustedPositionCoordinate->SetCoordinateSystemToViewport();

  // This intializes the rectangle structure.
  // It will be used to display the text image as a texture map.
  this->Rectangle = vtkPolyData::New();
  this->RectanglePoints = vtkPoints::New();
  // The actual corner points of the rectangle will be computed later.
  this->Rectangle->SetPoints(this->RectanglePoints);
  vtkCellArray* polys = vtkCellArray::New();
  polys->InsertNextCell(4);
  polys->InsertCellPoint(0);
  polys->InsertCellPoint(1);
  polys->InsertCellPoint(2);
  polys->InsertCellPoint(3);
  this->Rectangle->SetPolys(polys);
  polys->Delete();
  vtkFloatArray* tc = vtkFloatArray::New();
  tc->SetNumberOfComponents(2);
  tc->SetNumberOfTuples(4);
  tc->InsertComponent(0,0, 0.0);  tc->InsertComponent(0,1, 0.0);
  tc->InsertComponent(1,0, 0.0);  tc->InsertComponent(1,1, 1.0);
  tc->InsertComponent(2,0, 1.0);  tc->InsertComponent(2,1, 1.0);
  tc->InsertComponent(3,0, 1.0);  tc->InsertComponent(3,1, 0.0);  
  this->Rectangle->GetPointData()->SetTCoords(tc);
  tc->Delete();  
  
  this->ImageData = vtkImageData::New();
  vtkTexture* texture = vtkTexture::New();
  texture->SetInput(this->ImageData);
  this->SetTexture(texture);
  texture->Delete();


  vtkPolyDataMapper2D *mapper = vtkPolyDataMapper2D::New();
  this->PDMapper = 0;
  this->SetMapper(mapper);
  mapper->Delete();
  // Done already in SetMapper.
  //this->PDMapper->SetInput(this->Rectangle);

  this->TextProperty = vtkTextProperty::New();
  this->Transform = vtkTransform::New();

  this->LastOrigin[0]     = 0;
  this->LastOrigin[1]     = 0;

  this->LastSize[0]       = 0;
  this->LastSize[1]       = 0;

  this->MinimumSize[0]    = 10;
  this->MinimumSize[1]    = 10;

  this->MaximumLineHeight = 1.0;
  this->ScaledText        = 0;
  this->AlignmentPoint    = 0;
  this->Orientation       = 0.0;

  this->FontScaleExponent = 1;
  this->FontScaleTarget   = 10;

  this->Input = 0;
  this->InputRendered = false;

  this->FormerJustification[0] = VTK_TEXT_LEFT;
  this->FormerJustification[1] = VTK_TEXT_BOTTOM;
  this->FormerCoordinateSystem = VTK_VIEWPORT;
  this->FormerLineOffset = 0.0;
  this->FormerOrientation = 0.0;

  this->FreeTypeUtilities = vtkFreeTypeUtilities::GetInstance();
  if (!this->FreeTypeUtilities)
    {
    vtkErrorMacro(<<"Failed getting the FreeType utilities instance");
    }
  this->AlignmentPointSet = false;
  // since we're not using a vtkTextMapper anymore, the code/warning below
  // seems obsolete.
  //
  // IMPORTANT: backward compat: the buildtime is updated here so that the 
  // TextProperty->GetMTime() is lower than BuildTime. In that case, this
  // will prevent the TextProperty to override the mapper's TextProperty
  // when the actor is created after the mapper.
  //this->BuildTime.Modified();
}

// ----------------------------------------------------------------------------
vtkTextActor::~vtkTextActor()
{
  this->ImageData->Delete();
  this->AdjustedPositionCoordinate->Delete();
  this->Transform->Delete();
  this->SetTextProperty(NULL);
  if(this->Input)
    {
    delete [] this->Input;
    }
  this->Rectangle->Delete();
  this->Rectangle = 0;
  this->RectanglePoints->Delete();
  this->RectanglePoints = 0;
  this->SetTexture(0);
}

// ----------------------------------------------------------------------------
void vtkTextActor::SetNonLinearFontScale(double exp, int tgt)
{
  if (this->FontScaleExponent == exp && this->FontScaleTarget == tgt)
    {
    return;
    }
  this->FontScaleExponent = exp;
  this->FontScaleTarget = tgt;
  this->Modified();
}

// ----------------------------------------------------------------------------
void vtkTextActor::SetMapper(vtkPolyDataMapper2D *mapper)
{
  // I will not reference count this because the superclass does.
  this->PDMapper = mapper; // So what is the point of have the ivar PDMapper?
  this->vtkActor2D::SetMapper( mapper );
  
  mapper->SetInput(this->Rectangle);
}

// ----------------------------------------------------------------------------
void vtkTextActor::SetMapper(vtkMapper2D *mapper)
{
  if (mapper->IsA("vtkPolyDataMapper2D"))
    {
    this->SetMapper( (vtkPolyDataMapper2D *)mapper );
    }
  else
    {
    vtkErrorMacro(<<"Must use a vtkPolyDataMapper2D with this class");
    }
  }

// ----------------------------------------------------------------------------
void vtkTextActor::SetInput(const char* str)
{
  if(!str)
    {
      vtkErrorMacro(
        <<"vtkTextActor::SetInput was passed an uninitialized string");
    return;
    }
  if(this->Input)
    {
    if(strcmp(this->Input, str) == 0)
      {
      return;
      }
    delete [] this->Input;
    }
  this->Input = new char[strlen(str)+1];
  strcpy(this->Input, str);
  this->InputRendered = false;
}

// ----------------------------------------------------------------------------
char* vtkTextActor::GetInput()
{
  return this->Input;
}

// ----------------------------------------------------------------------------
void vtkTextActor::SetTextProperty(vtkTextProperty *p)
{
  if ( this->TextProperty == p )
    {
    return;
    }
  if ( this->TextProperty )
    {
    this->TextProperty->UnRegister( this );
    this->TextProperty = NULL; 
    }
  this->TextProperty = p;
  if (this->TextProperty)
    {
    this->TextProperty->Register(this);
    }
  this->Modified();
}

// ----------------------------------------------------------------------------
void vtkTextActor::ShallowCopy(vtkProp *prop)
{
  vtkTextActor *a = vtkTextActor::SafeDownCast(prop);
  if ( a != NULL )
    {
    this->SetPosition2(a->GetPosition2());
    this->SetMinimumSize(a->GetMinimumSize());
    this->SetMaximumLineHeight(a->GetMaximumLineHeight());
    this->SetScaledText(a->GetScaledText());
    this->SetAlignmentPoint(a->GetAlignmentPoint());
    this->SetTextProperty(a->GetTextProperty());
    }
  // Now do superclass (mapper is handled by it as well).
  this->vtkActor2D::ShallowCopy(prop);
}

// ----------------------------------------------------------------------------
// Release any graphics resources that are being consumed by this actor.
// The parameter window could be used to determine which graphic
// resources to release.
void vtkTextActor::ReleaseGraphicsResources(vtkWindow *win)
{
  this->vtkActor2D::ReleaseGraphicsResources(win);
}

// ----------------------------------------------------------------------------
int vtkTextActor::RenderOverlay(vtkViewport *viewport)
{
  // Everything is built in RenderOpaqueGeometry, just have to render
  return this->vtkActor2D::RenderOverlay(viewport);
}

// ----------------------------------------------------------------------------
int vtkTextActor::RenderOpaqueGeometry(vtkViewport *viewport)
{
  //Make sure we have a string to render
  if(!this->Input)
    {
    return 0;
    }

  int *vSize = viewport->GetSize();
  //vSize == (0,0) means that we're not ready to render yet
  if(vSize[0] == 0 && vSize[1] == 0)
    {
    return 0;
    }
  //not sure what vSize == 1 means, but it can cause divide-by-zero errors
  //in some of the coordinate conversion methods used below
  if(vSize[0] == 1 || vSize[1] == 1)
    {
    return 0;
    }

  //if PositionCoordinate has changed use its new value in
  //AdjustedPositionCoordinate.
  if(this->PositionCoordinate->GetMTime() >
     this->AdjustedPositionCoordinate->GetMTime())
    {
    this->AdjustedPositionCoordinate->SetValue(
      this->PositionCoordinate->GetValue());
    //this has the side-effect of causing us to re-calculate any movements made
    //due to justification, alignmentpoint, or lineoffset.
    if(this->AlignmentPoint != 0)
      {
      this->AlignmentPointSet = true;
      }
    this->FormerJustification[0] = VTK_TEXT_LEFT;
    this->FormerJustification[1] = VTK_TEXT_BOTTOM;
    this->FormerLineOffset = 0.0;
    }

  //Check that PositionCoordinate and AdjustedPositionCoordinate are
  //using the same CoordinateSystem
  int positionSystem = this->PositionCoordinate->GetCoordinateSystem();
  int adjustedSystem = this->AdjustedPositionCoordinate->GetCoordinateSystem();
  if(positionSystem != adjustedSystem)
    {
    if(adjustedSystem == this->FormerCoordinateSystem)
      {
      this->AdjustedPositionCoordinate->SetCoordinateSystem(positionSystem);
      this->FormerCoordinateSystem = positionSystem;
      }
    else
      {
      this->PositionCoordinate->SetCoordinateSystem(adjustedSystem);
      this->FormerCoordinateSystem = adjustedSystem;
      }
    //at this point FormerCoordinateSystem is also the current CoordinateSystem
    }

  int size[2];
  int *point1, *point2;
  point1 = this->PositionCoordinate->GetComputedViewportValue(viewport);
  point2 = this->Position2Coordinate->GetComputedViewportValue(viewport);
  size[0] = point2[0] - point1[0];
  size[1] = point2[1] - point1[1];
  double adjustedPos[3];

  //check if we need to adjust our position based on AlignmentPoint
  if (this->AlignmentPointSet)
    {
    switch (this->AlignmentPoint)
      {
      case 0:
        this->TextProperty->SetJustificationToLeft();
        this->TextProperty->SetVerticalJustificationToBottom();
        break;
      case 1:
        this->TextProperty->SetJustificationToCentered();
        this->TextProperty->SetVerticalJustificationToBottom();
        break;
      case 2:
        this->TextProperty->SetJustificationToRight();
        this->TextProperty->SetVerticalJustificationToBottom();
        break;
      case 3:
        this->TextProperty->SetJustificationToLeft();
        this->TextProperty->SetVerticalJustificationToCentered();
        break;
      case 4:
        this->TextProperty->SetJustificationToCentered();
        this->TextProperty->SetVerticalJustificationToCentered();
        break;
      case 5:
        this->TextProperty->SetJustificationToRight();
        this->TextProperty->SetVerticalJustificationToCentered();
        break;
      case 6:
        this->TextProperty->SetJustificationToLeft();
        this->TextProperty->SetVerticalJustificationToTop();
        break;
      case 7:
        this->TextProperty->SetJustificationToCentered();
        this->TextProperty->SetVerticalJustificationToTop();
        break;
      case 8:
        this->TextProperty->SetJustificationToRight();
        this->TextProperty->SetVerticalJustificationToTop();
        break;
      }
    this->AlignmentPointSet = false;
    this->BuildTime.Modified();
    //end of handle AlignmentPoint case
    }
  
  //Scaled text case.  We need to be sure that our text will fit
  //inside the specified boundaries
  if(this->ScaledText)
    {
    // Check to see whether we have to rebuild everything
    int positionsHaveChanged = 0;
    if (viewport->GetMTime() > this->BuildTime ||
        (viewport->GetVTKWindow() &&
         viewport->GetVTKWindow()->GetMTime() > this->BuildTime))
      {
      // if the viewport has changed we may - or may not need
      // to rebuild, it depends on if the projected coords change
      if (this->LastSize[0]   != size[0]   || this->LastSize[1]   != size[1] ||
          this->LastOrigin[0] != point1[0] || this->LastOrigin[1] != point1[1])
        {
        positionsHaveChanged = 1;
        }
      }
    
    // Check to see whether we have to rebuild everything
    if (positionsHaveChanged ||
        this->GetMTime() > this->BuildTime ||
        this->Mapper->GetMTime() > this->BuildTime ||
        this->TextProperty->GetMTime() > this->BuildTime)
      {
      vtkDebugMacro(<<"Rebuilding text");

      this->LastOrigin[0] = point1[0];
      this->LastOrigin[1] = point1[1];

      //  Lets try to minimize the number of times we change the font size.
      //  If the width of the font box has not changed by more than a pixel
      // (numerical issues) do not recompute font size.
      if (this->Mapper->GetMTime() > this->BuildTime ||
          this->TextProperty->GetMTime() > this->BuildTime ||
          this->LastSize[0] < size[0] - 1 || this->LastSize[1] < size[1] - 1 ||
          this->LastSize[0] > size[0] + 1 || this->LastSize[1] > size[1] + 1)
        {
        this->LastSize[0] = size[0];
        this->LastSize[1] = size[1];
      
        // limit by minimum size
        if (this->MinimumSize[0] > size[0])
          {
          size[0] = this->MinimumSize[0];
          }
        if (this->MinimumSize[1] > size[1])
          {
          size[1] = this->MinimumSize[1];
          }    
        int max_height = (int)(this->MaximumLineHeight * (float)size[1]);

        int fsize = this->FreeTypeUtilities->GetConstrainedFontSize(
          this->Input, this->TextProperty, size[0],
          (size[1] < max_height ? size[1] : max_height));
          
        // apply non-linear scaling
        fsize =static_cast<int>(pow(static_cast<double>(fsize),this->FontScaleExponent)*
          pow(this->FontScaleTarget, 1.0 - this->FontScaleExponent));
        // and set the new font size
        this->TextProperty->SetFontSize(fsize);
        }
      }
    }
    
  //check if we need to render the string
  if(this->TextProperty->GetMTime() > this->BuildTime || !this->InputRendered)
    {
    if(!this->FreeTypeUtilities->RenderString(this->TextProperty,
                                              this->Input,
                                              this->ImageData))
      {
      vtkErrorMacro(<<"Failed rendering text to buffer");
      return 0;
      }
    
    this->ComputeRectangle();

    this->Texture->SetInput(this->ImageData);
    this->InputRendered = true;
    this->BuildTime.Modified();
    }

  //handle justification & vertical justification
  if(this->FormerJustification[0] !=
      this->TextProperty->GetJustification() ||
      this->FormerJustification[1] !=
      this->TextProperty->GetVerticalJustification())
    {
    if(this->FormerCoordinateSystem == VTK_USERDEFINED)
      {
      vtkErrorMacro(<<"user defined system, cannot handle justification");
      }
    else
      {
      int textSize[2] = {-1, -1};
      float descender = -1;
      this->FreeTypeUtilities->GetWidthHeightDescender(this->Input,
                                                      this->TextProperty,
                                                      &textSize[0],
                                                      &textSize[1],
                                                      &descender);
      this->AdjustedPositionCoordinate->GetValue(adjustedPos);
      this->SpecifiedToDisplay(
        adjustedPos, viewport, this->FormerCoordinateSystem);
      if(this->FormerOrientation != this->TextProperty->GetOrientation())
        {
        this->Transform->Identity();
        this->Transform->RotateZ(this->TextProperty->GetOrientation());
        this->FormerOrientation = this->TextProperty->GetOrientation();
        }
      double changeVector[3];
      switch(this->TextProperty->GetJustification())
        {
        case VTK_TEXT_RIGHT:
          changeVector[0] = textSize[0];
          changeVector[1] = 0;
          changeVector[2] = 0;
          this->Transform->TransformPoint(changeVector, changeVector);
          changeVector[0] = floor(changeVector[0] + 0.5);
          changeVector[1] = floor(changeVector[1] + 0.5);
          adjustedPos[0] -= changeVector[0];
          adjustedPos[1] -= changeVector[1];
          break;
        case VTK_TEXT_CENTERED:
          changeVector[0] = textSize[0] / 2;
          changeVector[1] =  0;
          changeVector[2] = 0;
          this->Transform->TransformPoint(changeVector, changeVector);
          changeVector[0] = floor(changeVector[0] + 0.5);
          changeVector[1] = floor(changeVector[1] + 0.5);
          adjustedPos[0] -= changeVector[0];
          adjustedPos[1] -= changeVector[1];
          break;
        case VTK_TEXT_LEFT:
          break;
        }
      
      switch(this->TextProperty->GetVerticalJustification())
        {
        case VTK_TEXT_TOP:
          changeVector[0] = 0;
          changeVector[1] = textSize[1] - descender;
          changeVector[2] = 0;
          this->Transform->TransformPoint(changeVector, changeVector);
          changeVector[0] = floor(changeVector[0] + 0.5);
          changeVector[1] = floor(changeVector[1] + 0.5);
          adjustedPos[0] -= changeVector[0];
          adjustedPos[1] -= changeVector[1];
          break;
        case VTK_TEXT_CENTERED:
          changeVector[0] = 0;
          changeVector[1] = textSize[1] / 2 - descender / 2;
          changeVector[2] = 0;
          this->Transform->TransformPoint(changeVector, changeVector);
          changeVector[0] = floor(changeVector[0] + 0.5);
          changeVector[1] = floor(changeVector[1] + 0.5);
          adjustedPos[0] -= changeVector[0];
          adjustedPos[1] -= changeVector[1];
          break;
        case VTK_TEXT_BOTTOM:
          break;
        }
      
      this->DisplayToSpecified(adjustedPos,
                                viewport,
                                this->FormerCoordinateSystem);
      this->AdjustedPositionCoordinate->SetValue(adjustedPos);
      this->FormerJustification[0] =
        this->TextProperty->GetJustification();
      this->FormerJustification[1] =
        this->TextProperty->GetVerticalJustification();
      }
    this->BuildTime.Modified();
    }

  //handle line offset
  if(this->FormerLineOffset != this->TextProperty->GetLineOffset())
    {
    if(this->FormerCoordinateSystem == VTK_USERDEFINED)
      {
      vtkErrorMacro(<<"user defined system, cannot handle lineoffset");
      }
    else
      {
      this->GetActualPositionCoordinate()->GetValue(adjustedPos);
      this->SpecifiedToDisplay(adjustedPos,
                                viewport,
                                this->FormerCoordinateSystem);
      double changeVector[3] = {0, this->TextProperty->GetLineOffset(), 0};
      if(this->FormerOrientation != this->TextProperty->GetOrientation())
        {
        this->Transform->Identity();
        this->Transform->RotateZ(this->TextProperty->GetOrientation());
        this->FormerOrientation = this->TextProperty->GetOrientation();
        }
      this->Transform->TransformPoint(changeVector, changeVector);
      changeVector[0] = floor(changeVector[0] + 0.5);
      changeVector[1] = floor(changeVector[1] + 0.5);

      adjustedPos[0] -= changeVector[0];
      adjustedPos[1] -= changeVector[1];

      this->DisplayToSpecified(adjustedPos,
                                viewport,
                                this->FormerCoordinateSystem);
      this->GetActualPositionCoordinate()->SetValue(adjustedPos);
      this->FormerLineOffset = this->TextProperty->GetLineOffset();
      }
    this->BuildTime.Modified();
    }

  // Everything is built, just have to render
  return this->vtkActor2D::RenderOpaqueGeometry(viewport);
}


// ----------------------------------------------------------------------------
void vtkTextActor::SetOrientation(float orientation) 
{
  if (this->Orientation == orientation)
    {
    return;
    }
  this->Modified();
  this->Orientation = orientation;
  this->ComputeRectangle();
}

// ----------------------------------------------------------------------------
void vtkTextActor::SetAlignmentPoint(int val) 
{
  if (this->AlignmentPoint == val)
    {
    return;
    }
  if (this->AlignmentPoint < 0 || this->AlignmentPoint > 8)
    {
    vtkErrorMacro("Bad alignment code " << val);
    return;
    }
  this->AlignmentPoint = val;
  this->AlignmentPointSet = true;
  this->ComputeRectangle();
  this->Modified();
}
  
// ----------------------------------------------------------------------------
void vtkTextActor::ComputeRectangle() 
{
  int dims[3];
  this->RectanglePoints->Reset();
  if (this->ImageData)
    {
    this->ImageData->GetDimensions(dims);
    }
  else
    {
    dims[0] = dims[1] = 0;
    }
    
  // I could do this with a transform, but it is simple enough
  // to rotate the four corners in 2D ...
  double radians = this->Orientation * vtkMath::DegreesToRadians();
  double c = cos(radians);      
  double s = sin(radians);
  double xo, yo;
  double x, y;
  xo = yo = 0.0;
  switch (this->AlignmentPoint)
    {
    case 0:
      break;
    case 1:
      xo = -(double)(dims[0]) * 0.5;
      break;
    case 2:
      xo = -(double)(dims[0]);
      break;
    case 3:
      yo = -(double)(dims[1]) * 0.5;
      break;
    case 4:
      yo = -(double)(dims[1]) * 0.5;
      xo = -(double)(dims[0]) * 0.5;
      break;
    case 5:
      yo = -(double)(dims[1]) * 0.5;
      xo = -(double)(dims[0]);
      break;
    case 6:
      yo = -(double)(dims[1]);
      break;
    case 7:
      yo = -(double)(dims[1]);
      xo = -(double)(dims[0]) * 0.5;
      break;
    case 8:
      yo = -(double)(dims[1]);
      xo = -(double)(dims[0]);
      break;
    default:
      vtkErrorMacro(<< "Bad alignment point value " << this->AlignmentPoint);
    }
  
  x = xo; y = yo;      
  this->RectanglePoints->InsertNextPoint(c*x-s*y,s*x+c*y,0.0);
  x = xo; y = yo + (double)(dims[1]);      
  this->RectanglePoints->InsertNextPoint(c*x-s*y,s*x+c*y,0.0);
  x = xo + (double)(dims[0]); y = yo + (double)(dims[1]);      
  this->RectanglePoints->InsertNextPoint(c*x-s*y,s*x+c*y,0.0);
  x = xo + (double)(dims[0]); y = yo;      
  this->RectanglePoints->InsertNextPoint(c*x-s*y,s*x+c*y,0.0);
}


// ----------------------------------------------------------------------------
void vtkTextActor::SpecifiedToDisplay(double *pos, vtkViewport *vport,
                                      int specified) 
{
  switch(specified)
  {    
  case VTK_WORLD:
    vport->WorldToView(pos[0], pos[1], pos[2]);
  case VTK_VIEW:
    vport->ViewToNormalizedViewport(pos[0], pos[1], pos[2]);
  case VTK_NORMALIZED_VIEWPORT:
    vport->NormalizedViewportToViewport(pos[0], pos[1]);
  case VTK_VIEWPORT:
    vport->ViewportToNormalizedDisplay(pos[0], pos[1]);
  case VTK_NORMALIZED_DISPLAY:
    vport->NormalizedDisplayToDisplay(pos[0], pos[1]);
  case VTK_DISPLAY:
    break;
  }
}

// ----------------------------------------------------------------------------
void vtkTextActor::DisplayToSpecified(double *pos, vtkViewport *vport,
                                      int specified)
{
  switch(specified)
    {
    case VTK_WORLD:
      vport->DisplayToNormalizedDisplay(pos[0], pos[1]);
      vport->NormalizedDisplayToViewport(pos[0], pos[1]);
      vport->ViewportToNormalizedViewport(pos[0], pos[1]);
      vport->NormalizedViewportToView(pos[0], pos[1], pos[2]);
      vport->ViewToWorld(pos[0], pos[1], pos[2]);
      break;
    case VTK_VIEW:
      vport->DisplayToNormalizedDisplay(pos[0], pos[1]);
      vport->NormalizedDisplayToViewport(pos[0], pos[1]);
      vport->ViewportToNormalizedViewport(pos[0], pos[1]);
      vport->NormalizedViewportToView(pos[0], pos[1], pos[2]);
      break;
    case VTK_NORMALIZED_VIEWPORT:
      vport->DisplayToNormalizedDisplay(pos[0], pos[1]);
      vport->NormalizedDisplayToViewport(pos[0], pos[1]);
      vport->ViewportToNormalizedViewport(pos[0], pos[1]);
      break;
    case VTK_VIEWPORT:
      vport->DisplayToNormalizedDisplay(pos[0], pos[1]);
      vport->NormalizedDisplayToViewport(pos[0], pos[1]);
      break;
    case VTK_NORMALIZED_DISPLAY:
      vport->DisplayToNormalizedDisplay(pos[0], pos[1]);
      break;
    case VTK_DISPLAY:
      break;
    }
}


// ----------------------------------------------------------------------------
void vtkTextActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->TextProperty)
    {
    os << indent << "Text Property:\n";
    this->TextProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Text Property: (none)\n";
    }

  os << indent << "MaximumLineHeight: " << this->MaximumLineHeight << endl;
  os << indent << "MinimumSize: " << this->MinimumSize[0] << " " << this->MinimumSize[1] << endl;
  os << indent << "ScaledText: " << this->ScaledText << endl;
  os << indent << "AlignmentPoint: " << this->AlignmentPoint << endl;
  os << indent << "Orientation: " << this->Orientation << endl;
  os << indent << "FontScaleExponent: " << this->FontScaleExponent << endl;
  os << indent << "FontScaleTarget: " << this->FontScaleTarget << endl;
}
