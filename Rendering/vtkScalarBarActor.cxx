/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalarBarActor.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkScalarBarActor.h"

#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkScalarsToColors.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"
#include "vtkViewport.h"
#include "vtkWindow.h"

vtkCxxRevisionMacro(vtkScalarBarActor, "1.42");
vtkStandardNewMacro(vtkScalarBarActor);

vtkCxxSetObjectMacro(vtkScalarBarActor,LookupTable,vtkScalarsToColors);
vtkCxxSetObjectMacro(vtkScalarBarActor,LabelTextProperty,vtkTextProperty);
vtkCxxSetObjectMacro(vtkScalarBarActor,TitleTextProperty,vtkTextProperty);

//----------------------------------------------------------------------------
// Instantiate object with 64 maximum colors; 5 labels; %%-#6.3g label
// format, no title, and vertical orientation. The initial scalar bar
// size is (0.05 x 0.8) of the viewport size.
vtkScalarBarActor::vtkScalarBarActor()
{
  this->LookupTable = NULL;
  this->Position2Coordinate->SetValue(0.17, 0.8);
  
  this->PositionCoordinate->SetCoordinateSystemToNormalizedViewport();
  this->PositionCoordinate->SetValue(0.82,0.1);
  
  this->MaximumNumberOfColors = 64;
  this->NumberOfLabels = 5;
  this->NumberOfLabelsBuilt = 0;
  this->Orientation = VTK_ORIENT_VERTICAL;
  this->Title = NULL;

  this->LabelTextProperty = vtkTextProperty::New();
  this->LabelTextProperty->SetFontSize(12);
  this->LabelTextProperty->SetBold(1);
  this->LabelTextProperty->SetItalic(1);
  this->LabelTextProperty->SetShadow(1);
  this->LabelTextProperty->SetFontFamilyToArial();

  this->TitleTextProperty = vtkTextProperty::New();
  this->TitleTextProperty->ShallowCopy(this->LabelTextProperty);

  this->LabelFormat = new char[8]; 
  sprintf(this->LabelFormat,"%s","%-#6.3g");

  this->TitleMapper = vtkTextMapper::New();
  this->TitleActor = vtkActor2D::New();
  this->TitleActor->SetMapper(this->TitleMapper);
  this->TitleActor->GetPositionCoordinate()->
    SetReferenceCoordinate(this->PositionCoordinate);
  
  this->TextMappers = NULL;
  this->TextActors = NULL;

  this->ScalarBar = vtkPolyData::New();
  this->ScalarBarMapper = vtkPolyDataMapper2D::New();
  this->ScalarBarMapper->SetInput(this->ScalarBar);
  this->ScalarBarActor = vtkActor2D::New();
  this->ScalarBarActor->SetMapper(this->ScalarBarMapper);
  this->ScalarBarActor->GetPositionCoordinate()->
    SetReferenceCoordinate(this->PositionCoordinate);
  this->LastOrigin[0] = 0;
  this->LastOrigin[1] = 0;
  this->LastSize[0] = 0;
  this->LastSize[1] = 0;
}

//----------------------------------------------------------------------------
// Release any graphics resources that are being consumed by this actor.
// The parameter window could be used to determine which graphic
// resources to release.
void vtkScalarBarActor::ReleaseGraphicsResources(vtkWindow *win)
{
  this->TitleActor->ReleaseGraphicsResources(win);
  if (this->TextMappers != NULL )
    {
    for (int i=0; i < this->NumberOfLabelsBuilt; i++)
      {
      this->TextActors[i]->ReleaseGraphicsResources(win);
      }
    }
  this->ScalarBarActor->ReleaseGraphicsResources(win);
}

