/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalarBarActor.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Kitware & RPI/SCOREC who supported the development
             of this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

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
#include "vtkScalarBarActor.h"

// Instantiate object with 64 maximum colors; 5 labels; font size 12
// of font Arial (bolding, italic, shadows on); %%-#6.3g label
// format, no title, and vertical orientation. The initial scalar bar
// size is (0.05 x 0.8) of the viewport size.
vtkScalarBarActor::vtkScalarBarActor()
{
  this->LookupTable = NULL;
  this->Position2Coordinate = vtkCoordinate::New();
  this->Position2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Position2Coordinate->SetValue(0.17, 0.8);
  this->Position2Coordinate->SetReferenceCoordinate(this->PositionCoordinate);
  
  this->PositionCoordinate->SetCoordinateSystemToNormalizedViewport();
  this->PositionCoordinate->SetValue(0.82,0.1);
  
  this->MaximumNumberOfColors = 64;
  this->NumberOfLabels = 5;
  this->NumberOfLabelsBuilt = 0;
  this->Orientation = VTK_ORIENT_VERTICAL;
  this->Title = NULL;

  this->Bold = 1;
  this->Italic = 1;
  this->Shadow = 1;
  this->FontFamily = VTK_ARIAL;
  this->LabelFormat = new char[8]; 
  sprintf(this->LabelFormat,"%s","%-#6.3g");

  this->TitleMapper = vtkTextMapper::New();
  this->TitleMapper->SetJustificationToCentered();
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
}

vtkScalarBarActor::~vtkScalarBarActor()
{
  this->Position2Coordinate->Delete();
  this->Position2Coordinate = NULL;
  
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
}

void vtkScalarBarActor::SetWidth(float w)
{
  float *pos;

  pos = this->Position2Coordinate->GetValue();
  this->Position2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Position2Coordinate->SetValue(w,pos[1]);
}

void vtkScalarBarActor::SetHeight(float w)
{
  float *pos;

  pos = this->Position2Coordinate->GetValue();
  this->Position2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Position2Coordinate->SetValue(pos[0],w);
}
    
float vtkScalarBarActor::GetWidth()
{
  return this->Position2Coordinate->GetValue()[0];
}
float vtkScalarBarActor::GetHeight()
{
  return this->Position2Coordinate->GetValue()[1];
}

void vtkScalarBarActor::RenderOverlay(vtkViewport *viewport)
{
  int i;
  
  // Everything is built, just have to render
  if (this->Title != NULL)
    {
    this->TitleActor->RenderOverlay(viewport);
    }
  this->ScalarBarActor->RenderOverlay(viewport);
  for (i=0; i<this->NumberOfLabels; i++)
    {
    this->TextActors[i]->RenderOverlay(viewport);
    }
}

