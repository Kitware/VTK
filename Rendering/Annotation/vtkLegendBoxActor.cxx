/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLegendBoxActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLegendBoxActor.h"

#include "vtkActor.h"
#include "vtkCellArray.h"
#include "vtkDoubleArray.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPlaneSource.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkProperty2D.h"
#include "vtkProperty.h"
#include "vtkTexturedActor2D.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"
#include "vtkTransform.h"
#include "vtkTransformPolyDataFilter.h"
#include "vtkViewport.h"

vtkStandardNewMacro(vtkLegendBoxActor);

vtkCxxSetObjectMacro(vtkLegendBoxActor,EntryTextProperty,vtkTextProperty);

//----------------------------------------------------------------------------
vtkLegendBoxActor::vtkLegendBoxActor()
{
  // Positioning information
  this->PositionCoordinate->SetCoordinateSystemToNormalizedViewport();
  this->PositionCoordinate->SetValue(0.75, 0.75);

  this->Position2Coordinate->SetValue(0.2, 0.2);

  this->LockBorder = 0;
  this->ScalarVisibility = 1;

  // Control font properties
  this->EntryTextProperty = vtkTextProperty::New();
  this->EntryTextProperty->SetBold(0);
  this->EntryTextProperty->SetItalic(0);
  this->EntryTextProperty->SetShadow(0);
  this->EntryTextProperty->SetFontFamily(VTK_ARIAL);
  this->EntryTextProperty->SetJustification(VTK_TEXT_LEFT);
  this->EntryTextProperty->SetVerticalJustification(VTK_TEXT_CENTERED);

  this->Border = 1;
  this->Box = 0;
  this->Padding = 3;

  // Symbols and text strings
  this->NumberOfEntries = 0;
  this->Size = 0;
  this->Colors = NULL;
  this->Symbol = NULL;
  this->Transform = NULL;
  this->SymbolTransform = NULL;
  this->SymbolMapper = NULL;
  this->SymbolActor = NULL;
  this->TextMapper = NULL;
  this->TextActor = NULL;

  this->Icon = NULL;
  this->IconActor = NULL;
  this->IconMapper = NULL;
  this->IconTransformFilter = NULL;
  this->IconTransform = NULL;
  this->IconImage = NULL;

  // Construct the border
  this->BorderPolyData = vtkPolyData::New();
  vtkPoints *points = vtkPoints::New();
  points->SetNumberOfPoints(4);
  this->BorderPolyData->SetPoints(points);
  points->Delete();
  vtkCellArray *lines = vtkCellArray::New();
  lines->InsertNextCell(5); //points will be updated later
  lines->InsertCellPoint(0);
  lines->InsertCellPoint(1);
  lines->InsertCellPoint(2);
  lines->InsertCellPoint(3);
  lines->InsertCellPoint(0);
  this->BorderPolyData->SetLines(lines);
  lines->Delete();

  this->BorderMapper = vtkPolyDataMapper2D::New();
  this->BorderMapper->SetInputData(this->BorderPolyData);

  this->BorderActor = vtkActor2D::New();
  this->BorderActor->SetMapper(this->BorderMapper);

  // Construct the box
  this->BoxPolyData = vtkPolyData::New();
  this->BoxPolyData->SetPoints(this->BorderPolyData->GetPoints());
  vtkCellArray *polys = vtkCellArray::New();
  polys->InsertNextCell(4);
  polys->InsertCellPoint(0);
  polys->InsertCellPoint(1);
  polys->InsertCellPoint(2);
  polys->InsertCellPoint(3);
  this->BoxPolyData->SetPolys(polys);
  polys->Delete();

  this->BoxMapper = vtkPolyDataMapper2D::New();
  this->BoxMapper->SetInputData(this->BoxPolyData);

  this->BoxActor = vtkActor2D::New();
  this->BoxActor->SetMapper(this->BoxMapper);

  // Background.
  this->UseBackground = 0;
  this->BackgroundOpacity = 1.0;
  this->BackgroundColor[0] =  this->BackgroundColor[1] = this->BackgroundColor[2] = 0.3;
  this->Background = vtkPlaneSource::New();
  this->BackgroundActor = vtkTexturedActor2D::New();
  this->BackgroundMapper = vtkPolyDataMapper2D::New();
  this->BackgroundActor->SetMapper(this->BackgroundMapper);
}