//----------------------------------------------------------------------------
vtkScalarBarActor::~vtkScalarBarActor()
{
  if (this->LabelFormat) 
    {
    delete [] this->LabelFormat;
    this->LabelFormat = NULL;
    }

  this->TitleMapper->Delete();
  this->TitleActor->Delete();

  if (this->TextMappers != NULL )
    {
    for (int i=0; i < this->NumberOfLabelsBuilt; i++)
      {
      this->TextMappers[i]->Delete();
      this->TextActors[i]->Delete();
      }
    delete [] this->TextMappers;
    delete [] this->TextActors;
    }

  this->ScalarBar->Delete();
  this->ScalarBarMapper->Delete();
  this->ScalarBarActor->Delete();

  if (this->Title)
    {
    delete [] this->Title;
    this->Title = NULL;
    }
  
  this->SetLookupTable(NULL);
  this->SetLabelTextProperty(NULL);
  this->SetTitleTextProperty(NULL);
}

//----------------------------------------------------------------------------
int vtkScalarBarActor::RenderOverlay(vtkViewport *viewport)
{
  int renderedSomething = 0;
  int i;
  
  // Everything is built, just have to render
  if (this->Title != NULL)
    {
    renderedSomething += this->TitleActor->RenderOverlay(viewport);
    }
  this->ScalarBarActor->RenderOverlay(viewport);
  if( this->TextActors == NULL)
    {
     vtkWarningMacro(<<"Need a mapper to render a scalar bar");
     return renderedSomething;
    }
  
  for (i=0; i<this->NumberOfLabels; i++)
    {
    renderedSomething += this->TextActors[i]->RenderOverlay(viewport);
    }

  renderedSomething = (renderedSomething > 0)?(1):(0);

  return renderedSomething;
}

