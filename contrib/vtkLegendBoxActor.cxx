/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLegendBoxActor.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Tim Smith who sponsored and encouraged the development
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
#include "vtkLegendBoxActor.h"
#include "vtkActor.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkPolyDataMapper.h"
#include "vtkFloatArray.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkTransform.h"
#include "vtkObjectFactory.h"

//----------------------------------------------------------------------------
vtkLegendBoxActor* vtkLegendBoxActor::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkLegendBoxActor");
  if(ret)
    {
    return (vtkLegendBoxActor*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkLegendBoxActor;
}

vtkLegendBoxActor::vtkLegendBoxActor()
{
  // Positioning information
  this->PositionCoordinate->SetCoordinateSystemToNormalizedViewport();
  this->PositionCoordinate->SetValue(0.75, 0.75);
  
  this->Position2Coordinate = vtkCoordinate::New();
  this->Position2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Position2Coordinate->SetValue(0.2, 0.2);
  this->Position2Coordinate->SetReferenceCoordinate(this->PositionCoordinate);

  this->LockBorder = 0;
  this->ScalarVisibility = 1;
  
  // Control font properties
  this->Bold = 1;
  this->Italic = 1;
  this->Shadow = 1;
  this->FontFamily = VTK_ARIAL;
  this->Border = 1;
  this->Padding = 3;
  
  // Symbols and text strings
  this->NumberOfEntries = 0;
  this->Symbol = NULL;
  this->Transform = NULL;
  this->SymbolTransform = NULL;
  this->SymbolMapper = NULL;
  this->SymbolActor = NULL;
  this->TextMapper = NULL;
  this->TextActor = NULL;

  // Construct the border
  this->BorderPolyData = vtkPolyData::New();
  vtkPoints *points = vtkPoints::New();
  points->SetNumberOfPoints(4);
  this->BorderPolyData->SetPoints(points); points->Delete();
  vtkCellArray *lines = vtkCellArray::New();
  lines->InsertNextCell(5); //points will be updated later
  lines->InsertCellPoint(0);
  lines->InsertCellPoint(1);
  lines->InsertCellPoint(2);
  lines->InsertCellPoint(3);
  lines->InsertCellPoint(0);
  this->BorderPolyData->SetLines(lines); lines->Delete();

  this->BorderMapper = vtkPolyDataMapper2D::New();
  this->BorderMapper->SetInput(this->BorderPolyData);
  
  this->BorderActor = vtkActor2D::New();
  this->BorderActor->SetMapper(this->BorderMapper);
}

vtkLegendBoxActor::~vtkLegendBoxActor()
{
  this->Position2Coordinate->Delete();
  this->Position2Coordinate = NULL;

  this->InitializeEntries();

  if ( this->BorderActor )
    {
    this->BorderActor->Delete();
    this->BorderMapper->Delete();
    this->BorderPolyData->Delete();
    }
}

void vtkLegendBoxActor::InitializeEntries()
{
  int i;

  if ( this->NumberOfEntries > 0 )
    {
    for (i=0; i<this->NumberOfEntries; i++)
      {
      if ( this->Symbol[i] )
        {
        this->Symbol[i]->Delete();
        this->Transform[i]->Delete();
        this->SymbolTransform[i]->Delete();
        this->SymbolMapper[i]->Delete();
        this->SymbolActor[i]->Delete();
        }
      if ( this->TextMapper[i] )
        {
        this->TextMapper[i]->Delete();
        this->TextActor[i]->Delete();
        }
      }//for all entries
    delete [] this->Symbol; this->Symbol = NULL;
    delete [] this->Transform; this->Transform = NULL;
    delete [] this->SymbolTransform; this->SymbolTransform = NULL;
    delete [] this->SymbolMapper; this->SymbolMapper = NULL;
    delete [] this->SymbolActor; this->SymbolActor = NULL;
    delete [] this->TextMapper; this->TextMapper = NULL;
    delete [] this->TextActor; this->TextActor = NULL;
    }//if entries have been defined
}
  
void vtkLegendBoxActor::SetNumberOfEntries(int num)
{
  if ( num == this->NumberOfEntries )
    {
    return;
    }

  else if ( num < this->NumberOfEntries )
    {
    this->NumberOfEntries = num;
    }
  
  else //allocate space
    {
    //Clear out the old stuff
    this->InitializeEntries();

    //Rebuild the internal actors, etc.
    this->NumberOfEntries = num;
    this->TextMapper = new vtkTextMapper* [num];
    this->TextActor = new vtkActor2D* [num];
    this->Symbol = new vtkPolyData* [num];
    this->Transform = new vtkTransform* [num];
    this->SymbolTransform = new vtkTransformPolyDataFilter* [num];
    this->SymbolMapper = new vtkPolyDataMapper2D* [num];
    this->SymbolActor = new vtkActor2D* [num];
    for (int i=0; i<num; i++)
      {
      this->TextMapper[i] = vtkTextMapper::New();
      this->TextMapper[i]->SetJustificationToLeft();
      this->TextMapper[i]->SetVerticalJustificationToCentered();
      this->TextActor[i] = vtkActor2D::New();
      this->TextActor[i]->SetMapper(this->TextMapper[i]);
      this->Symbol[i] = NULL;
      this->Transform[i] = vtkTransform::New();
      this->SymbolTransform[i] = vtkTransformPolyDataFilter::New();
      this->SymbolTransform[i]->SetTransform(this->Transform[i]);
      this->SymbolMapper[i] = vtkPolyDataMapper2D::New();
      this->SymbolMapper[i]->SetInput(this->SymbolTransform[i]->GetOutput());
      this->SymbolActor[i] = vtkActor2D::New();
      this->SymbolActor[i]->SetMapper(this->SymbolMapper[i]);
      }
    }
  
  this->Modified();
  return;
}

void vtkLegendBoxActor::SetEntry(int i, vtkPolyData *symbol, char* string)
{
  if ( i >= 0 && i < this->NumberOfEntries )
    {
    this->SetEntrySymbol(i,symbol);
    this->SetEntryString(i,string);
    }
  
  return;
}

void vtkLegendBoxActor::SetEntrySymbol(int i, vtkPolyData *symbol)
{
  if ( i >= 0 && i < this->NumberOfEntries )
    {
    if ( this->Symbol[i] == symbol )
      {
      return;
      }
    if ( this->Symbol[i] )
      {
      this->Symbol[i]->Delete();
      }
    this->Symbol[i] = symbol;
    if ( this->Symbol[i] )
      {
      this->Symbol[i]->Register(this);
      }
    this->Modified();
    }
}

void vtkLegendBoxActor::SetEntryString(int i, char* string)
{
  if ( i >= 0 && i < this->NumberOfEntries )
    {
    if ( this->TextMapper[i]->GetInput() && string 
         && (!strcmp(this->TextMapper[i]->GetInput(),string))) 
      {
      return;
      }
    this->TextMapper[i]->SetInput(string);
    this->Modified(); 
    }
}

vtkPolyData *vtkLegendBoxActor::GetEntrySymbol(int i)
{
  if ( i < 0 || i >= this->NumberOfEntries )
    {
    return NULL;
    }
  else
    {
    return this->Symbol[i];
    }
}

char* vtkLegendBoxActor::GetEntryString(int i)
{
  if ( i < 0 || i >= this->NumberOfEntries )
    {
    return NULL;
    }
  else
    {
    return this->TextMapper[i]->GetInput();
    }
}

// Release any graphics resources that are being consumed by this actor.
// The parameter window could be used to determine which graphic
// resources to release.
void vtkLegendBoxActor::ReleaseGraphicsResources(vtkWindow *win)
{
  this->vtkActor2D::ReleaseGraphicsResources(win);
}

void vtkLegendBoxActor::SetWidth(float w)
{
  float *pos;

  pos = this->Position2Coordinate->GetValue();
  this->Position2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Position2Coordinate->SetValue(w,pos[1]);
}

void vtkLegendBoxActor::SetHeight(float w)
{
  float *pos;

  pos = this->Position2Coordinate->GetValue();
  this->Position2Coordinate->SetCoordinateSystemToNormalizedViewport();
  this->Position2Coordinate->SetValue(pos[0],w);
}
    
float vtkLegendBoxActor::GetWidth()
{
  return this->Position2Coordinate->GetValue()[0];
}
float vtkLegendBoxActor::GetHeight()
{
  return this->Position2Coordinate->GetValue()[1];
}

int vtkLegendBoxActor::RenderOverlay(vtkViewport *viewport)
{
  if ( this->NumberOfEntries <= 0 )
    {
    return 0;
    }

  int renderedSomething = 0;
  if ( this->Border )
    {
    renderedSomething += this->BorderActor->RenderOverlay(viewport);
    }

  if ( this->LegendEntriesVisible )
    {
    for (int i=0; i<this->NumberOfEntries; i++)
      {
      if ( this->Symbol[i] )
        {
        this->SymbolMapper[i]->SetScalarVisibility(this->ScalarVisibility);
        this->SymbolActor[i]->SetProperty(this->GetProperty());
        renderedSomething += this->SymbolActor[i]->RenderOpaqueGeometry(viewport);
        }
      this->TextActor[i]->SetProperty(this->GetProperty());
      renderedSomething += this->TextActor[i]->RenderOpaqueGeometry(viewport);
      }
    }
  
  return renderedSomething;
}

int vtkLegendBoxActor::RenderOpaqueGeometry(vtkViewport *viewport)
{
  int i;
  float symbolSize;

  if ( this->NumberOfEntries <= 0 )
    {
    return 0;
    }

  // Check to see whether we have to rebuild everything
  int *vsize = viewport->GetSize();
  if ( this->GetMTime() > this->BuildTime ||
       vsize[0] != this->CachedSize[0] || vsize[1] != this->CachedSize[1] )
    {
    vtkDebugMacro(<<"Rebuilding text");
    this->CachedSize[0] = vsize[0];
    this->CachedSize[1] = vsize[1];

    //Get position information
    int *x1, *x2;
    float p1[3], p2[3];
    x1 = this->PositionCoordinate->GetComputedViewportValue(viewport);
    x2 = this->Position2Coordinate->GetComputedViewportValue(viewport);
    p1[0] = (float)x1[0]; p1[1] = (float)x1[1]; p1[2] = 0.0;
    p2[0] = (float)x2[0]; p2[1] = (float)x2[1]; p2[2] = 0.0;

    //Compute spacing...trying to keep things proportional
    //
    //Find the longest string and symbol width ratio
    int length, maxLength;
    int maxTextMapper;
    char *str;
    int tempi[2], fontSize;
    float sf, twr, swr, *bounds;
    for (swr=0.0, maxLength=i=0; i<this->NumberOfEntries; i++)
      {
      str = this->TextMapper[i]->GetInput();
      if ( str ) //if there is a string
        {
        length = strlen(str);
        if ( length > maxLength )
          {
          maxLength = length;
          maxTextMapper = i;
          }
        }//if string

      if ( this->Symbol[i] ) //if there is a symbol
        {
        this->Symbol[i]->Update();
        bounds = this->Symbol[i]->GetBounds();
        if ( (bounds[3]-bounds[2]) == 0.0 )
          {
          sf = 1.0;
          }
        else
          {
          }
        sf = (bounds[1]-bounds[0]) / (bounds[3]-bounds[2]);
        if ( sf > swr )
          {
          swr = sf;
          }
        }//if symbol defined
      }

    //Compute the final proportion (symbol width to text width)
    fontSize = 12;
	this->TextMapper[maxTextMapper]->SetFontSize(fontSize);
    this->TextMapper[maxTextMapper]->GetSize(viewport,tempi);
    twr = (float)tempi[0]/tempi[1];
    symbolSize = swr / (swr + twr);

    //Okay, now that the proportions are okay, let's size everything
    //First the text
    int size[2];
    size[0] = (int)((1.0-symbolSize)*(p2[0] - p1[0] - 2.0*this->Padding));
    size[1] = (int)((p2[1] - p1[1] - 2.0*this->Padding)/this->NumberOfEntries);

    // while the size is too small increase it
    while ( tempi[0] < size[0] && tempi[1] < size[1] )
      {
      fontSize++;
      this->TextMapper[maxTextMapper]->SetFontSize(fontSize);
      this->TextMapper[maxTextMapper]->GetSize(viewport,tempi);
      }
    // while the size is too large decrease it
    while ((tempi[0] > size[0] || tempi[1] > size[1]) && fontSize > 0)
      {
      fontSize--;
      this->TextMapper[maxTextMapper]->SetFontSize(fontSize);
      this->TextMapper[maxTextMapper]->GetSize(viewport,tempi);
      }

    // don't draw anything if it's too small
    if ( size[1] > 0 && fontSize > 0)
      {
      this->LegendEntriesVisible = 1;
      }
    else
      {
      this->LegendEntriesVisible = 0;
      }
    
    //Border - may adjust spacing based on font size relationship
    //to the proportions relative to the border
    //
    if ( this->Border )
      {
      //adjust the border placement if too much whitespace
      if ( !this->LockBorder && tempi[0] < size[0] )
        {
        p2[0] = p1[0] + 2.0*this->Padding + 
                symbolSize*(p2[0] - p1[0] - 2.0*this->Padding) + tempi[0];
        }
      this->BorderActor->SetProperty(this->GetProperty());
      vtkPoints *pts = this->BorderPolyData->GetPoints();
      pts->SetPoint(0, p1);
      pts->SetPoint(1, p2[0],p1[1],0.0);
      pts->SetPoint(2, p2[0],p2[1],0.0);
      pts->SetPoint(3, p1[0],p2[1],0.0);
      }
    
    //Place text strings
    float posY;
    float posX = p1[0] + this->Padding + 
                 symbolSize*(p2[0] - p1[0] - 2.0*this->Padding);
    for (i=0; i<this->NumberOfEntries; i++)
      {
      posY = p2[1] - this->Padding - (float)i*size[1] - 0.5*size[1];
      this->TextActor[i]->SetPosition(posX,posY);
	  this->TextMapper[i]->SetFontSize(fontSize);
      }
    
    //Place symbols
    //
    //Find the x-y bounds of the symbols...we'll be scaling these as well
    size[0] = (int)(symbolSize*(p2[0] - p1[0] - 2.0*this->Padding));
    posX = p1[0] + this->Padding + 
                 0.5*symbolSize*(p2[0] - p1[0] - 2.0*this->Padding);
    for (i=0; i<this->NumberOfEntries; i++)
      {
      if ( this->Symbol[i] )
        {
        this->SymbolTransform[i]->SetInput(this->Symbol[i]);
        bounds = this->Symbol[i]->GetBounds();
        sf = size[0]/(bounds[1]-bounds[0]);
        if ( (size[1]/(bounds[3]-bounds[2])) < sf )
          {
          sf = size[1]/(bounds[3]-bounds[2]);
          }
        posY = p2[1] - this->Padding - (float)i*size[1] - 0.5*size[1] -
                       0.25*tempi[1];
        this->Transform[i]->Identity();
        this->Transform[i]->Translate(posX, posY, 0.0);
        this->Transform[i]->Scale(0.5*sf, 0.5*sf, 1);
        }//if symbol defined
      }//for all entries

    this->BuildTime.Modified();
    }//rebuild legend box

  //Okay, now we're ready to render something
  //Border
  int renderedSomething = 0;
  if ( this->Border )
    {
    renderedSomething += this->BorderActor->RenderOpaqueGeometry(viewport);
    }

  if ( this->LegendEntriesVisible )
    {
    for (i=0; i<this->NumberOfEntries; i++)
      {
      if ( this->Symbol[i] )
        {
        this->SymbolMapper[i]->SetScalarVisibility(this->ScalarVisibility);
        this->SymbolActor[i]->SetProperty(this->GetProperty());
        renderedSomething += this->SymbolActor[i]->RenderOpaqueGeometry(viewport);
        }
      this->TextActor[i]->SetProperty(this->GetProperty());
      renderedSomething += this->TextActor[i]->RenderOpaqueGeometry(viewport);
      }
    }

  return renderedSomething;
}

void vtkLegendBoxActor::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkActor2D::PrintSelf(os,indent);

  os << indent << "Position2 Coordinate: " 
     << this->Position2Coordinate << "\n";
  this->Position2Coordinate->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Number Of Entries: " << this->NumberOfEntries << "\n";

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

  os << indent << "Scalar Visibility: " 
     << (this->ScalarVisibility ? "On\n" : "Off\n");
  os << indent << "Padding: " << this->Padding << "\n";
  os << indent << "Border: " << (this->Border ? "On\n" : "Off\n");
  os << indent << "LockBorder: " << (this->LockBorder ? "On\n" : "Off\n");
}

