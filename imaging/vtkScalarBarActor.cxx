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

// Description:
// Instantiate object with 64 maximum colors; 5 labels; font size 12
// of font Arial (bolding, italic, shadows on); %%-#6.3g label
// format, no title, and vertical orientation. The initial scalar bar
// size is (0.05 x 0.8) of the viewport size.
vtkScalarBarActor::vtkScalarBarActor()
{
  this->LookupTable = NULL;
  this->Width = 0.05;
  this->Height = 0.8;
  this->MaximumNumberOfColors = 64;
  this->NumberOfLabels = 5;
  this->Orientation = VTK_ORIENT_VERTICAL;
  this->Title = NULL;

  this->FontSize = 12;
  this->Bold = 1;
  this->Italic = 1;
  this->Shadow = 1;
  this->FontFamily = VTK_ARIAL;
  this->LabelFormat = new char[8]; 
  sprintf(this->LabelFormat,"%s","%-#6.3g");

  this->TitleMapper = vtkTextMapper::New();
  this->TitleActor = vtkActor2D::New();
  this->TitleActor->SetMapper(this->TitleMapper);
 
  this->TextMappers = NULL;
  this->TextActors = NULL;

  this->ScalarBar = vtkPolyData::New();
  this->ScalarBarMapper = vtkPolyDataMapper2D::New();
  this->ScalarBarMapper->SetInput(this->ScalarBar);
  this->ScalarBarActor = vtkActor2D::New();
  this->ScalarBarActor->SetMapper(this->ScalarBarMapper);
}

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
    for (int i=0; i < this->NumberOfLabels; i++)
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
}

void vtkScalarBarActor::Render(vtkViewport *viewport)
{
  int i;

  if ( ! this->LookupTable )
    {
    vtkWarningMacro(<<"Need a mapper to render a scalar bar");
    return;
    }

  // Check to see whether we have to rebuild everything
  if ( this->GetMTime() > this->BuildTime || 
  viewport->GetMTime() > this->BuildTime ||
  this->LookupTable->GetMTime() > this->BuildTime )
    {
    vtkDebugMacro(<<"Rebuilding subobjects");

    // Delete previously constructed objects
    //
    if (this->TextMappers != NULL )
      {
      for (i=0; i < this->NumberOfLabels; i++)
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
    int *size=viewport->GetSize();
    float *center=viewport->GetCenter(), delta;
    int barOrigin[2], barWidth, barHeight;

    // generate points
    float x[3]; x[2] = 0.0;
    if ( this->Orientation == VTK_ORIENT_VERTICAL )
      {
      barWidth=this->Width*size[0];
      barHeight=this->Height*size[1];
      delta=(float)barHeight/numColors;
      barOrigin[0]=center[0]-(barWidth/2);
      barOrigin[1]=center[1]-(barHeight/2);
      for (i=0; i<numPts/2; i++)
	{
	x[0] = barOrigin[0];
	x[1] = barOrigin[1] + i*delta;
        pts->SetPoint(2*i,x);
	x[0] = barOrigin[0] + barWidth;
        pts->SetPoint(2*i+1,x);
	}
      }
    else
      {
      barWidth=this->Width*size[1];
      barHeight=this->Height*size[0];
      delta=(float)barHeight/numColors;
      barOrigin[0]=center[0]-(barHeight/2);
      barOrigin[1]=center[1]-(barWidth/2);
      for (i=0; i<numPts/2; i++)
	{
	x[0] = barOrigin[0] + i*delta;
	x[1] = barOrigin[1] + barWidth;
        pts->SetPoint(2*i,x);
	x[1] = barOrigin[1];
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

    // Update all the composing objects
    //
    if (this->Title == NULL )
      {
      this->TitleActor->VisibilityOff();
      }
    else
      {
      this->TitleActor->VisibilityOn();
      this->TitleActor->SetProperty(this->GetProperty());
      this->TitleMapper->SetInput(this->Title);
      this->TitleMapper->SetFontSize(this->FontSize);
      this->TitleMapper->SetBold(this->Bold);
      this->TitleMapper->SetItalic(this->Italic);
      this->TitleMapper->SetShadow(this->Shadow);
      this->TitleMapper->SetFontFamily(this->FontFamily);
      }

    this->TextMappers = new vtkTextMapper * [this->NumberOfLabels];
    this->TextActors = new vtkActor2D * [this->NumberOfLabels];
    char string[50];
    float val;
    for (i=0; i < this->NumberOfLabels; i++)
      {
      this->TextMappers[i] = vtkTextMapper::New();
      val = range[0] + (float)i/(this->NumberOfLabels-1) * (range[1]-range[0]);
      sprintf(string, this->LabelFormat, val);
      this->TextMappers[i]->SetInput(string);
      this->TextMappers[i]->SetFontSize(this->FontSize);
      this->TextMappers[i]->SetBold(this->Bold);
      this->TextMappers[i]->SetItalic(this->Italic);
      this->TextMappers[i]->SetShadow(this->Shadow);
      this->TextMappers[i]->SetFontFamily(this->FontFamily);
      this->TextActors[i] = vtkActor2D::New();
      this->TextActors[i]->SetMapper(this->TextMappers[i]);
      this->TextActors[i]->SetProperty(this->GetProperty());
      }

    // Now position everything properly
    //
    float *position=this->GetPosition();
    this->ScalarBarActor->SetPosition(position);

    if (this->Orientation == VTK_ORIENT_VERTICAL)
      {
      this->TitleActor->SetPosition(position[0] + barOrigin[0], 
            position[1] + barOrigin[1] + barHeight + this->FontSize*2);

      for (i=0; i < this->NumberOfLabels; i++)
	{
	val = (float)i/(this->NumberOfLabels-1) * barHeight;
	this->TextActors[i]->SetPosition(position[0] + barOrigin[0] + barWidth+3,
              position[1] + barOrigin[1] + val + this->FontSize/2);
	}
      }
    else
      {
      this->TitleActor->SetPosition(position[0] + center[0], 
            position[1] + barOrigin[1] + barWidth + this->FontSize*2.75);
      for (i=0; i < this->NumberOfLabels; i++)
	{
	val = (float)i/(this->NumberOfLabels-1) * barHeight;
	this->TextActors[i]->SetPosition(position[0] + barOrigin[0] + val,
              position[1] + barWidth + barOrigin[1] + this->FontSize + 3);
	}
      }

    this->BuildTime.Modified();
    }

  // Everything is built, just have to render
  if (this->Title != NULL)
    {
    this->TitleActor->Render(viewport);
    }
  this->ScalarBarActor->Render(viewport);
  for (i=0; i<this->NumberOfLabels; i++)
    {
    this->TextActors[i]->Render(viewport);
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
  os << indent << "Width: " << this->Width << "\n";
  os << indent << "Height: " << this->Height << "\n";
  os << indent << "Maximum Number Of Colors: " 
     << this->MaximumNumberOfColors << "\n";
  os << indent << "Number Of Labels: " << this->NumberOfLabels << "\n";

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

  os << indent << "Font Size: " << this->FontSize << "\n";
  os << indent << "Bold: " << (this->Bold ? "On\n" : "Off\n");
  os << indent << "Italic: " << (this->Italic ? "On\n" : "Off\n");
  os << indent << "Shadow: " << (this->Shadow ? "On\n" : "Off\n");
  os << indent << "Label Format: " << this->LabelFormat << "\n";
}