//----------------------------------------------------------------------------
int vtkScalarBarActor::RenderOpaqueGeometry(vtkViewport *viewport)
{
  int renderedSomething = 0;
  int i;
  int size[2];
  
  if ( ! this->LookupTable )
    {
    vtkWarningMacro(<<"Need a mapper to render a scalar bar");
    return 0;
    }

  // Check to see whether we have to rebuild everything
  if (viewport->GetMTime() > this->BuildTime || 
      (viewport->GetVTKWindow() && 
       viewport->GetVTKWindow()->GetMTime() > this->BuildTime))
    {
    // if the viewport has changed we may - or may not need
    // to rebuild, it depends on if the projected coords chage
    int *barOrigin;
    barOrigin = this->PositionCoordinate->GetComputedViewportValue(viewport);
    size[0] = 
      this->Position2Coordinate->GetComputedViewportValue(viewport)[0] -
      barOrigin[0];
    size[1] = 
      this->Position2Coordinate->GetComputedViewportValue(viewport)[1] -
      barOrigin[1];
    if (this->LastSize[0] != size[0] || 
        this->LastSize[1] != size[1] ||
        this->LastOrigin[0] != barOrigin[0] || 
        this->LastOrigin[1] != barOrigin[1])
      {
      this->Modified();
      }
    }
  
  // Check to see whether we have to rebuild everything
  if (this->GetMTime() > this->BuildTime || 
      this->LookupTable->GetMTime() > this->BuildTime ||
      this->LabelTextProperty->GetMTime() > this->BuildTime ||
      this->TitleTextProperty->GetMTime() > this->BuildTime)
    {
    vtkDebugMacro(<<"Rebuilding subobjects");

    // Delete previously constructed objects
    //
    if (this->TextMappers != NULL )
      {
      for (i=0; i < this->NumberOfLabelsBuilt; i++)
        {
        this->TextMappers[i]->Delete();
        this->TextActors[i]->Delete();
        }
      delete [] this->TextMappers;
      delete [] this->TextActors;
      }

    // Build scalar bar object
    //
    vtkScalarsToColors *lut = this->LookupTable;
    // we hard code how many steps to display
    int numColors = this->MaximumNumberOfColors;
    float *range = lut->GetRange();

    int numPts = 2*(numColors + 1);
    vtkPoints *pts = vtkPoints::New();
    pts->SetNumberOfPoints(numPts);
    vtkCellArray *polys = vtkCellArray::New();
    polys->Allocate(polys->EstimateSize(numColors,4));
    vtkUnsignedCharArray *colors = vtkUnsignedCharArray::New();
    colors->SetNumberOfComponents(3);
    colors->SetNumberOfTuples(numColors);

    this->ScalarBarActor->SetProperty(this->GetProperty());
    this->ScalarBar->Initialize();
    this->ScalarBar->SetPoints(pts);
    this->ScalarBar->SetPolys(polys);
    this->ScalarBar->GetCellData()->SetScalars(colors);
    pts->Delete(); polys->Delete(); colors->Delete();

    // get the viewport size in display coordinates
    int *barOrigin, barWidth, barHeight;
    barOrigin = this->PositionCoordinate->GetComputedViewportValue(viewport);
    size[0] = 
      this->Position2Coordinate->GetComputedViewportValue(viewport)[0] -
      barOrigin[0];
    size[1] = 
      this->Position2Coordinate->GetComputedViewportValue(viewport)[1] -
      barOrigin[1];
    this->LastOrigin[0] = barOrigin[0];
    this->LastOrigin[1] = barOrigin[1];
    this->LastSize[0] = size[0];
    this->LastSize[1] = size[1];
    
    // Update all the composing objects
    
    this->TitleActor->SetProperty(this->GetProperty());
    this->TitleMapper->SetInput(this->Title);
    if (this->TitleTextProperty->GetMTime() > this->BuildTime)
      {
      // Shallow copy here so that the size of the title prop is not affected
      // by the automatic adjustment of its text mapper's size (i.e. its
      // mapper's text property is identical except for the font size
      // which will be modified later). This allows text actors to
      // share the same text property, and in that case specifically allows
      // the title and label text prop to be the same.
      this->TitleMapper->GetTextProperty()->ShallowCopy(this->TitleTextProperty);
      this->TitleMapper->GetTextProperty()->SetJustificationToCentered();
      }
    
    // find the best size for the title font
    int titleSize[2];
    this->SizeTitle(titleSize, size, viewport);
    
    // find the best size for the ticks
    int labelSize[2];
    this->AllocateAndSizeLabels(labelSize, size, viewport,range);
    this->NumberOfLabelsBuilt = this->NumberOfLabels;
    
    // generate points
    float x[3]; x[2] = 0.0;
    float delta;
    if ( this->Orientation == VTK_ORIENT_VERTICAL )
      {
      barWidth = size[0] - 4 - labelSize[0];
      barHeight = (int)(0.86*size[1]);
      delta=(float)barHeight/numColors;
      for (i=0; i<numPts/2; i++)
        {
        x[0] = 0;
        x[1] = i*delta;
        pts->SetPoint(2*i,x);
        x[0] = barWidth;
        pts->SetPoint(2*i+1,x);
        }
      }
    else
      {
      barWidth = size[0];
      barHeight = (int)(0.4*size[1]);
      delta=(float)barWidth/numColors;
      for (i=0; i<numPts/2; i++)
        {
        x[0] = i*delta;
        x[1] = barHeight;
        pts->SetPoint(2*i,x);
        x[1] = 0;
        pts->SetPoint(2*i+1,x);
        }
      }

    //polygons & cell colors
    unsigned char *rgba, *rgb;
    vtkIdType ptIds[4];
    for (i=0; i<numColors; i++)
      {
      ptIds[0] = 2*i;
      ptIds[1] = ptIds[0] + 1;
      ptIds[2] = ptIds[1] + 2;
      ptIds[3] = ptIds[0] + 2;
      polys->InsertNextCell(4,ptIds);

      rgba = lut->MapValue(range[0] + (range[1] - range[0])*
                           ((float)i /(numColors-1.0)));
      rgb = colors->GetPointer(3*i); //write into array directly
      rgb[0] = rgba[0];
      rgb[1] = rgba[1];
      rgb[2] = rgba[2];
      }

    // Now position everything properly
    //
    float val;
    if (this->Orientation == VTK_ORIENT_VERTICAL)
      {
      int sizeTextData[2];
      
      // center the title
      this->TitleActor->SetPosition(size[0]/2, 0.9*size[1]);
      
      for (i=0; i < this->NumberOfLabels; i++)
        {
        val = (float)i/(this->NumberOfLabels-1) *barHeight;
        this->TextMappers[i]->GetSize(viewport,sizeTextData);
        this->TextMappers[i]->GetTextProperty()->SetJustificationToLeft();
        this->TextActors[i]->SetPosition(barWidth+3,
                                         val - sizeTextData[1]/2);
        }
      }
    else
      {
      this->TitleActor->SetPosition(size[0]/2, 
                                    barHeight + labelSize[1] + 0.1*size[1]);
      for (i=0; i < this->NumberOfLabels; i++)
        {
        this->TextMappers[i]->GetTextProperty()->SetJustificationToCentered();
        val = (float)i/(this->NumberOfLabels-1) * barWidth;
        this->TextActors[i]->SetPosition(val, barHeight + 0.05*size[1]);
        }
      }

    this->BuildTime.Modified();
    }

  // Everything is built, just have to render
  if (this->Title != NULL)
    {
    renderedSomething += this->TitleActor->RenderOpaqueGeometry(viewport);
    }
  this->ScalarBarActor->RenderOpaqueGeometry(viewport);
  for (i=0; i<this->NumberOfLabels; i++)
    {
    renderedSomething += this->TextActors[i]->RenderOpaqueGeometry(viewport);
    }

  renderedSomething = (renderedSomething > 0)?(1):(0);

  return renderedSomething;
}