vtkCoordinate *vtkLegendBoxActor::GetPosition2Coordinate() 
{ 
  vtkDebugMacro(<< this->GetClassName() << " (" << this 
                << "): returning Position2Coordinate address " 
                << this->Position2Coordinate ); 
  return this->Position2Coordinate; 
} 
void vtkLegendBoxActor::SetPosition2(float x[2]) 
{
  this->SetPosition2(x[0],x[1]);
} 
void vtkLegendBoxActor::SetPosition2(float x, float y) 
{ 
  this->Position2Coordinate->SetCoordinateSystem(VTK_VIEWPORT); 
  this->Position2Coordinate->SetValue(x,y); 
} 
float *vtkLegendBoxActor::GetPosition2() 
{ 
  return this->Position2Coordinate->GetValue(); 
}

void vtkLegendBoxActor::ShallowCopy(vtkProp *prop)
{
  vtkLegendBoxActor *a = vtkLegendBoxActor::SafeDownCast(prop);
  if ( a != NULL )
    {
    this->SetPosition2(a->GetPosition2());
    this->SetBold(a->GetBold());
    this->SetItalic(a->GetItalic());
    this->SetShadow(a->GetShadow());
    this->SetFontFamily(a->GetFontFamily());
    this->SetBorder(a->GetBorder());
    this->SetPadding(a->GetPadding());
    this->SetLockBorder(a->GetLockBorder());
    this->SetLockBorder(a->GetScalarVisibility());
    }

  // Now do superclass
  this->vtkActor2D::ShallowCopy(prop);
}


