/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkParallelCoordinatesActor.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to Kitware & RPI/SCOREC who supported the development
             of this class.

Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
  this->PositionCoordinate->SetValue(0.25,0.25);
  
  this->Position2Coordinate = vtkCoordinate::New();
  this->Position2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Position2Coordinate->SetValue(0.5, 0.5);
  this->Position2Coordinate->SetReferenceCoordinate(this->PositionCoordinate);
  
  this->Input = NULL;
  this->IndependentVariables = VTK_IV_ROW;
  this->N = 0;
  this->Axes = NULL;

  this->Title = NULL;
  this->TitleMapper = vtkTextMapper::New();
  this->TitleActor = vtkActor2D::New();
  this->TitleActor->SetMapper(this->TitleMapper);
  this->TitleActor->GetPositionCoordinate()->SetCoordinateSystemToViewport();

  int   NumberOfLabels; //along each axis
  int	Bold;
  int   Italic;
  int   Shadow;
  int   FontFamily;
  char  *LabelFormat;

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
  this->Position2Coordinate->Delete();
  this->Position2Coordinate = NULL;
  
  if ( this->Input )
    {
    this->Input->Delete();
    this->Input = NULL;
    }

  if ( this->Axes )
    {
    for (int i=0; i<this->N; i++)
      {
      this->Axes[i]->Delete();
      }
    delete [] this->Axes;
    this->Axes = NULL;
    }
  
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

  for (int i=0; i<this->N; i++)
    {
    renderedSomething += this->Axes[i]->RenderOverlay(viewport);
    }
  
  if ( this->Title != NULL )
    {
    renderedSomething += this->TitleActor->RenderOverlay(viewport);
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
    return 0;
    }

  // Check modified time to see whether we have to rebuild.
  this->Input->Update();
  mtime = this->Input->GetMTime();

  if ( mtime > this->BuildTime || 
       viewport->GetMTime() > this->BuildTime ||
       this->GetMTime() > this->BuildTime )
    {
    int pos[2], pos2[2];
    int *size=viewport->GetSize();
    int stringWidth, stringHeight;

    vtkDebugMacro(<<"Rebuilding plot");

    this->PlaceAxes(viewport, size);
    
    // manage title
    if ( this->Title != NULL )
      {
      this->TitleMapper->SetInput(this->Title);
      this->TitleMapper->SetBold(this->Bold);
      this->TitleMapper->SetItalic(this->Italic);
      this->TitleMapper->SetShadow(this->Shadow);
      this->TitleMapper->SetFontFamily(this->FontFamily);
      vtkAxisActor2D::SetFontSize(viewport, this->TitleMapper, size, 1.0,
                                  stringWidth, stringHeight);
      this->TitleActor->GetPositionCoordinate()->SetValue(
        pos[0]+0.5*(pos2[0]-pos[0])-stringWidth/2.0,pos2[1]-stringHeight/2.0);
      this->TitleActor->SetProperty(this->GetProperty());
      }

    this->BuildTime.Modified();
    }//if need to rebuild the plot
  
  for (int i=0; i<this->N; i++)
    {
    renderedSomething += this->Axes[i]->RenderOverlay(viewport);
    }
  if ( this->Title != NULL )
    {
    renderedSomething += this->TitleActor->RenderOpaqueGeometry(viewport);
    }

  return renderedSomething;
}

void vtkParallelCoordinatesActor::PlaceAxes(vtkViewport *viewport, int *size)
{
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

  os << indent << "Position2 Coordinate: " 
     << this->Position2Coordinate << "\n";
  this->Position2Coordinate->PrintSelf(os, indent.GetNextIndent());
  
}