//----------------------------------------------------------------------------
void vtkScalarBarActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if ( this->LookupTable )
    {
    os << indent << "Lookup Table:\n";
    this->LookupTable->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Lookup Table: (none)\n";
    }

  if (this->TitleTextProperty)
    {
    os << indent << "Title Text Property:\n";
    this->TitleTextProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Title Text Property: (none)\n";
    }

  if (this->LabelTextProperty)
    {
    os << indent << "Label Text Property:\n";
    this->LabelTextProperty->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Label Text Property: (none)\n";
    }

  os << indent << "Title: " << (this->Title ? this->Title : "(none)") << "\n";
  os << indent << "Maximum Number Of Colors: " 
     << this->MaximumNumberOfColors << "\n";
  os << indent << "Number Of Labels: " << this->NumberOfLabels << "\n";
  os << indent << "Number Of Labels Built: " << this->NumberOfLabelsBuilt << "\n";

  os << indent << "Orientation: ";
  if ( this->Orientation == VTK_ORIENT_HORIZONTAL )
    {
    os << "Horizontal\n";
    }
  else
    {
    os << "Vertical\n";
    }

  os << indent << "Label Format: " << this->LabelFormat << "\n";
}

//----------------------------------------------------------------------------
void vtkScalarBarActor::ShallowCopy(vtkProp *prop)
{
  vtkScalarBarActor *a = vtkScalarBarActor::SafeDownCast(prop);
  if ( a != NULL )
    {
    this->SetPosition2(a->GetPosition2());
    this->SetLookupTable(a->GetLookupTable());
    this->SetMaximumNumberOfColors(a->GetMaximumNumberOfColors());
    this->SetOrientation(a->GetOrientation());
    this->SetLabelTextProperty(a->GetLabelTextProperty());
    this->SetTitleTextProperty(a->GetTitleTextProperty());
    this->SetLabelFormat(a->GetLabelFormat());
    this->SetTitle(a->GetTitle());
    this->GetPositionCoordinate()->SetCoordinateSystem(
      a->GetPositionCoordinate()->GetCoordinateSystem());    
    this->GetPositionCoordinate()->SetValue(
      a->GetPositionCoordinate()->GetValue());
    this->GetPosition2Coordinate()->SetCoordinateSystem(
      a->GetPosition2Coordinate()->GetCoordinateSystem());    
    this->GetPosition2Coordinate()->SetValue(
      a->GetPosition2Coordinate()->GetValue());
    }

  // Now do superclass
  this->vtkActor2D::ShallowCopy(prop);
}

