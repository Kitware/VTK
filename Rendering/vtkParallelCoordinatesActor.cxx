/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParallelCoordinatesActor.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Kitware & RPI/SCOREC who supported the development
             of this class.

Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkParallelCoordinatesActor.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"


//------------------------------------------------------------------------
vtkParallelCoordinatesActor* vtkParallelCoordinatesActor::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = 
    vtkObjectFactory::CreateInstance("vtkParallelCoordinatesActor");
  if(ret)
    {
    return (vtkParallelCoordinatesActor*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkParallelCoordinatesActor;
}

// Instantiate object
vtkParallelCoordinatesActor::vtkParallelCoordinatesActor()
{
  this->PositionCoordinate->SetCoordinateSystemToNormalizedViewport();
  this->PositionCoordinate->SetValue(0.1,0.1);
  
  this->Position2Coordinate->SetValue(0.9, 0.8);
  
  this->Input = NULL;
  this->IndependentVariables = VTK_IV_COLUMN;
  this->N = 0;
  this->Axes = NULL; 
  this->Mins = NULL;
  this->Maxs = NULL;
  this->Xs = NULL;

  this->Title = NULL;
  this->TitleMapper = vtkTextMapper::New();
  this->TitleMapper->SetJustificationToCentered();
  this->TitleActor = vtkActor2D::New();
  this->TitleActor->SetMapper(this->TitleMapper);
  this->TitleActor->GetPositionCoordinate()->SetCoordinateSystemToViewport();

  this->PlotData = vtkPolyData::New();
  this->PlotMapper = vtkPolyDataMapper2D::New();
  this->PlotMapper->SetInput(this->PlotData);
  this->PlotActor = vtkActor2D::New();
  this->PlotActor->SetMapper(this->PlotMapper);

  this->NumberOfLabels = 2;
  this->Bold = 1;
  this->Italic = 1;
  this->Shadow = 1;
  this->FontFamily = VTK_ARIAL;
  this->LabelFormat = new char[8]; 
  sprintf(this->LabelFormat,"%s","%-#6.3g");
}

vtkParallelCoordinatesActor::~vtkParallelCoordinatesActor()
{
  this->TitleMapper->Delete();
  this->TitleMapper = NULL;
  this->TitleActor->Delete();
  this->TitleActor = NULL;
  
  if ( this->Input )
    {
    this->Input->Delete();
    this->Input = NULL;
    }

  this->Initialize();
  
  this->PlotData->Delete();
  this->PlotMapper->Delete();
  this->PlotActor->Delete();

  if (this->Title)
    {
    delete [] this->Title;
    this->Title = NULL;
    }
  
  if (this->LabelFormat) 
    {
    delete [] this->LabelFormat;
    this->LabelFormat = NULL;
    }
}

// Free-up axes and related stuff
void vtkParallelCoordinatesActor::Initialize()
{
  if ( this->Axes )
    {
    for (int i=0; i<this->N; i++)
      {
      this->Axes[i]->Delete();
      }
    delete [] this->Axes;
    this->Axes = NULL;
    delete [] this->Mins;
    this->Mins = NULL;
    delete [] this->Maxs;
    this->Maxs = NULL;
    delete [] this->Xs;
    this->Xs = NULL;
    }
  this->N = 0;
}

// Plot scalar data for each input dataset.
int vtkParallelCoordinatesActor::RenderOverlay(vtkViewport *viewport)
{
  int renderedSomething=0;

  // Make sure input is up to date.
  if ( this->Input == NULL || this->N <= 0 )
    {
    vtkErrorMacro(<< "Nothing to plot!");
    return 0;
    }

  if ( this->Title != NULL )
    {
    renderedSomething += this->TitleActor->RenderOverlay(viewport);
    }

  this->PlotActor->SetProperty(this->GetProperty());
  renderedSomething += this->PlotActor->RenderOverlay(viewport);

  for (int i=0; i<this->N; i++)
    {
    renderedSomething += this->Axes[i]->RenderOverlay(viewport);
    }
  
  return renderedSomething;
}

int vtkParallelCoordinatesActor::RenderOpaqueGeometry(vtkViewport *viewport)
{
  int renderedSomething=0;
  unsigned long mtime;

  // Initialize
  vtkDebugMacro(<<"Plotting parallel coordinates");

  // Make sure input is up to date, and that the data is the correct shape to
  // plot.
  if ( !this->Input  )
    {
    vtkErrorMacro(<< "Nothing to plot!");
    return renderedSomething;
    }

  // Check modified time to see whether we have to rebuild.
  this->Input->Update();
  mtime = this->Input->GetMTime();

  if ( mtime > this->BuildTime || 
       viewport->GetMTime() > this->BuildTime ||
       this->GetMTime() > this->BuildTime )
    {
    int *size=viewport->GetSize();
    int stringWidth, stringHeight;

    vtkDebugMacro(<<"Rebuilding plot");

    if ( !this->PlaceAxes(viewport, size) )
      {
      return renderedSomething;
      }

    this->TitleMapper->SetInput(this->Title);
    this->TitleMapper->SetBold(this->Bold);
    this->TitleMapper->SetItalic(this->Italic);
    this->TitleMapper->SetShadow(this->Shadow);
    this->TitleMapper->SetFontFamily(this->FontFamily);
    vtkAxisActor2D::SetFontSize(viewport, this->TitleMapper, size, 1.0,
                                stringWidth, stringHeight);
    this->TitleActor->GetPositionCoordinate()->
      SetValue((this->Xs[0]+this->Xs[this->N-1])/2.0,this->YMax+stringHeight/2.0);
    this->TitleActor->SetProperty(this->GetProperty());

    this->BuildTime.Modified();
    }//if need to rebuild the plot

  if ( this->Title != NULL )
    {
    renderedSomething += this->TitleActor->RenderOpaqueGeometry(viewport);
    }

  this->PlotActor->SetProperty(this->GetProperty());
  renderedSomething += this->PlotActor->RenderOpaqueGeometry(viewport);

  for (int i=0; i<this->N; i++)
    {
    renderedSomething += this->Axes[i]->RenderOpaqueGeometry(viewport);
    }

  return renderedSomething;
}


int vtkParallelCoordinatesActor::PlaceAxes(vtkViewport *viewport, int *vtkNotUsed(size))
{
  vtkIdType i, j, id;
  vtkDataObject *input = this->GetInput();
  vtkFieldData *field = input->GetFieldData();
  float v;
  
  this->Initialize();

  if ( ! field )
    {
    return 0;
    }
  
  // Determine the shape of the field
  int numColumns = field->GetNumberOfComponents(); //number of "columns"
  vtkIdType numRows = VTK_LARGE_ID; //figure out number of rows
  vtkIdType numTuples;
  vtkDataArray *array;
  for (i=0; i<field->GetNumberOfArrays(); i++)
    {
    array = field->GetArray(i);
    numTuples = array->GetNumberOfTuples();
    if ( numTuples < numRows )
      {
      numRows = numTuples;
      }
    }

  // Determine the number of independent variables
  if ( this->IndependentVariables == VTK_IV_COLUMN )
    {
    this->N = numColumns;
    }
  else //row
    {
    this->N = numRows;
    }

  if ( this->N <= 0 || this->N >= VTK_LARGE_ID )
    {
    this->N = 0;
    vtkErrorMacro(<<"No field data to plot");
    return 0;
    }
  
  // We need to loop over the field to determine the range of
  // each independent variable.
  this->Mins = new float [this->N];
  this->Maxs = new float [this->N];
  for (i=0; i<this->N; i++)
    {
    this->Mins[i] =  VTK_LARGE_FLOAT;
    this->Maxs[i] = -VTK_LARGE_FLOAT;
    }

  if ( this->IndependentVariables == VTK_IV_COLUMN )
    {
    for (j=0; j<numColumns; j++)
      {
      for (i=0; i<numRows; i++)
        {
        v = field->GetComponent(i,j);
        if ( v < this->Mins[j] )
          {
          this->Mins[j] = v;
          }
        if ( v > this->Maxs[j] )
          {
          this->Maxs[j] = v;
          }
        }
      }
    }
  else //row
    {
    for (j=0; j<numRows; j++)
      {
      for (i=0; i<numColumns; i++)
        {
        v = field->GetComponent(j,i);
        if ( v < this->Mins[j] )
          {
          this->Mins[j] = v;
          }
        if ( v > this->Maxs[j] )
          {
          this->Maxs[j] = v;
          }
        }
      }
    }

  // Allocate space and create axes
  this->Axes = new vtkAxisActor2D* [this->N];
  for (i=0; i<this->N; i++)
    {
    this->Axes[i] = vtkAxisActor2D::New();
    this->Axes[i]->GetPoint1Coordinate()->SetCoordinateSystemToViewport();
    this->Axes[i]->GetPoint2Coordinate()->SetCoordinateSystemToViewport();
    this->Axes[i]->SetRange(this->Mins[i],this->Maxs[i]);
    this->Axes[i]->AdjustLabelsOff();
    this->Axes[i]->SetNumberOfLabels(this->NumberOfLabels);
    this->Axes[i]->SetBold(this->Bold);
    this->Axes[i]->SetItalic(this->Italic);
    this->Axes[i]->SetShadow(this->Shadow);
    this->Axes[i]->SetFontFamily(this->FontFamily);
    this->Axes[i]->SetLabelFormat(this->LabelFormat);
    this->Axes[i]->SetProperty(this->GetProperty());
    }
  this->Xs = new int [this->N];

  // Get the location of the corners of the box
  int *p1 = this->PositionCoordinate->GetComputedViewportValue(viewport);
  int *p2 = this->Position2Coordinate->GetComputedViewportValue(viewport);

  // Specify the positions for the axes
  this->YMin = p1[1];
  this->YMax = p2[1];
  for (i=0; i<this->N; i++)
    {
    this->Xs[i] = (int) (p1[0] + (float)i/((float)this->N) * (p2[0]-p1[0]));
    this->Axes[i]->GetPoint1Coordinate()->SetValue(this->Xs[i], this->YMin);
    this->Axes[i]->GetPoint2Coordinate()->SetValue(this->Xs[i], this->YMax);
    }

  // Now generate the lines to plot
  this->PlotData->Initialize(); //remove old polydata, if any
  vtkPoints *pts = vtkPoints::New();
  pts->Allocate(numRows*numColumns);
  vtkCellArray *lines = vtkCellArray::New();
  vtkScalars *lineColors = vtkScalars::New();
  this->PlotData->SetPoints(pts);
  this->PlotData->SetLines(lines);
//  this->PlotData->GetCellData()->SetScalars(lineColors);
  
  float x[3]; x[2] = 0.0;
  if ( this->IndependentVariables == VTK_IV_COLUMN )
    {
    lines->Allocate(lines->EstimateSize(numRows,numColumns));
    lineColors->Allocate(numRows);
    for (j=0; j<numRows; j++)
      {
      lines->InsertNextCell(numColumns);
      for (i=0; i<numColumns; i++)
        {
        x[0] = this->Xs[i];
        v = field->GetComponent(j,i);
        if ( (this->Maxs[i]-this->Mins[i]) == 0.0 )
          {
          x[1] = 0.5 * (this->YMax - this->YMin);
          }
        else
          {
          x[1] = this->YMin + 
            ((v - this->Mins[i]) / (this->Maxs[i] - this->Mins[i])) *
            (this->YMax - this->YMin);
          }
        id = pts->InsertNextPoint(x);
        lines->InsertCellPoint(id);
        }
      }
    }
  else //row
    {
    lines->Allocate(lines->EstimateSize(numColumns,numRows));
    lineColors->Allocate(numColumns);
    for (j=0; j<numColumns; j++)
      {
      lines->InsertNextCell(numColumns);
      for (i=0; i<numRows; i++)
        {
        x[0] = this->Xs[i];
        v = field->GetComponent(i,j);
        if ( (this->Maxs[i]-this->Mins[i]) == 0.0 )
          {
          x[1] = 0.5 * (this->YMax - this->YMin);
          }
        else
          {
          x[1] = this->YMin + 
            ((v - this->Mins[i]) / (this->Maxs[i] - this->Mins[i])) *
            (this->YMax - this->YMin);
          }
        id = pts->InsertNextPoint(x);
        lines->InsertCellPoint(id);
        }
      }
    }

  pts->Delete();
  lines->Delete();
  lineColors->Delete();

  return 1;
}


// Release any graphics resources that are being consumed by this actor.
// The parameter window could be used to determine which graphic
// resources to release.
void vtkParallelCoordinatesActor::ReleaseGraphicsResources(vtkWindow *win)
{
  this->TitleActor->ReleaseGraphicsResources(win);
  for (int i=0; this->Axes && i<this->N; i++)
    {
    this->Axes[i]->ReleaseGraphicsResources(win);
    }
}

void vtkParallelCoordinatesActor::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkActor2D::PrintSelf(os,indent);

  os << indent << "Input: " << this->Input << "\n";
  os << indent << "Position2 Coordinate: " 
     << this->Position2Coordinate << "\n";
  this->Position2Coordinate->PrintSelf(os, indent.GetNextIndent());
  
  os << indent << "Title: " << (this->Title ? this->Title : "(none)") << "\n";
  os << indent << "Number Of Independent Variables: " << this->N << "\n";
  os << indent << "Independent Variables: ";
  if ( this->IndependentVariables == VTK_IV_COLUMN )
    {
    os << "Columns\n";
    }
  else
    {
    os << "Rows\n";
    }

  os << indent << "Number Of Labels: " << this->NumberOfLabels << "\n";

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
