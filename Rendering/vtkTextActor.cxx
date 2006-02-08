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
#include "vtkImageMapper.h"
#include "vtkTextProperty.h"
#include "vtkViewport.h"
#include "vtkWindow.h"
#include "vtkTransform.h"
#include "vtkImageData.h"
#include "vtkFreeTypeUtilities.h"

vtkCxxRevisionMacro(vtkTextActor, "1.27");
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

  vtkImageMapper *mapper = vtkImageMapper::New();
  mapper->SetColorWindow(255);
  mapper->SetColorLevel(127);
  this->SetMapper(mapper);
  mapper->Delete();

  this->TextProperty = vtkTextProperty::New();
  this->ImageData = vtkImageData::New();
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

  this->FontScaleExponent = 1;
  this->FontScaleTarget   = 10;

  this->Input = 0;
  this->InputRendered = false;

  this->FormerAlignmentPoint = 0;
  this->FormerJustification[0] = VTK_TEXT_LEFT;
  this->FormerJustification[1] = VTK_TEXT_BOTTOM;
  this->FormerCoordinateSystem = VTK_VIEWPORT;
  this->FormerLineOffset = 0.0;
  this->FormerOrientation = 0.0;

  this->FreeTypeUtilities = vtkFreeTypeUtilities::GetInstance();
  if (!this->FreeTypeUtilities)
    {
    vtkErrorMacro("Failed getting the FreeType utilities instance");
    }
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
void vtkTextActor::SetMapper(vtkImageMapper *mapper)
{
  this->Mapper = mapper;
  this->vtkActor2D::SetMapper( mapper );
}

// ----------------------------------------------------------------------------
void vtkTextActor::SetMapper(vtkMapper2D *mapper)
{
  if (mapper->IsA("vtkImageMapper"))
    {
    this->SetMapper( (vtkImageMapper *)mapper );
    }
  else
    {
    vtkErrorMacro("Must use a vtkImageMapper with this class");
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

  //if PositionCoordinate has changed use its new value in
  //AdjustedPositionCoordinate.
  if(this->PositionCoordinate->GetMTime() >
     this->AdjustedPositionCoordinate->GetMTime())
    {
    this->AdjustedPositionCoordinate->SetValue(
      this->PositionCoordinate->GetValue());
    //this has the side-effect of causing us to re-calculate any movements made
    //due to justification, alignmentpoint, or lineoffset.
    this->FormerAlignmentPoint = 0;
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
  if (!this->ScaledText && this->AlignmentPoint != this->FormerAlignmentPoint)
    {
    // if we're using world or view coordinates we need to keep track of
    // the third term
    if(this->FormerCoordinateSystem == VTK_WORLD)
      {
      this->AdjustedPositionCoordinate->GetValue(adjustedPos);
      viewport->WorldToView(adjustedPos[0], adjustedPos[1], adjustedPos[2]);
      viewport->ViewToNormalizedViewport(
        adjustedPos[0], adjustedPos[1], adjustedPos[2]);
      }
    else if(this->FormerCoordinateSystem == VTK_VIEW)
      {
      this->AdjustedPositionCoordinate->GetValue(adjustedPos);
      viewport->ViewToNormalizedViewport(
        adjustedPos[0], adjustedPos[1], adjustedPos[2]);
      }

    //actually adjust the position based on AlignmentPoint
    switch (this->AlignmentPoint)
      {
      case 0:
        adjustedPos[0] = point1[0];
        adjustedPos[1] = point1[1];
        break;
      case 1:
        adjustedPos[0] = point1[0] + size[0]/2;
        adjustedPos[1] = point1[1];
        break;
      case 2:
        adjustedPos[0] = point2[0];
        adjustedPos[1] = point1[1];
        break;
      case 3:
        adjustedPos[0] = point1[0];
        adjustedPos[1] = point1[1] + size[1]/2;
        break;
      case 4:
        adjustedPos[0] = point1[0] + size[0]/2;
        adjustedPos[1] = point1[1] + size[1]/2;
        break;
      case 5:
        adjustedPos[0] = point2[0];
        adjustedPos[1] = point1[1] + size[1]/2;
        break;
      case 6:
        adjustedPos[0] = point1[0];
        adjustedPos[1] = point2[1];
        break;
      case 7:
        adjustedPos[0] = point1[0] + size[0]/2;
        adjustedPos[1] = point2[1];
        break;
      case 8:
        adjustedPos[0] = point2[0];
        adjustedPos[1] = point2[1];
        break;
      }

    //convert adjustedPos to the coordinate system being used
    //and set it as AdjustedPositionCoordinate's new value
    switch(this->FormerCoordinateSystem)
      {
      case VTK_WORLD:
        viewport->ViewportToNormalizedViewport(
          adjustedPos[0], adjustedPos[1]);
        viewport->NormalizedViewportToView(
          adjustedPos[0], adjustedPos[1], adjustedPos[2]);
        viewport->ViewToWorld(
          adjustedPos[0], adjustedPos[1], adjustedPos[2]);
        break;
      case VTK_VIEW:
        viewport->ViewportToNormalizedViewport(
          adjustedPos[0], adjustedPos[1]);
        viewport->NormalizedViewportToView(
          adjustedPos[0], adjustedPos[1], adjustedPos[2]);
        break;
      case VTK_NORMALIZED_VIEWPORT:;
        viewport->ViewportToNormalizedViewport(
          adjustedPos[0], adjustedPos[1]);
        break;
      case VTK_VIEWPORT:
        break;
      case VTK_NORMALIZED_DISPLAY:
        viewport->ViewportToNormalizedDisplay(
          adjustedPos[0], adjustedPos[1]);
        break;
      case VTK_DISPLAY:
        viewport->ViewportToNormalizedDisplay(
          adjustedPos[0], adjustedPos[1]);
        viewport->NormalizedDisplayToDisplay(
          adjustedPos[0], adjustedPos[1]);
        break;
      }
    this->FormerAlignmentPoint = this->AlignmentPoint;
    this->AdjustedPositionCoordinate->SetValue(adjustedPos);
    this->BuildTime.Modified();
    //end of handle AlignmentPoint case
    }
  
  //Scaled text case.  We need to be sure that our text will fit
  //inside the specified boundaries
  else if(this->ScaledText)
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
    this->Mapper->SetCustomDisplayExtents(this->ImageData->GetWholeExtent());
    this->Mapper->SetInput(this->ImageData);
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
  os << indent << "FontScaleExponent: " << this->FontScaleExponent << endl;
  os << indent << "FontScaleTarget: " << this->FontScaleTarget << endl;
}