//----------------------------------------------------------------------------
void vtkScalarBarActor::AllocateAndSizeLabels(int *labelSize, 
                                              int *size,
                                              vtkViewport *viewport,
                                              float *range)
{
  labelSize[0] = labelSize[1] = 0;

  this->TextMappers = new vtkTextMapper * [this->NumberOfLabels];
  this->TextActors = new vtkActor2D * [this->NumberOfLabels];

  char string[512];

  float val;
  int i;
  
  // TODO: this should be optimized, maybe by keeping a list of
  // allocated mappers, in order to avoid creation/destruction of
  // their underlying text properties (i.e. each time a mapper is
  // created, text properties are created and shallow-assigned a font size
  // which value might be "far" from the target font size).
  
  for (i=0; i < this->NumberOfLabels; i++)
    {
    this->TextMappers[i] = vtkTextMapper::New();

    val = range[0] + (float)i/(this->NumberOfLabels-1) * (range[1]-range[0]);
    sprintf(string, this->LabelFormat, val);
    this->TextMappers[i]->SetInput(string);

    // Shallow copy here so that the size of the label prop is not affected
    // by the automatic adjustment of its text mapper's size (i.e. its
    // mapper's text property is identical except for the font size
    // which will be modified later). This allows text actors to
    // share the same text property, and in that case specifically allows
    // the title and label text prop to be the same.
    this->TextMappers[i]->GetTextProperty()->ShallowCopy(
      this->LabelTextProperty);

    this->TextActors[i] = vtkActor2D::New();
    this->TextActors[i]->SetMapper(this->TextMappers[i]);
    this->TextActors[i]->SetProperty(this->GetProperty());
    this->TextActors[i]->GetPositionCoordinate()->
      SetReferenceCoordinate(this->PositionCoordinate);
    }

  if (this->NumberOfLabels)
    {
    int targetWidth, targetHeight;

    if ( this->Orientation == VTK_ORIENT_VERTICAL )
      {
      targetWidth = (int)(0.6*size[0]);
      targetHeight = (int)(0.86*size[1]/this->NumberOfLabels);
      }
    else
      {
      targetWidth = (int)(size[0]*0.8/this->NumberOfLabels);
      targetHeight = (int)(0.25*size[1]);
      }

    vtkTextMapper::SetMultipleConstrainedFontSize(viewport, 
                                                  targetWidth, 
                                                  targetHeight,
                                                  this->TextMappers,
                                                  this->NumberOfLabels,
                                                  labelSize);
    }
}

//----------------------------------------------------------------------------
void vtkScalarBarActor::SizeTitle(int *titleSize, 
                                  int *size, 
                                  vtkViewport *viewport)
{
  titleSize[0] = titleSize[1] = 0;

  if (this->Title == NULL || !strlen(this->Title))
    {
    return;
    }

  int targetWidth, targetHeight;
  
  targetWidth = size[0];
  if ( this->Orientation == VTK_ORIENT_VERTICAL )
    {
    targetHeight = (int)(0.1*size[1]);
    }
  else
    {
    targetHeight = (int)(0.25*size[1]);
    }

  this->TitleMapper->SetConstrainedFontSize(
    viewport, targetWidth, targetHeight);

  this->TitleMapper->GetSize(viewport, titleSize);
}

//----------------------------------------------------------------------------
// Backward compatibility calls

void vtkScalarBarActor::SetFontFamily(int val) 
{ 
  this->LabelTextProperty->SetFontFamily(val); 
  this->TitleTextProperty->SetFontFamily(val); 
}

int vtkScalarBarActor::GetFontFamily()
{ 
  return this->LabelTextProperty->GetFontFamily(); 
}

void vtkScalarBarActor::SetBold(int val)
{ 
  this->LabelTextProperty->SetBold(val); 
  this->TitleTextProperty->SetBold(val); 
}

int vtkScalarBarActor::GetBold()
{ 
  return this->LabelTextProperty->GetBold(); 
}

void vtkScalarBarActor::SetItalic(int val)
{ 
  this->LabelTextProperty->SetItalic(val); 
  this->TitleTextProperty->SetItalic(val); 
}

int vtkScalarBarActor::GetItalic()
{ 
  return this->LabelTextProperty->GetItalic(); 
}

void vtkScalarBarActor::SetShadow(int val)
{ 
  this->LabelTextProperty->SetShadow(val); 
  this->TitleTextProperty->SetShadow(val); 
}

int vtkScalarBarActor::GetShadow()
{ 
  return this->LabelTextProperty->GetShadow(); 
}