//----------------------------------------------------------------------------
vtkLegendBoxActor::~vtkLegendBoxActor()
{
  this->InitializeEntries();

  if ( this->BorderActor )
  {
    this->BorderActor->Delete();
    this->BorderMapper->Delete();
    this->BorderPolyData->Delete();
  }

  if ( this->BoxActor )
  {
    this->BoxActor->Delete();
    this->BoxMapper->Delete();
    this->BoxPolyData->Delete();
  }

  if ( this->BackgroundActor )
  {
    this->BackgroundActor->Delete();
    this->BackgroundMapper->Delete();
    this->Background->Delete();
  }

  this->SetEntryTextProperty(NULL);
}

//----------------------------------------------------------------------------
void vtkLegendBoxActor::InitializeEntries()
{
  int i;

  if ( this->Size > 0 )
  {
    this->Colors->Delete();
    for (i=0; i<this->Size; i++)
    {
      if ( this->Symbol[i] )
      {
        this->Symbol[i]->Delete();
      }
      this->Transform[i]->Delete();
      this->SymbolTransform[i]->Delete();
      this->SymbolMapper[i]->Delete();
      this->SymbolActor[i]->Delete();
      if ( this->TextMapper[i] )
      {
        this->TextMapper[i]->Delete();
        this->TextActor[i]->Delete();
      }

      if( this->IconImage[i] )
      {
        this->IconImage[i]->Delete();
      }

      this->Icon[i]->Delete();
      this->IconTransform[i]->Delete();
      this->IconTransformFilter[i]->Delete();
      this->IconMapper[i]->Delete();
      this->IconActor[i]->Delete();
    }//for all entries
    delete [] this->Symbol; this->Symbol = NULL;
    delete [] this->Transform; this->Transform = NULL;
    delete [] this->SymbolTransform; this->SymbolTransform = NULL;
    delete [] this->SymbolMapper; this->SymbolMapper = NULL;
    delete [] this->SymbolActor; this->SymbolActor = NULL;
    delete [] this->TextMapper; this->TextMapper = NULL;
    delete [] this->TextActor; this->TextActor = NULL;

    delete [] this->IconImage; this->IconImage = NULL;
    delete [] this->Icon; this->Icon = NULL;
    delete [] this->IconActor; this->IconActor = NULL;
    delete [] this->IconMapper; this->IconMapper = NULL;
    delete [] this->IconTransform; this->IconTransform = NULL;
    delete [] this->IconTransformFilter; this->IconTransformFilter = NULL;
  }//if entries have been defined
}