void vtkScalarBarActor::RenderOpaqueGeometry(vtkViewport *viewport)
{
  int i;
  int size[2];
  int stringHeight, stringWidth;
  int fontSize;
  
  if ( ! this->LookupTable )
    {
    vtkWarningMacro(<<"Need a mapper to render a scalar bar");
    return;
    }

  // Check to see whether we have to rebuild everything
  if ( this->GetMTime() > this->BuildTime || 
       // viewport->GetMTime() > this->BuildTime ||
  this->LookupTable->GetMTime() > this->BuildTime )
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
    vtkLookupTable *lut = this->LookupTable;
    int numLutColors = lut->GetNumberOfColors();
    int numColors = (numLutColors > this->MaximumNumberOfColors ? 
		     this->MaximumNumberOfColors : numLutColors);
    float *range = lut->GetTableRange();

    int numPts = 2*(numColors + 1);
    vtkPoints *pts = vtkPoints::New();
    pts->SetNumberOfPoints(numPts);
    vtkCellArray *polys = vtkCellArray::New();
    polys->Allocate(polys->EstimateSize(numColors,4));
    vtkScalars *colors = vtkScalars::New(VTK_UNSIGNED_CHAR,3);
    colors->SetNumberOfScalars(numColors);
    vtkUnsignedCharArray *colorData = (vtkUnsignedCharArray *)colors->GetData();

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

    // Update all the composing objects
    //
    if (this->Title == NULL )
      {
      this->TitleActor->VisibilityOff();
      }
    this->TitleActor->VisibilityOn();
    this->TitleActor->SetProperty(this->GetProperty());
    this->TitleMapper->SetInput(this->Title);
    this->TitleMapper->SetBold(this->Bold);
    this->TitleMapper->SetItalic(this->Italic);
    this->TitleMapper->SetShadow(this->Shadow);
    this->TitleMapper->SetFontFamily(this->FontFamily);
    
    // find the best size for the font
    int tempi[2];
    int target;
    if ( this->Orientation == VTK_ORIENT_VERTICAL )
      {
      target = size[1]*0.05 + 0.05*size[0];
      }
    else
      {
      target = size[1]*0.07 + size[0]*0.03;
      }
    fontSize = target;
    this->TitleMapper->SetFontSize(fontSize);
    this->TitleMapper->GetSize(viewport,tempi);
    while (tempi[1] < target && fontSize < 100)
      {
      fontSize++;
      this->TitleMapper->SetFontSize(fontSize);
      this->TitleMapper->GetSize(viewport,tempi);
      }
    while ((tempi[1] > target || tempi[0] > size[0])&& fontSize > 0)
      {
      fontSize--;
      this->TitleMapper->SetFontSize(fontSize);
      this->TitleMapper->GetSize(viewport,tempi);
      }
    stringHeight = tempi[1];
      
    this->TextMappers = new vtkTextMapper * [this->NumberOfLabels];
    this->TextActors = new vtkActor2D * [this->NumberOfLabels];
    char string[512];
    float val;
    for (i=0; i < this->NumberOfLabels; i++)
      {
      this->TextMappers[i] = vtkTextMapper::New();
      val = range[0] + (float)i/(this->NumberOfLabels-1) * (range[1]-range[0]);
      sprintf(string, this->LabelFormat, val);
      this->TextMappers[i]->SetInput(string);
      this->TextMappers[i]->SetFontSize(fontSize);
      this->TextMappers[i]->SetBold(this->Bold);
      this->TextMappers[i]->SetItalic(this->Italic);
      this->TextMappers[i]->SetShadow(this->Shadow);
      this->TextMappers[i]->SetFontFamily(this->FontFamily);
      this->TextActors[i] = vtkActor2D::New();
      this->TextActors[i]->SetMapper(this->TextMappers[i]);
      this->TextActors[i]->SetProperty(this->GetProperty());
      this->TextActors[i]->GetPositionCoordinate()->
	SetReferenceCoordinate(this->PositionCoordinate);
      }

    this->NumberOfLabelsBuilt = this->NumberOfLabels;

    // generate points
    float x[3]; x[2] = 0.0;
    float delta;
    if ( this->Orientation == VTK_ORIENT_VERTICAL )
      {
      // need to find maximum width
      stringWidth = 0;
      for (i=0; i < this->NumberOfLabels; i++)
	{
	this->TextMappers[i]->GetSize(viewport,tempi);
	if (stringWidth < tempi[0])
	  {
	  stringWidth = tempi[0];
	  }
	}
      barWidth = size[0] - 4 - stringWidth;
      barHeight = size[1] - stringHeight*2.2;
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
      barHeight = size[1] - stringHeight*2.6;
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
    int ptIds[4];
    for (i=0; i<numColors; i++)
      {
      ptIds[0] = 2*i;
      ptIds[1] = ptIds[0] + 1;
      ptIds[2] = ptIds[1] + 2;
      ptIds[3] = ptIds[0] + 2;
      polys->InsertNextCell(4,ptIds);

      rgba = lut->GetPointer((int)((float)numLutColors*i/numColors));
      rgb = colorData->GetPointer(3*i); //write into array directly
      rgb[0] = rgba[0];
      rgb[1] = rgba[1];
      rgb[2] = rgba[2];
      }

    // Now position everything properly
    //
    if (this->Orientation == VTK_ORIENT_VERTICAL)
      {
      // center the title
      this->TitleActor->SetPosition(size[0]/2, size[1] - stringHeight);
      for (i=0; i < this->NumberOfLabels; i++)
	{
	val = (float)i/(this->NumberOfLabels-1) *barHeight;
	this->TextMappers[i]->SetJustificationToLeft();
	this->TextActors[i]->SetPosition(barWidth+3,val - stringHeight/2);
	}
      }
    else
      {
      this->TitleActor->SetPosition(size[0]/2, size[1] - stringHeight);
      for (i=0; i < this->NumberOfLabels; i++)
	{
	val = (float)i/(this->NumberOfLabels-1) * barWidth;
	this->TextMappers[i]->SetJustificationToCentered();
	this->TextActors[i]->SetPosition(val, barHeight + 0.2*stringHeight);
	}
      }

    this->BuildTime.Modified();
    }

  // Everything is built, just have to render
  if (this->Title != NULL)
    {
    this->TitleActor->RenderOpaqueGeometry(viewport);
    }
  this->ScalarBarActor->RenderOpaqueGeometry(viewport);
  for (i=0; i<this->NumberOfLabels; i++)
    {
    this->TextActors[i]->RenderOpaqueGeometry(viewport);
    }
}

void vtkScalarBarActor::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkActor2D::PrintSelf(os,indent);

  if ( this->LookupTable )
    {
    os << indent << "Lookup Table:\n";
    this->LookupTable->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Lookup Table: (none)\n";
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

  os << indent << "Font Family: ";
  if ( this->FontFamily == VTK_ARIAL )
    {
    os << "Arial\n";
    }
  else if ( this->FontFamily == VTK_COURIER )
    {
    os << "Courier\n";
    }
  else
    {
    os << "Times\n";
    }

  os << indent << "Bold: " << (this->Bold ? "On\n" : "Off\n");
  os << indent << "Italic: " << (this->Italic ? "On\n" : "Off\n");
  os << indent << "Shadow: " << (this->Shadow ? "On\n" : "Off\n");
  os << indent << "Label Format: " << this->LabelFormat << "\n";
}