//----------------------------------------------------------------------------
void vtkLegendBoxActor::SetNumberOfEntries(int num)
{
  if ( num == this->NumberOfEntries )
  {
    return;
  }

  else if ( num < this->Size )
  {
    this->NumberOfEntries = num;
  }

  else //allocate space
  {
    int i;

    //Create internal actors, etc.
    vtkDoubleArray *colors = vtkDoubleArray::New();
    colors->SetNumberOfComponents(3);
    colors->SetNumberOfTuples(num);
    vtkTextMapper **textMapper= new vtkTextMapper* [num];
    vtkActor2D **textActor = new vtkActor2D* [num];

    // Symbol
    vtkPolyData **symbol = new vtkPolyData* [num];
    vtkTransform **transform= new vtkTransform* [num];
    vtkTransformPolyDataFilter **symbolTransform =
                                 new vtkTransformPolyDataFilter* [num];
    vtkPolyDataMapper2D **symbolMapper = new vtkPolyDataMapper2D* [num];
    vtkActor2D **symbolActor = new vtkActor2D* [num];

    // Icon.
    vtkPlaneSource **icon = new vtkPlaneSource* [num];
    vtkTransform **iconTransform= new vtkTransform* [num];
    vtkTransformPolyDataFilter **iconTransformFilter =
                                 new vtkTransformPolyDataFilter* [num];
    vtkPolyDataMapper2D **iconMapper = new vtkPolyDataMapper2D* [num];
    vtkTexturedActor2D **iconActor = new vtkTexturedActor2D* [num];
    vtkImageData **iconImage = new vtkImageData* [num];

    //copy old values
    for (i=0; i<this->NumberOfEntries; i++)
    {
      colors->SetTuple(i,this->Colors->GetTuple(i));
      textMapper[i] = this->TextMapper[i];
      textMapper[i]->Register(this);
      textActor[i] = this->TextActor[i];
      textActor[i]->Register(this);

      // Symbol.
      symbol[i] = this->Symbol[i];
      if ( symbol[i] )
      {
        symbol[i]->Register(this);
      }

      transform[i] = this->Transform[i];
      transform[i]->Register(this);

      symbolTransform[i] = this->SymbolTransform[i];
      symbolTransform[i]->Register(this);

      symbolMapper[i] = this->SymbolMapper[i];
      symbolMapper[i]->Register(this);

      symbolActor[i] = this->SymbolActor[i];
      symbolActor[i]->Register(this);

      // Icon.
      icon[i] = this->Icon[i];
      icon[i]->Register(this);

      iconTransform[i] = this->IconTransform[i];
      iconTransform[i]->Register(this);

      iconTransformFilter[i] = this->IconTransformFilter[i];
      iconTransformFilter[i]->Register(this);

      iconMapper[i] = this->IconMapper[i];
      iconMapper[i]->Register(this);

      iconActor[i] = this->IconActor[i];
      iconActor[i]->Register(this);

      iconImage[i] = this->IconImage[i];
      if( iconImage[i] )
      {
        iconImage[i]->Register(this);
      }
    }

    //initialize data values
    static double color[3]={-1.0,-1.0,-1.0};
    for (i=this->NumberOfEntries; i<num; i++) //initialize
    {
      colors->SetTuple(i,color);
      textMapper[i] = vtkTextMapper::New();
      textActor[i] = vtkActor2D::New();
      textActor[i]->SetMapper(textMapper[i]);

      // Symbol.
      symbol[i] = NULL;
      transform[i] = vtkTransform::New();
      symbolTransform[i] = vtkTransformPolyDataFilter::New();
      symbolTransform[i]->SetTransform(transform[i]);
      symbolMapper[i] = vtkPolyDataMapper2D::New();
      symbolMapper[i]->SetInputConnection(
        symbolTransform[i]->GetOutputPort());
      symbolActor[i] = vtkActor2D::New();
      symbolActor[i]->SetMapper(symbolMapper[i]);

      // Icon.
      iconImage[i] = NULL;

      icon[i] = vtkPlaneSource::New();
      icon[i]->SetPoint1(1.0, 0.0, 0.0);
      icon[i]->SetPoint2(0, 1.0, 0.0);
      icon[i]->SetOrigin(0.0, 0.0, 0.0);
      icon[i]->SetResolution(1, 1);

      iconTransform[i] = vtkTransform::New();

      iconTransformFilter[i] = vtkTransformPolyDataFilter::New();
      iconTransformFilter[i]->SetTransform(iconTransform[i]);

      iconMapper[i] = vtkPolyDataMapper2D::New();
      iconMapper[i]->SetInputConnection(
        iconTransformFilter[i]->GetOutputPort());

      iconActor[i] = vtkTexturedActor2D::New();
      iconActor[i]->SetMapper(iconMapper[i]);
    }

    //Clear out the old stuff
    this->InitializeEntries();

    //Bring everything up to date
    this->NumberOfEntries = this->Size = num;
    this->Colors = colors;
    this->TextMapper = textMapper;
    this->TextActor = textActor;

    this->Symbol = symbol;
    this->Transform = transform;
    this->SymbolTransform = symbolTransform;
    this->SymbolMapper = symbolMapper;
    this->SymbolActor = symbolActor;

    this->Icon = icon;
    this->IconTransform = iconTransform;
    this->IconTransformFilter = iconTransformFilter;
    this->IconMapper = iconMapper;
    this->IconActor = iconActor;
    this->IconImage = iconImage;
  }

  this->Modified();
  return;
}

//----------------------------------------------------------------------------
void vtkLegendBoxActor::SetEntry(int i, vtkPolyData *symbol, const char* string,
                                 double color[3])
{
  if ( i >= 0 && i < this->NumberOfEntries )
  {
    this->SetEntrySymbol(i,symbol);
    this->SetEntryString(i,string);
    this->SetEntryColor(i,color);
  }

  return;
}

//----------------------------------------------------------------------------
void vtkLegendBoxActor::SetEntry(int i, vtkImageData *icon, const char* string, double color[3])
{
  if ( i >= 0 && i < this->NumberOfEntries )
  {
    this->SetEntryIcon(i,icon);
    this->SetEntryString(i,string);
    this->SetEntryColor(i,color);
  }

  return;
}


//----------------------------------------------------------------------------
void vtkLegendBoxActor::SetEntry(int i, vtkPolyData *symbol, vtkImageData *icon,
                                 const char* string, double color[3])
{
  if ( i >= 0 && i < this->NumberOfEntries )
  {
    this->SetEntry(i, symbol, string, color);
    this->SetEntryIcon(i, icon);
  }

  return;
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
void vtkLegendBoxActor::SetEntryIcon(int i, vtkImageData *icon)
{
  if ( i >= 0 && i < this->NumberOfEntries )
  {
    if ( this->IconImage[i] == icon )
    {
      return;
    }
    if ( this->IconImage[i] )
    {
      this->IconImage[i]->Delete();
    }
    this->IconImage[i] = icon;
    if ( this->IconImage[i] )
    {
      this->IconImage[i]->Register(this);
    }
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void vtkLegendBoxActor::SetEntryString(int i, const char* string)
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

//----------------------------------------------------------------------------
void vtkLegendBoxActor::SetEntryColor(int i, double color[3])
{
  if ( i >= 0 && i < this->NumberOfEntries )
  {
    double oldColor[3];
    this->Colors->GetTuple(i, oldColor);

    if ( oldColor[0] != color[0] || oldColor[1] != color[1] ||
         oldColor[2] != color[2] )
    {
      this->Colors->SetTuple3(i,color[0],color[1],color[2]);
      this->Modified();
    }
  }
}

//----------------------------------------------------------------------------
void vtkLegendBoxActor::SetEntryColor(int i, double r, double g, double b)
{
  double rgb[3];
  rgb[0] = r; rgb[1] = g; rgb[2] = b;
  this->SetEntryColor(i,rgb);
}

//----------------------------------------------------------------------------
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

//----------------------------------------------------------------------------
vtkImageData* vtkLegendBoxActor::GetEntryIcon(int i)
{
  if ( i < 0 || i >= this->NumberOfEntries )
  {
    return NULL;
  }
  else
  {
    return this->IconImage[i];
  }
}

//----------------------------------------------------------------------------
const char* vtkLegendBoxActor::GetEntryString(int i)
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

//----------------------------------------------------------------------------
double* vtkLegendBoxActor::GetEntryColor(int i)
{
  if ( i < 0 || i >= this->NumberOfEntries )
  {
    return NULL;
  }
  else
  {
    return vtkArrayDownCast<vtkDoubleArray>(this->Colors)->GetPointer(i*3);
  }
}

//----------------------------------------------------------------------------
// Release any graphics resources that are being consumed by this actor.
// The parameter window could be used to determine which graphic
// resources to release.
void vtkLegendBoxActor::ReleaseGraphicsResources(vtkWindow *win)
{
  if ( this->BackgroundActor )
  {
     this->BackgroundActor->ReleaseGraphicsResources(win);
  }

  if ( this->BorderActor )
  {
     this->BorderActor->ReleaseGraphicsResources(win);
  }

  if ( this->BoxActor )
  {
     this->BoxActor->ReleaseGraphicsResources(win);
  }

  for (int i=0; i < this->Size; i++)
  {
    this->TextActor[i]->ReleaseGraphicsResources(win);
    this->SymbolActor[i]->ReleaseGraphicsResources(win);
    this->IconActor[i]->ReleaseGraphicsResources(win);
  }
}

//----------------------------------------------------------------------------
int vtkLegendBoxActor::RenderOverlay(vtkViewport *viewport)
{
  if ( this->NumberOfEntries <= 0 )
  {
    return 0;
  }

  int renderedSomething = 0;
  if ( this->BackgroundActor && this->UseBackground )
  {
    this->BackgroundActor->RenderOverlay(viewport);
  }

  if ( this->Border )
  {
    renderedSomething += this->BorderActor->RenderOverlay(viewport);
  }

  if ( this->Box )
  {
    renderedSomething += this->BoxActor->RenderOverlay(viewport);
  }

  if ( this->LegendEntriesVisible )
  {
    for (int i=0; i<this->NumberOfEntries; i++)
    {
      if ( this->Symbol[i] )
      {
        renderedSomething += this->SymbolActor[i]->RenderOverlay(viewport);
      }
      if ( this->IconImage[i] )
      {
        renderedSomething += this->IconActor[i]->RenderOverlay(viewport);
      }

      renderedSomething += this->TextActor[i]->RenderOverlay(viewport);
    }
  }

  return renderedSomething;
}

//----------------------------------------------------------------------------
int vtkLegendBoxActor::RenderOpaqueGeometry(vtkViewport *viewport)
{
  int i;
  double symbolSize;

  if ( this->NumberOfEntries <= 0 )
  {
    return 0;
  }

  if (!this->EntryTextProperty)
  {
    vtkErrorMacro(<<"Need entry text property to render legend box actor");
    return 0;
  }

  // Check to see whether we have to rebuild everything
  int *vsize = viewport->GetSize();
  if (this->GetMTime() > this->BuildTime ||
      this->EntryTextProperty->GetMTime()  > this->BuildTime ||
      vsize[0] != this->CachedSize[0] || vsize[1] != this->CachedSize[1] )
  {
    vtkDebugMacro(<<"Rebuilding text");
    this->CachedSize[0] = vsize[0];
    this->CachedSize[1] = vsize[1];

    // If text prop has changed, recopy it to all mappers
    // We have to use shallow copy since the color of each text prop
    // can be overridden

    if (this->EntryTextProperty->GetMTime()  > this->BuildTime)
    {
      for (i = 0; i < this->NumberOfEntries; i++)
      {
        this->TextMapper[i]->GetTextProperty()->ShallowCopy(
          this->EntryTextProperty);
      }
    }

    //Get position information
    int *x1, *x2;
    double p1[3], p2[3];
    x1 = this->PositionCoordinate->GetComputedViewportValue(viewport);
    x2 = this->Position2Coordinate->GetComputedViewportValue(viewport);
    p1[0] = (double)x1[0]; p1[1] = (double)x1[1]; p1[2] = 0.0;
    p2[0] = (double)x2[0]; p2[1] = (double)x2[1]; p2[2] = 0.0;

    //Compute spacing...trying to keep things proportional
    //
    //Find the longest string and symbol width ratio
    int length, maxLength;
    int maxTextMapper = 0;
    int tempi[2], fontSize;
    double sf, twr, swr;
    const double *bounds;
    bool iconExists   (false);
    bool symbolExists (false);

    for (swr=0.0, maxLength=i=0; i<this->NumberOfEntries; i++)
    {
      this->TextMapper[i]->GetTextProperty()->SetFontSize(12);
      length = this->TextMapper[i]->GetWidth(viewport);
      if ( length > maxLength )
      {
        maxLength = length;
        maxTextMapper = i;
      }

      if ( this->Symbol[i] ) //if there is a symbol
      {
        symbolExists = true;
        //this->Symbol[i]->Update();
        bounds = this->Symbol[i]->GetBounds();
        if ( (bounds[3]-bounds[2]) == 0.0 )
        {
          sf = 1.0;
        }
        else
        {
          sf = (bounds[1]-bounds[0]) / (bounds[3]-bounds[2]);
        }
        if ( sf > swr )
        {
          swr = sf;
        }
      }//if symbol defined

        // We pick the one with highest ratio if both symbol and icon
        // exists.
        if(this->IconImage[i])
        {
          iconExists = true;

          bounds = this->IconImage[i]->GetBounds();
          if ( (bounds[3]-bounds[2]) == 0.0 )
          {
            sf = 1.0;
          }
          else
          {
            sf = (bounds[1]-bounds[0]) / (bounds[3]-bounds[2]);
          }
          if ( sf > swr )
          {
            swr = sf;
          }
        } // if icon defined.
    }

    //Compute the final proportion (symbol width to text width)
    fontSize = 12;
    this->TextMapper[maxTextMapper]->GetTextProperty()->SetFontSize(fontSize);
    this->TextMapper[maxTextMapper]->GetSize(viewport,tempi);

    if(maxLength>0) // make sure that tempi is not 0, to avoid a
      // divide-by-zero floating-point exception.
    {
      twr = (double)tempi[0]/tempi[1];
      symbolSize = swr / (swr + twr);
    }
    else
    {
      symbolSize=0;
    }

    if(iconExists && symbolExists)
    {
      symbolSize *= 2;
    }

    //Okay, now that the proportions are okay, let's size everything
    //First the text
    int size[2];
    size[0] = (int)((1.0-symbolSize)*(p2[0] - p1[0] - 2.0*this->Padding));
    size[1] = (int)((p2[1] - p1[1] - 2.0*this->Padding)/this->NumberOfEntries);

    fontSize = this->TextMapper[maxTextMapper]->SetConstrainedFontSize(
      viewport, size[0], size[1]);
    this->TextMapper[maxTextMapper]->GetSize(viewport,tempi);

    // don't draw anything if it's too small
    if ( size[1] > 0 && fontSize > 0)
    {
      this->LegendEntriesVisible = 1;
    }
    else
    {
      this->LegendEntriesVisible = 0;
    }

    //Border and box - may adjust spacing based on font size relationship
    //to the proportions relative to the border
    //
    if (this->Border || this->Box)
    {
      //adjust the border/box placement if too much whitespace
      if ( !this->LockBorder && tempi[0] < size[0] )
      {
        p2[0] = p1[0] + 2. * this->Padding
          + symbolSize * ( p2[0] - p1[0] - 2. * this->Padding ) + tempi[0];
      }
      vtkPoints *pts = this->BorderPolyData->GetPoints();
      pts->SetPoint( 0, p1 );
      pts->SetPoint(1, p2[0], p1[1], 0. );
      pts->SetPoint(2, p2[0], p2[1], 0. );
      pts->SetPoint(3, p1[0], p2[1], 0. );
      pts->Modified();
    }

    if ( this->UseBackground )
    {
      this->Background->SetOrigin( p1[0], p1[1], 0. );
      this->Background->SetPoint1( p2[0], p1[1], 0. );
      this->Background->SetPoint2( p1[0], p2[1], 0. );

      this->BackgroundMapper->SetInputConnection( this->Background->GetOutputPort() );
      this->BackgroundActor->GetProperty()->SetOpacity( this->BackgroundOpacity );
      this->BackgroundActor->GetProperty()->SetColor( this->BackgroundColor );
    }

    if ( this->Border )
    {
      this->BorderActor->SetProperty( this->GetProperty() );
    }

    //Place text strings
    double color[3];
    double posY;
    double posX = p1[0] + this->Padding +
                 symbolSize*(p2[0] - p1[0] - 2.0*this->Padding);
    for (i=0; i<this->NumberOfEntries; i++)
    {
      posY = p2[1] - this->Padding - (double)i*size[1] - 0.5*size[1];
      this->TextActor[i]->SetPosition(posX,posY);
      this->TextMapper[i]->GetTextProperty()->SetFontSize(fontSize);
      this->TextMapper[i]->GetTextProperty()->SetVerticalJustification(
            VTK_TEXT_CENTERED);
      this->TextMapper[i]->GetTextProperty()->SetJustification(VTK_TEXT_LEFT);
      this->Colors->GetTuple(i, color);
      if ( color[0] >= 0.0 && color[1] >= 0.0 && color[2] >= 0.0 )
      {
        this->TextMapper[i]->GetTextProperty()->SetColor(color[0],
                                                         color[1],
                                                         color[2]);
      }
    }

    double sizeFraction = 1.0;

    double symbolsPositionFraction = 0.5;
    double iconsPositionFraction   = 0.0;

    if(symbolExists && iconExists)
    {
      symbolsPositionFraction = 0.25;
      iconsPositionFraction   = 0.625;

      sizeFraction = 0.5;
    }
    else if(iconExists)
    {
      iconsPositionFraction = 0.5;
    }
    else
    {
      // Do nothing.
    }

    //Place symbols
    //
    //Find the x-y bounds of the symbols...we'll be scaling these as well
    size[0] = (int)(sizeFraction * symbolSize*(p2[0] - p1[0] - 2.0*this->Padding));
    posX = p1[0] + this->Padding +
                 symbolsPositionFraction * symbolSize*(p2[0] - p1[0] - 2.0*this->Padding);
    for (i=0; i<this->NumberOfEntries; i++)
    {
      if ( this->Symbol[i] )
      {
        this->SymbolTransform[i]->SetInputData(this->Symbol[i]);
        bounds = this->Symbol[i]->GetBounds();

        if ( (bounds[1]-bounds[0]) == 0.0 )
        {
          sf = VTK_DOUBLE_MAX;
        }
        else
        {
          sf = size[0]/(bounds[1]-bounds[0]);
        }

        if ( (bounds[3]-bounds[2]) == 0.0 )
        {
          if ( sf >= VTK_DOUBLE_MAX )
          {
            sf = 1.0;
          }
        }
        else if ( (size[1]/(bounds[3]-bounds[2])) < sf )
        {
          sf = size[1]/(bounds[3]-bounds[2]);
        }

        posY = p2[1] - this->Padding - (double)i*size[1] - 0.5*size[1] -
                       0.25*tempi[1];
        this->Transform[i]->Identity();
        this->Transform[i]->Translate(posX, posY, 0.0);
        this->Transform[i]->Scale(0.5*sf, 0.5*sf, 1);
        this->SymbolMapper[i]->SetScalarVisibility(this->ScalarVisibility);
        this->SymbolActor[i]->GetProperty()->DeepCopy(this->GetProperty());
        this->Colors->GetTuple(i, color);
        if ( color[0] >= 0.0 && color[1] >= 0.0 && color[2] >= 0.0 )
        {
          this->SymbolActor[i]->GetProperty()->SetColor(color[0],
                                                        color[1],
                                                        color[2]);
        }
      }//if symbol defined
      else
      {
        std::cout << "Symbol is not defined: " << std::endl;
      }
    }

      // Place icons.
      size[0] = (int)(sizeFraction * symbolSize*(p2[0] - p1[0] - 2.0*this->Padding));
      posX = p1[0] + this->Padding +
                   iconsPositionFraction * symbolSize*(p2[0] - p1[0] - 2.0*this->Padding);
      for (i=0; i<this->NumberOfEntries; i++)
      {
        if ( this->IconImage[i] )
        {
          vtkTexture *texture (vtkTexture::New());
          texture->SetInputData(this->IconImage[i]);
          this->IconActor[i]->SetTexture(texture);
          texture->Delete();
          this->Icon[i]->Update();
          this->IconTransformFilter[i]->SetInputConnection(
            this->Icon[i]->GetOutputPort());
          this->IconTransformFilter[i]->Update();
          bounds = this->Icon[i]->GetOutput(0)->GetBounds();

          if ( (bounds[1]-bounds[0]) == 0.0 )
          {
            sf = VTK_DOUBLE_MAX;
          }
          else
          {
            sf = size[0]/(bounds[1]-bounds[0]);
          }

          if ( (bounds[3]-bounds[2]) == 0.0 )
          {
            if ( sf >= VTK_DOUBLE_MAX )
            {
              sf = 1.0;
            }
          }
          else if ( (size[1]/(bounds[3]-bounds[2])) < sf )
          {
            sf = size[1]/(bounds[3]-bounds[2]);
          }

          posY = p2[1] - this->Padding - (double)i*size[1] - 0.5*size[1] -
                         0.25*tempi[1];
          this->IconTransform[i]->Identity();
          this->IconTransform[i]->Translate(posX, posY, 0.0);
          this->IconTransform[i]->Scale(0.5 * sf, 0.5 * sf, 1);
          this->IconMapper[i]->SetScalarVisibility(this->ScalarVisibility);
        }// If icon is defined.
      }
    this->BuildTime.Modified();
  }//rebuild legend box

  //Okay, now we're ready to render something
  //Border
  int renderedSomething = 0;
  if ( this->BackgroundActor && this->UseBackground )
  {
    this->BackgroundActor->RenderOpaqueGeometry(viewport);
  }

  if ( this->Border )
  {
    renderedSomething += this->BorderActor->RenderOpaqueGeometry(viewport);
  }

  if ( this->Box )
  {
    renderedSomething += this->BoxActor->RenderOpaqueGeometry(viewport);
  }

  if ( this->LegendEntriesVisible )
  {
    for (i=0; i<this->NumberOfEntries; i++)
    {
      if ( this->Symbol[i] )
      {
        renderedSomething += this->SymbolActor[i]->RenderOpaqueGeometry(viewport);
      }
      if( this->IconImage[i])
      {
        renderedSomething += this->IconActor[i]->RenderOpaqueGeometry(viewport);
      }
      renderedSomething += this->TextActor[i]->RenderOpaqueGeometry(viewport);
    }
  }

  return renderedSomething;
}

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
int vtkLegendBoxActor::HasTranslucentPolygonalGeometry()
{
  return 0;
}

//----------------------------------------------------------------------------
void vtkLegendBoxActor::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  if (this->EntryTextProperty)
  {
    os << indent << "Entry Text Property:\n";
    this->EntryTextProperty->PrintSelf(os,indent.GetNextIndent());
  }
  else
  {
    os << indent << "Entry Text Property: (none)\n";
  }

  os << indent << "Number Of Entries: " << this->NumberOfEntries << "\n";

  os << indent << "Scalar Visibility: "
     << (this->ScalarVisibility ? "On\n" : "Off\n");
  os << indent << "Padding: " << this->Padding << "\n";
  os << indent << "Border: " << (this->Border ? "On\n" : "Off\n");
  os << indent << "Box: " << (this->Box ? "On\n" : "Off\n");
  os << indent << "LockBorder: " << (this->LockBorder ? "On\n" : "Off\n");

  os << indent << "UseBackgroud: " << (this->UseBackground ? "On\n" : "Off\n");
  os << indent << "BackgroundOpacity: " << this->BackgroundOpacity << "\n";

  os << indent << "BackgroundColor: (" << this->BackgroundColor[0] << ", "
    << this->BackgroundColor[1] << ", " << this->BackgroundColor[2] << ")\n";
}

//----------------------------------------------------------------------------
void vtkLegendBoxActor::ShallowCopy(vtkProp *prop)
{
  vtkLegendBoxActor *a = vtkLegendBoxActor::SafeDownCast(prop);
  if ( a != NULL )
  {
    this->SetPosition2(a->GetPosition2());
    this->SetEntryTextProperty(a->GetEntryTextProperty());
    this->SetBorder(a->GetBorder());
    this->SetLockBorder(a->GetLockBorder());
    this->SetPadding(a->GetPadding());
    this->SetScalarVisibility(a->GetScalarVisibility());
    this->SetNumberOfEntries(a->GetNumberOfEntries());
    for (int i=0; i<this->NumberOfEntries; i++)
    {
      this->SetEntrySymbol(i,a->GetEntrySymbol(i));
      this->SetEntryString(i,a->GetEntryString(i));
      this->SetEntryColor(i,a->GetEntryColor(i));
    }
  }

  // Now do superclass
  this->vtkActor2D::ShallowCopy(prop);
}
