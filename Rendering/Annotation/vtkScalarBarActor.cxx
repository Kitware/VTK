/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalarBarActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkScalarBarActor.h"

#include "vtkCellArray.h"
#include "vtkCellData.h"
#include "vtkObjectFactory.h"
#include "vtkPolyData.h"
#include "vtkPolyDataMapper2D.h"
#include "vtkScalarsToColors.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"
#include "vtkViewport.h"
#include "vtkWindow.h"
#include "vtkFloatArray.h"
#include "vtkPointData.h"
#include "vtkTexture.h"
#include "vtkImageData.h"
#include "vtkRenderer.h"
#include "vtkMathTextActor.h"
#include "vtkProperty2D.h"

#include <vector>
#include <set>

vtkStandardNewMacro(vtkScalarBarActor);

vtkCxxSetObjectMacro(vtkScalarBarActor,LookupTable,vtkScalarsToColors);
vtkCxxSetObjectMacro(vtkScalarBarActor,LabelTextProperty,vtkTextProperty);
vtkCxxSetObjectMacro(vtkScalarBarActor,TitleTextProperty,vtkTextProperty);
vtkCxxSetObjectMacro(vtkScalarBarActor,BackgroundProperty,vtkProperty2D);
vtkCxxSetObjectMacro(vtkScalarBarActor,FrameProperty,vtkProperty2D);

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
  this->ComponentTitle = NULL;

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
  this->ScalarBarMapper->SetInputData(this->ScalarBar);
  this->ScalarBarActor = vtkActor2D::New();
  this->ScalarBarActor->SetMapper(this->ScalarBarMapper);
  this->ScalarBarActor->GetPositionCoordinate()->
    SetReferenceCoordinate(this->PositionCoordinate);
  this->LastOrigin[0] = 0;
  this->LastOrigin[1] = 0;
  this->LastSize[0] = 0;
  this->LastSize[1] = 0;

  this->DrawAnnotations = 1;
  this->AnnotationLeaderPadding = 8.;
  this->AnnotationBoxes = vtkPolyData::New();
  this->AnnotationBoxesMapper = vtkPolyDataMapper2D::New();
  this->AnnotationBoxesActor = vtkActor2D::New();
  this->AnnotationBoxesMapper->SetInputData( this->AnnotationBoxes );
  this->AnnotationBoxesActor->SetMapper( this->AnnotationBoxesMapper );
  this->AnnotationBoxesActor->GetPositionCoordinate()->
    SetReferenceCoordinate( this->PositionCoordinate );
  this->AnnotationLeaders = vtkPolyData::New();
  this->AnnotationLeadersMapper = vtkPolyDataMapper2D::New();
  this->AnnotationLeadersActor = vtkActor2D::New();
  this->AnnotationLeadersMapper->SetInputData( this->AnnotationLeaders );
  this->AnnotationLeadersActor->SetMapper( this->AnnotationLeadersMapper );
  this->AnnotationLeadersActor->GetPositionCoordinate()->
    SetReferenceCoordinate( this->PositionCoordinate );
  this->AnnotationLabels = 0; // Can't allocate until we have a lookup table.
  this->NumberOfAnnotationLabelsBuilt = 0;

  // If opacity is on, a jail like texture is displayed behind it..

  this->UseOpacity       = 0;
  this->TextureGridWidth = 10.0;

  this->TexturePolyData = vtkPolyData::New();
  vtkPolyDataMapper2D * textureMapper = vtkPolyDataMapper2D::New();
  textureMapper->SetInputData(this->TexturePolyData);
  this->TextureActor = vtkActor2D::New();
  this->TextureActor->SetMapper(textureMapper);
  textureMapper->Delete();
  this->TextureActor->GetPositionCoordinate()->
    SetReferenceCoordinate(this->PositionCoordinate);
  vtkFloatArray* tc = vtkFloatArray::New();
  tc->SetNumberOfComponents(2);
  tc->SetNumberOfTuples(4);
  tc->InsertComponent(0,0, 0.0);
  tc->InsertComponent(0,1, 0.0);
  tc->InsertComponent(1,1, 0.0);
  tc->InsertComponent(3,0, 0.0);
  this->TexturePolyData->GetPointData()->SetTCoords(tc);
  tc->Delete();

  vtkCellArray* polys2 = vtkCellArray::New();
  polys2->InsertNextCell(4);
  polys2->InsertCellPoint(0);
  polys2->InsertCellPoint(1);
  polys2->InsertCellPoint(2);
  polys2->InsertCellPoint(3);
  this->TexturePolyData->SetPolys(polys2);
  polys2->Delete();

  vtkProperty2D *imageProperty = vtkProperty2D::New();
  imageProperty->SetOpacity(0.08);
  this->TextureActor->SetProperty(imageProperty);
  imageProperty->Delete();

  // Create the default texture. Just a "Jail" like grid

  const unsigned int dim = 128;
  vtkImageData *image = vtkImageData::New();
  image->SetDimensions(dim, dim, 1);
  image->AllocateScalars(VTK_UNSIGNED_CHAR, 1);

  for (unsigned int y = 0; y < dim; y++)
    {
    unsigned char *ptr =
      static_cast< unsigned char * >(image->GetScalarPointer(0, y, 0));
    for (unsigned int x = 0; x < dim; x++)
      {
      *ptr = ((x == y) || (x == (dim-y-1))) ? 255 : 0;
      ++ptr;
      }
    }

  this->Texture = vtkTexture::New();
  this->Texture->SetInputData( image );
  this->Texture->RepeatOn();
  image->Delete();

  // Default text position : Above scalar bar if orientation is horizontal
  //                         Right of scalar bar if orientation is vertical
  this->TextPosition = SucceedScalarBar;

  this->MaximumWidthInPixels = VTK_INT_MAX;
  this->MaximumHeightInPixels = VTK_INT_MAX;

  this->BackgroundProperty = vtkProperty2D::New();
  this->FrameProperty = vtkProperty2D::New();

  this->DrawBackground = 0;
  this->Background = vtkPolyData::New();
  this->BackgroundMapper = vtkPolyDataMapper2D::New();
  this->BackgroundMapper->SetInputData(this->Background);
  this->BackgroundActor = vtkActor2D::New();
  this->BackgroundActor->SetMapper(this->BackgroundMapper);
  this->BackgroundActor->GetPositionCoordinate()->SetReferenceCoordinate(this->PositionCoordinate);

  this->DrawFrame = 0;
  this->Frame = vtkPolyData::New();
  this->FrameMapper = vtkPolyDataMapper2D::New();
  this->FrameMapper->SetInputData(this->Frame);
  this->FrameActor = vtkActor2D::New();
  this->FrameActor->SetMapper(this->FrameMapper);
  this->FrameActor->GetPositionCoordinate()->SetReferenceCoordinate(this->PositionCoordinate);

  this->DrawColorBar = true;
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
  if ( this->AnnotationLabels != NULL )
    {
    for ( int i = 0; i < this->NumberOfAnnotationLabelsBuilt; ++ i )
      {
      this->AnnotationLabels[i]->ReleaseGraphicsResources(win);
      }
    }
  this->ScalarBarActor->ReleaseGraphicsResources(win);
  this->AnnotationBoxesActor->ReleaseGraphicsResources(win);
  this->AnnotationLeadersActor->ReleaseGraphicsResources(win);
  this->BackgroundActor->ReleaseGraphicsResources(win);
  this->FrameActor->ReleaseGraphicsResources(win);
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

  if ( this->AnnotationLabels != NULL )
    {
    for ( int i = 0; i < this->NumberOfAnnotationLabelsBuilt; ++ i )
      {
      this->AnnotationLabels[i]->Delete();
      }
    delete [] this->AnnotationLabels;
    }

  this->ScalarBar->Delete();
  this->ScalarBarMapper->Delete();
  this->ScalarBarActor->Delete();

  this->AnnotationBoxes->Delete();
  this->AnnotationBoxesMapper->Delete();
  this->AnnotationBoxesActor->Delete();

  this->AnnotationLeaders->Delete();
  this->AnnotationLeadersMapper->Delete();
  this->AnnotationLeadersActor->Delete();

  if (this->Title)
    {
    delete [] this->Title;
    this->Title = NULL;
    }

  if ( this->ComponentTitle )
    {
    delete [] this->ComponentTitle;
    this->ComponentTitle = NULL;
    }

  this->SetLookupTable(NULL);
  this->SetLabelTextProperty(NULL);
  this->SetTitleTextProperty(NULL);
  this->Texture->Delete();
  this->TextureActor->Delete();
  this->TexturePolyData->Delete();
  this->Background->Delete();
  this->BackgroundMapper->Delete();
  this->BackgroundActor->Delete();
  this->Frame->Delete();
  this->FrameMapper->Delete();
  this->FrameActor->Delete();
  this->SetBackgroundProperty(NULL);
  this->SetFrameProperty(NULL);
}

//----------------------------------------------------------------------------
int vtkScalarBarActor::RenderOverlay(vtkViewport *viewport)
{
  int renderedSomething = 0;
  int i;

  if (this->DrawBackground)
    {
    renderedSomething += this->BackgroundActor->RenderOverlay(viewport);
    }

  if (this->DrawFrame)
    {
    renderedSomething += this->FrameActor->RenderOverlay(viewport);
    }

  if (this->UseOpacity && this->DrawColorBar)
    {
    this->Texture->Render(vtkRenderer::SafeDownCast(viewport));
    renderedSomething += this->TextureActor->RenderOverlay(viewport);
    }

  // Everything is built, just have to render
  if (this->Title != NULL)
    {
    renderedSomething += this->TitleActor->RenderOverlay(viewport);
    }

  // Draw either the scalar bar (non-indexed mode) or the annotated value boxes (indexed mode).
  if ( ! this->LookupTable->GetIndexedLookup() )
    {
    if (this->DrawColorBar)
      {
      this->ScalarBarActor->RenderOverlay(viewport);
      }

    if( this->TextActors == NULL)
      {
      vtkWarningMacro(<<"Need a mapper to render a scalar bar");
      return renderedSomething;
      }

    for (i=0; i<this->NumberOfLabels; i++)
      {
      renderedSomething += this->TextActors[i]->RenderOverlay(viewport);
      }
    }
  else if (this->DrawColorBar)
    {
    this->AnnotationBoxesActor->RenderOverlay(viewport);
    }

  if ( this->AnnotationLabels == NULL && this->NumberOfAnnotationLabelsBuilt )
    {
    vtkWarningMacro(<<"Need a mapper to render the scalar bar");
    return renderedSomething;
    }

  if ( this->DrawAnnotations )
    {
    if ( this->NumberOfAnnotationLabelsBuilt )
      this->AnnotationLeadersActor->RenderOverlay( viewport );
    for ( i = 0; i < this->NumberOfAnnotationLabelsBuilt; ++ i )
      {
      renderedSomething += this->AnnotationLabels[i]->RenderOverlay( viewport );
      }
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

  if (!this->LookupTable)
    {
    vtkWarningMacro(<<"Need a mapper to render a scalar bar");
    return 0;
    }

  if (!this->TitleTextProperty)
    {
    vtkErrorMacro(<<"Need title text property to render a scalar bar");
    return 0;
    }

  if (!this->LabelTextProperty)
    {
    vtkErrorMacro(<<"Need label text property to render a scalar bar");
    return 0;
    }

  // Check to see whether we have to rebuild everything
  int positionsHaveChanged = 0;
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

    // Check if we have bounds on the maximum size
    size[0] = size[0] > this->MaximumWidthInPixels
            ? this->MaximumWidthInPixels : size[0];
    size[1] = size[1] > this->MaximumHeightInPixels
            ? this->MaximumHeightInPixels : size[1];

    if (this->LastSize[0] != size[0] ||
        this->LastSize[1] != size[1] ||
        this->LastOrigin[0] != barOrigin[0] ||
        this->LastOrigin[1] != barOrigin[1])
      {
      positionsHaveChanged = 1;
      }
    }

  // Check to see whether we have to rebuild everything
  if (positionsHaveChanged ||
      this->GetMTime() > this->BuildTime ||
      this->LookupTable->GetMTime() > this->BuildTime ||
      this->LabelTextProperty->GetMTime() > this->BuildTime ||
      this->TitleTextProperty->GetMTime() > this->BuildTime ||
      this->BackgroundProperty->GetMTime() > this->BuildTime ||
      this->FrameProperty->GetMTime() > this->BuildTime)

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

    if ( this->AnnotationLabels != NULL )
      {
      for ( i = 0; i < this->NumberOfAnnotationLabelsBuilt; ++ i )
        {
        this->AnnotationLabels[i]->Delete();
        }
      delete [] this->AnnotationLabels;
      this->AnnotationLabels = 0;
      this->NumberOfAnnotationLabelsBuilt = 0;
      }

    // Build scalar bar object; determine its type
    // i.e. is scale set to log, is categorical or continuous?
    int isLogTable = this->LookupTable->UsingLogScale();
    int isCategorical = this->LookupTable->GetIndexedLookup();

    // we hard code how many steps to display
    vtkScalarsToColors *lut = this->LookupTable;
    int numColors = this->MaximumNumberOfColors;
    double *range = lut->GetRange();

    int numPts = 2*(numColors + 1);
    vtkPoints *pts = vtkPoints::New();
    pts->SetNumberOfPoints(numPts);
    vtkCellArray *polys = vtkCellArray::New();
    polys->Allocate(polys->EstimateSize(numColors,4));
    vtkUnsignedCharArray *colors = vtkUnsignedCharArray::New();

    unsigned int nComponents = ((this->UseOpacity) ? 4 : 3);
    colors->SetNumberOfComponents( nComponents );
    colors->SetNumberOfTuples(numColors);

    this->ScalarBarActor->SetProperty(this->GetProperty());
    this->ScalarBar->Initialize();
    this->ScalarBar->SetPoints(pts);
    this->ScalarBar->SetPolys(polys);
    this->ScalarBar->GetCellData()->SetScalars(colors);
    pts->Delete(); polys->Delete(); colors->Delete();

    // set frame structure
    vtkPoints *frPts = vtkPoints::New();
    frPts->SetNumberOfPoints(5);
    vtkCellArray *frLines = vtkCellArray::New();
    frLines->Allocate(frLines->EstimateSize(1,5));

    this->FrameActor->SetProperty(this->FrameProperty);
    this->Frame->Initialize();
    this->Frame->SetPoints(frPts);
    this->Frame->SetLines(frLines);
    frPts->Delete(); frLines->Delete();

    // set background structure
    vtkPoints *bgPts = vtkPoints::New();
    bgPts->SetNumberOfPoints(4);
    vtkCellArray *bgPolys = vtkCellArray::New();
    bgPolys->Allocate(bgPolys->EstimateSize(1,4));

    this->BackgroundActor->SetProperty(this->BackgroundProperty);
    this->Background->Initialize();
    this->Background->SetPoints(bgPts);
    this->Background->SetPolys(bgPolys);
    bgPts->Delete(); bgPolys->Delete();

    // get the viewport size in display coordinates
    int *barOrigin, barWidth, barHeight;
    barOrigin = this->PositionCoordinate->GetComputedViewportValue(viewport);
    size[0] =
      this->Position2Coordinate->GetComputedViewportValue(viewport)[0] -
      barOrigin[0];
    size[1] =
      this->Position2Coordinate->GetComputedViewportValue(viewport)[1] -
      barOrigin[1];

    // Check if we have bounds on the maximum size
    size[0] = size[0] > this->MaximumWidthInPixels
            ? this->MaximumWidthInPixels : size[0];
    size[1] = size[1] > this->MaximumHeightInPixels
            ? this->MaximumHeightInPixels : size[1];

    this->LastOrigin[0] = barOrigin[0];
    this->LastOrigin[1] = barOrigin[1];
    this->LastSize[0] = size[0];
    this->LastSize[1] = size[1];

    // Update all the composing objects
    this->TitleActor->SetProperty(this->GetProperty());


    //update with the proper title
    if ( this->ComponentTitle && strlen(this->ComponentTitle) > 0 )
      {
      //need to account for a space between title & component and null term
      char *combinedTitle = new char[ ( strlen(this->Title) + strlen(this->ComponentTitle) + 2) ];
      strcpy(combinedTitle, this->Title );
      strcat( combinedTitle, " " );
      strcat( combinedTitle, this->ComponentTitle );
      this->TitleMapper->SetInput(combinedTitle);
      delete [] combinedTitle;
      }
    else
      {
      this->TitleMapper->SetInput(this->Title);
      }

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
    double x[3]; x[2] = 0.0;
    double delta;
    int barX = 0;
    int barY = 0;
    if ( this->Orientation == VTK_ORIENT_VERTICAL )
      {
      // Adjust height and width only in enhanced more or if at least
      // one amongst the frame and the background was requested
      if ( this->DrawBackground ||
           this->DrawFrame )
        {
        barX = static_cast<int>(size[0] * 0.05);
        barY = static_cast<int>(size[1] * 0.05 + labelSize[1] / 2);
        }

      barWidth = size[0] - 4 - labelSize[0] - 2 * barX;
      barHeight = static_cast<int>(0.86*size[1]) - barY;
      delta=static_cast<double>(barHeight)/numColors;
      for (i=0; i<numPts/2; i++)
        {
        x[0] = (this->TextPosition == vtkScalarBarActor::PrecedeScalarBar)
          ? (size[0] - barWidth - barX) : barX;
        x[1] = barY + i*delta;
        pts->SetPoint(2*i,x);
        x[0] = (this->TextPosition == vtkScalarBarActor::PrecedeScalarBar)
          ? size[0] - barX: barX + barWidth;
        pts->SetPoint(2*i+1,x);
        }
      }
    else
      {
      // Adjust height and width only in enhanced more or if at least
      // one amongst the frame and the background was requested
      if ( this->DrawBackground ||
           this->DrawFrame )
        {
        barX = static_cast<int>(size[0] * 0.05) + labelSize[0] / 2;
        barY = static_cast<int>(size[1] * 0.05);
        }
      barWidth = size[0] - 2 * barX;
      barHeight = static_cast<int>(0.4*size[1]) - barY;
      delta=static_cast<double>(barWidth)/numColors;
      for (i=0; i<numPts/2; i++)
        {
        x[0] = barX + i*delta;
        x[1] = (this->TextPosition == vtkScalarBarActor::PrecedeScalarBar)
          ? size[1] - barY: barY + barHeight ;
        pts->SetPoint(2*i,x);
        x[1] = (this->TextPosition == vtkScalarBarActor::PrecedeScalarBar)
          ? (size[1]-barHeight - barY) : barY;
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

      if ( isLogTable )
        {
        double rgbval = log10(range[0]) +
          i*(log10(range[1])-log10(range[0]))/(numColors -1);
        rgba = lut->MapValue(pow(10.0,rgbval));
        }
      else
        {
        rgba = lut->MapValue(range[0] + (range[1] - range[0])*
                             (i /(numColors-1.0)));
        }

      rgb = colors->GetPointer( nComponents * i); //write into array directly
      rgb[0] = rgba[0];
      rgb[1] = rgba[1];
      rgb[2] = rgba[2];
      if (this->UseOpacity)
        {
        rgb[3] = rgba[3];
        }
      }

    // generate background and frame points and cell
    x[0]=0; x[1]=0;
    bgPts->SetPoint(0,x);
    frPts->SetPoint(0,x);
    frPts->SetPoint(4,x);

    x[0]=0; x[1]=size[1];
    bgPts->SetPoint(1,x);
    frPts->SetPoint(1,x);

    x[0]=size[0]; x[1]=size[1];
    bgPts->SetPoint(2,x);
    frPts->SetPoint(2,x);

    x[0]=size[0]; x[1]=0;
    bgPts->SetPoint(3,x);
    frPts->SetPoint(3,x);

    vtkIdType bgIds[5] = {0,1,2,3,4};
    bgPolys->InsertNextCell(4,bgIds);
    frLines->InsertNextCell(5,bgIds);

    // Now position everything properly
    //
    double val;
    int sizeTextData[2];
    if (this->Orientation == VTK_ORIENT_VERTICAL)
      {
      // center the title
      this->TitleActor->SetPosition(size[0]/2, 0.9*size[1]);

      for (i=0; i < this->NumberOfLabels; i++)
        {
        if (this->NumberOfLabels > 1)
          {
          val = static_cast<double>(i)/(this->NumberOfLabels-1) *barHeight + barY;
          }
        else
          {
          val = 0.5*(barHeight + barY);
          }
        this->TextMappers[i]->GetSize(viewport,sizeTextData);
        this->TextMappers[i]->GetTextProperty()->SetJustificationToLeft();
        if (this->TextPosition == vtkScalarBarActor::PrecedeScalarBar)
          {
          this->TextActors[i]->SetPosition(barX,
                                           val - 0.6*sizeTextData[1]);
          }
        else
          {
          this->TextActors[i]->SetPosition(barX + barWidth + 3,
                                           val - 0.6*sizeTextData[1]);
          }
        }
      }
    else // if (this->Orientation == VTK_ORIENT_VERTICAL)
      {
      if (this->TextPosition == vtkScalarBarActor::PrecedeScalarBar)
        {
        this->TitleActor->SetPosition(size[0]/2,
                                      barY + 0.1*titleSize[1]);
        }
      else
        {
        this->TitleActor->SetPosition(size[0]/2,
                                      barHeight + labelSize[1] + 0.1*size[1] + 0.15*titleSize[1]);
        }
      for (i=0; i < this->NumberOfLabels; i++)
        {
        this->TextMappers[i]->GetSize(viewport,sizeTextData);
        this->TextMappers[i]->GetTextProperty()->SetJustificationToCentered();
        if (this->NumberOfLabels > 1)
          {
          val = static_cast<double>(i)/(this->NumberOfLabels-1) * barWidth + barX;
          }
        else
          {
          val = 0.5*(barWidth+barY);
          }
        if (this->TextPosition == vtkScalarBarActor::PrecedeScalarBar)
          {
          this->TextActors[i]->SetPosition(val, size[1] * 0.3);
          }
        else
          {
          this->TextActors[i]->SetPosition(val, barY + barHeight + 0.05*size[1]);
          }
        }
      }

    // Set the texture points
    //
    vtkPoints *texturePoints = vtkPoints::New();
    texturePoints->SetNumberOfPoints(4);
    this->TexturePolyData->SetPoints(texturePoints);
    texturePoints->SetPoint(0, 0.0, 0.0, 0.0);

    double p1[2], p2[2];
    if (this->Orientation == VTK_ORIENT_VERTICAL)
      {
      p1[0] = (this->TextPosition == vtkScalarBarActor::PrecedeScalarBar)
        ? (size[0] - barWidth - barX) : barX;
      p1[1] = barY;
      p2[0] = p1[0] + barWidth;
      p2[1] = p1[1] + barHeight;
      }
    else
      {
      p1[0] = barX;
      p1[1] = (this->TextPosition == vtkScalarBarActor::PrecedeScalarBar)
        ? (size[1] - barHeight - barY) : barY;
      p2[0] = p1[0] + barWidth;
      p2[1] = p1[1] + barHeight;
      }

    texturePoints->SetPoint(0, p1[0], p1[1], 0.0);
    texturePoints->SetPoint(1, p2[0], p1[1], 0.0);
    texturePoints->SetPoint(2, p2[0], p2[1], 0.0);
    texturePoints->SetPoint(3, p1[0], p2[1], 0.0);
    texturePoints->Delete();

    vtkDataArray * tc = this->TexturePolyData->GetPointData()->GetTCoords();
    tc->SetTuple2(1, barWidth / this->TextureGridWidth, 0.0);
    tc->SetTuple2(2, barWidth / this->TextureGridWidth,
                  barHeight / this->TextureGridWidth);
    tc->SetTuple2(3, 0.0, barHeight / this->TextureGridWidth);

    if ( isCategorical )
      {
      // Must reset the color on the leader lines since vtkTextProperty doesn't inherit vtkProperty.
      this->AnnotationLeadersActor->GetProperty()->SetColor( this->LabelTextProperty->GetColor() );
      this->AnnotationLeadersActor->GetProperty()->SetOpacity( this->LabelTextProperty->GetOpacity() );
      // this->ScalarBar will not be drawn; instead, draw padded boxes
      // and leaders to labels for each annotated value.
      // Since labels are user-provided, we render with vtkMathTextActor to allow fancy-ness.
      int numNotes = this->LookupTable->GetNumberOfAnnotatedValues();
      numPts = 4 * numNotes; // 2 triangles per annotation: half-opaque, half-translucent.
      pts = vtkPoints::New();
      pts->SetNumberOfPoints( numPts );
      polys = vtkCellArray::New();
      polys->Allocate( polys->EstimateSize( 2 * numNotes, 3 ) );
      colors = vtkUnsignedCharArray::New();
      colors->SetNumberOfComponents( 4 ); // RGBA
      colors->SetNumberOfTuples( 2 * numNotes );

      this->AnnotationBoxes->Initialize();
      this->AnnotationBoxes->SetPoints(pts);
      this->AnnotationBoxes->SetPolys(polys);
      this->AnnotationBoxes->GetCellData()->SetScalars( colors );
      //this->AnnotationBoxes->SetProperty( this->GetProperty() );
      pts->Delete(); polys->Delete(); colors->Delete();

      // Use the nicely-provided scalar bar position to place
      // the annotated value swatches.
      double swatchPad;
      double swatchC0, swatchC1; // fixed swatch coordinates
      if ( this->Orientation == VTK_ORIENT_VERTICAL )
        {
        barWidth = size[0] - 4 - labelSize[0] - 2 * barX;
        barHeight = static_cast<int>(0.86*size[1]) - barY;
        delta = static_cast<double>(barHeight) / numNotes;
        swatchPad = delta > 16. ? 4. : ( delta / 4. );
        swatchC0 = ( this->TextPosition == vtkScalarBarActor::PrecedeScalarBar )
          ? ( size[0] - barWidth - barX ) : barX;
        swatchC1 = swatchC0 + barWidth;
        this->NumberOfAnnotationLabelsBuilt =
          this->LayoutAnnotationsVertically(
            this->TextPosition == vtkScalarBarActor::PrecedeScalarBar ? swatchC1 : swatchC0,
            barY, barWidth, barHeight, delta, swatchPad );
        for ( i = 0; i < numNotes; ++ i )
          {
          x[0] = swatchC0;
          x[1] = barY + barHeight - i * delta - swatchPad;
          pts->SetPoint( 4 * i, x );
          x[0] = swatchC1;
          pts->SetPoint( 4 * i + 1, x );
          x[1] -= delta - swatchPad * 2;
          pts->SetPoint( 4 * i + 2, x );
          x[0] = swatchC0;
          pts->SetPoint( 4 * i + 3, x );
          }
        }
      else
        {
        barWidth = size[0] - 2 * barX;
        barHeight = static_cast<int>( 0.4 * size[1] ) - barY;
        delta = static_cast<double>(barWidth) / numNotes;
        swatchPad = delta > 16. ? 4. : ( delta / 4. );
        swatchC0 = (this->TextPosition == vtkScalarBarActor::PrecedeScalarBar)
          ? size[1] - barY: barY + barHeight ;
        swatchC1 = (this->TextPosition == vtkScalarBarActor::PrecedeScalarBar)
          ? (size[1]-barHeight - barY) : barY;
        this->NumberOfAnnotationLabelsBuilt =
          this->LayoutAnnotationsHorizontally(
            barX, swatchC0,
            barWidth, barHeight, delta, swatchPad );
        for ( i = 0; i < numNotes; ++ i )
          {
          x[0] = barX + i * delta + swatchPad;
          x[1] = swatchC0;
          pts->SetPoint( 4 * i, x );
          x[0] += delta - swatchPad * 2;
          pts->SetPoint( 4 * i + 1, x );
          x[1] = swatchC1;
          pts->SetPoint( 4 * i + 2, x );
          x[0] -= delta - swatchPad * 2;
          pts->SetPoint( 4 * i + 3, x );
          }
        }
      for ( i = 0; i < numNotes; ++ i )
        {
        ptIds[0] = 4 * i;
        ptIds[1] = ptIds[0] + 1;
        ptIds[2] = ptIds[0] + 2;
        polys->InsertNextCell( 3, ptIds );

        ptIds[1] = ptIds[2];
        ptIds[2] = ptIds[0] + 3;
        polys->InsertNextCell( 3, ptIds );

        // We could just call this->LookupTable->GetTableValue( i % this->LookupTable->GetNumberOfTableValues() ), but
        // that would draw colors even when an annotation did not have a valid conversion to/from a double...
        rgba = this->LookupTable->MapValue( this->LookupTable->GetAnnotatedValue( i ).ToDouble() );
        rgb = colors->GetPointer( /* numComponents */ 4 * /* numCells/swatch */ 2 * /* swatch */ i ); //write into array directly
        rgb[0] = rgba[0]; rgb[1] = rgba[1]; rgb[2] = rgba[2]; rgb[3] = rgba[3];
        rgb[4] = rgba[0]; rgb[5] = rgba[1]; rgb[6] = rgba[2]; rgb[7] = 255; // second triangle is always opaque
        }
      }
    this->BuildTime.Modified();
    }

  // Everything is built, just have to render
  if (this->Title != NULL)
    {
    renderedSomething += this->TitleActor->RenderOpaqueGeometry(viewport);
    }
  // Draw either the scalar bar (non-indexed mode) or the annotated value boxes (indexed mode).
  if ( ! this->LookupTable->GetIndexedLookup() )
    {
    if (this->DrawColorBar)
      {
      this->ScalarBarActor->RenderOpaqueGeometry(viewport);
      }
    for (i=0; i<this->NumberOfLabels; i++)
      {
      renderedSomething += this->TextActors[i]->RenderOpaqueGeometry(viewport);
      }
    }
  else
    {
    if (this->DrawColorBar)
      {
      this->AnnotationBoxesActor->RenderOpaqueGeometry( viewport );
      }
    if ( this->DrawAnnotations )
      {
      if ( this->NumberOfAnnotationLabelsBuilt )
        this->AnnotationLeadersActor->RenderOpaqueGeometry( viewport );
      for ( i = 0; i < this->NumberOfAnnotationLabelsBuilt; ++ i )
        {
        renderedSomething += this->AnnotationLabels[i]->RenderOpaqueGeometry( viewport );
        }
      }
    }

  renderedSomething = (renderedSomething > 0)?(1):(0);

  return renderedSomething;
}

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
int vtkScalarBarActor::HasTranslucentPolygonalGeometry()
{ // TODO: Handle case when IndexedLookup is true and any colors in the palette have an alpha value,
  // as the color swatches drawn by this->AnnotationBoxesActor have 1 translucent triangle for each alpha-swatch.
  return 0;
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
  os << indent << "ComponentTitle: " << (this->ComponentTitle ? this->ComponentTitle : "(none)") << "\n";
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
  os << indent << "UseOpacity: " << this->UseOpacity << "\n";
  if (this->UseOpacity)
    {
    os << indent << "TextureGridWidth: " << this->TextureGridWidth << "\n";
    os << indent << "TextureActor:\n";
    this->TextureActor->PrintSelf(os, indent.GetNextIndent());
    }
  if (this->TextPosition == vtkScalarBarActor::PrecedeScalarBar)
    {
    os << indent << "TextPosition: PrecedeScalarBar\n";
    }
  else
    {
    os << indent << "TextPosition: SucceedScalarBar\n";
    }

  os << indent << "MaximumWidthInPixels: "
     << this->MaximumWidthInPixels << endl;
  os << indent << "MaximumHeightInPixels: "
     << this->MaximumHeightInPixels << endl;

  os << indent << "DrawAnnotations: "
    << this->DrawAnnotations << endl;
  os << indent << "AnnotationLeaderPadding: "
    << this->AnnotationLeaderPadding << endl;

  os << indent << "DrawBackground: " << this->DrawBackground << "\n";
  os << indent << "Background Property:\n";
  this->BackgroundProperty->PrintSelf(os,indent.GetNextIndent());
  os << indent << "DrawFrame: " << this->DrawFrame << "\n";
  os << indent << "Frame Property:\n";
  this->FrameProperty->PrintSelf(os,indent.GetNextIndent());
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
    this->SetDrawBackground(a->GetDrawBackground());
    this->SetBackgroundProperty(a->GetBackgroundProperty());
    this->SetDrawFrame(a->GetDrawFrame());
    this->SetFrameProperty(a->GetFrameProperty());
    }

  // Now do superclass
  this->vtkActor2D::ShallowCopy(prop);
}

//----------------------------------------------------------------------------
void vtkScalarBarActor::AllocateAndSizeLabels(int *labelSize,
                                              int *size,
                                              vtkViewport *viewport,
                                              double *range)
{
  labelSize[0] = labelSize[1] = 0;

  this->TextMappers = new vtkTextMapper * [this->NumberOfLabels];
  this->TextActors = new vtkActor2D * [this->NumberOfLabels];

  char string[512];

  double val;
  int i;

  // TODO: this should be optimized, maybe by keeping a list of
  // allocated mappers, in order to avoid creation/destruction of
  // their underlying text properties (i.e. each time a mapper is
  // created, text properties are created and shallow-assigned a font size
  // which value might be "far" from the target font size).

  // Does this map have its scale set to log?
  int isLogTable = this->LookupTable->UsingLogScale();

  for (i=0; i < this->NumberOfLabels; i++)
    {
    this->TextMappers[i] = vtkTextMapper::New();

    if ( isLogTable )
      {
      double lval;
      if (this->NumberOfLabels > 1)
        {
        lval = log10(range[0]) +
          static_cast<double>(i)/(this->NumberOfLabels-1) *
          (log10(range[1])-log10(range[0]));
        }
      else
        {
        lval = log10(range[0]) + 0.5*(log10(range[1])-log10(range[0]));
        }
      val = pow(10.0,lval);
      }
    else
      {
      if (this->NumberOfLabels > 1)
        {
        val = range[0] +
          static_cast<double>(i)/(this->NumberOfLabels-1)
          * (range[1]-range[0]);
        }
      else
        {
        val = range[0] + 0.5*(range[1]-range[0]);
        }
      }

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
      targetWidth = static_cast<int>(0.6*size[0]);
      targetHeight = static_cast<int>(0.86*size[1]/this->NumberOfLabels);
      }
    else
      {
      targetWidth = static_cast<int>(size[0]*0.8/this->NumberOfLabels);
      targetHeight = static_cast<int>(0.25*size[1]);
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
  if ( this->Orientation == VTK_ORIENT_VERTICAL )
    {
    targetWidth = static_cast<int>(0.9*size[0]);
    targetHeight = static_cast<int>(0.1*size[1]);
    }
  else
    {
    targetWidth = size[0];
    targetHeight = static_cast<int>(0.25*size[1]);
    }

  this->TitleMapper->SetConstrainedFontSize(
    viewport, targetWidth, targetHeight);

  this->TitleMapper->GetSize(viewport, titleSize);
}

int vtkScalarBarActor::AllocateAndSizeAnnotationLabels( vtkScalarsToColors* lkup )
{
  int numNotes = lkup->GetNumberOfAnnotatedValues();
  this->AnnotationLabels = new vtkMathTextActor*[ numNotes ];
  for ( int i = 0; i < numNotes; ++ i )
    {
    this->AnnotationLabels[i] = vtkMathTextActor::New();
    this->AnnotationLabels[i]->GetTextProperty()->ShallowCopy( this->LabelTextProperty );
    this->AnnotationLabels[i]->SetProperty( this->GetProperty() );
    // NB: If passed an empty string, pass a single space to the renderer;
    // empty strings get rendered as blobs which is Highly Undesirable.
    vtkStdString label = lkup->GetAnnotation( i );
    this->AnnotationLabels[i]->SetInput( label.empty() ? " " : label.c_str() );
    this->AnnotationLabels[i]->GetPositionCoordinate()->
      SetReferenceCoordinate( this->PositionCoordinate );
    }

  return numNotes;
}

int vtkScalarBarActor::LayoutAnnotationsVertically(
  double barX, double barY, double vtkNotUsed(barWidth), double barHeight, double delta, double pad )
{
  if ( ! this->LookupTable || this->LookupTable->GetNumberOfAnnotatedValues() <= 0 )
    {
    return 0;
    }

#define VTK_ANN_VLAYOUT(j,dir,delt) \
    ctr = barY + delta * ( j + 0.5 ); \
    ll[0] = lpts->InsertNextPoint( xl0, ctr, 0. ); \
    this->AnnotationLabels[numNotes - j - 1]->GetBoundingBox( bds ); \
    hh = ( bds[3] - bds[2] + pad ) / 2.; /* label half-height, including padding */ \
    if ( ( dir < 0 && ctr + hh > dnCum ) || ( dir > 0 && ctr - hh < upCum ) ) \
      ctr = delt + dir * hh; \
    this->AnnotationLabels[numNotes - j - 1]->GetTextProperty()->SetJustification( \
      this->TextPosition == PrecedeScalarBar ? VTK_TEXT_LEFT : VTK_TEXT_RIGHT ); \
    this->AnnotationLabels[numNotes - j - 1]->GetTextProperty()->SetVerticalJustification( VTK_TEXT_CENTERED ); \
    this->AnnotationLabels[numNotes - j - 1]->SetPosition( \
      barX + ( this->TextPosition == PrecedeScalarBar ? +1 : -1 ) * ( pad + this->AnnotationLeaderPadding ), \
      ctr ); \
    ll[1] = lpts->InsertNextPoint( xl1, ctr, 0. ); \
    if ( ! this->LookupTable->GetAnnotation( numNotes - j - 1 ).empty() ) \
      { \
      llines->InsertNextCell( 2, ll ); \
      } \
    delt = ( dir <= 0 ? ctr - hh : ctr + hh );

  int numNotes = this->AllocateAndSizeAnnotationLabels( this->LookupTable );
  vtkPoints* lpts = vtkPoints::New();
  vtkCellArray* llines = vtkCellArray::New();
  lpts->Allocate( 2 * numNotes );
  llines->Allocate( llines->EstimateSize( numNotes, 2 ) );
  this->AnnotationLeaders->Initialize();
  this->AnnotationLeaders->SetPoints( lpts );
  this->AnnotationLeaders->SetLines( llines );
  lpts->Delete(); llines->Delete();

  // Start at the center and move outward (both up and down), accumulating label heights as we go.
  int ic = numNotes / 2;
  int dn, up;
  double dnCum, upCum, ctr, hh;
  double bds[4];
  // leader-line endpoint x-coordinates:
  double xl0 = barX + ( this->TextPosition == PrecedeScalarBar ? +1 : -1 ) * pad / 2.;
  double xl1 = barX + ( this->TextPosition == PrecedeScalarBar ? +1 : -1 ) * ( pad / 2. + this->AnnotationLeaderPadding );
  vtkIdType ll[2]; // leader-line endpoint IDs
  if ( 2 * ic == numNotes )
    {
    dn = ic - 1;
    up = ic;
    dnCum = upCum = barY + barHeight / 2.;
    }
  else
    {
    dn = ic - 1;
    up = ic + 1;
    ctr = barY + delta * ( ic + 0.5 );
    VTK_ANN_VLAYOUT(ic,0,dnCum);
    upCum = ctr + hh;
    }
  for ( ; dn >= 0; -- dn, ++ up )
    {
    VTK_ANN_VLAYOUT(dn,-1,dnCum);
    VTK_ANN_VLAYOUT(up,+1,upCum);
    }
  return numNotes;
}

struct vtkScalarBarHLabelInfo
{
  double X[2]; // padded left-right label bounds
  double Y[2]; // padded top-bottom label bounds
  int Justification;
  double Anchor[2]; // x-y coordinates of anchor point
};

// A non-overlapping label placer for a horizontal array of annotated swatches.
// When space is tight, It displaces labels vertically and uses broken leader lines to relate labels back to swatches.
struct vtkScalarBarHLabelPlacer
{
  std::vector<vtkScalarBarHLabelInfo> Places;
  unsigned Ctr;
  double Y0;
  double XBounds[2];
  double Delta;
  double Pad;
  double LeaderPad;
  double Dir; // displacement direction (either +1 or -1)
  bool HaveCtr; // Is there a label at the dead center? (i.e., is Places.size() odd?)

  vtkScalarBarHLabelPlacer(
    unsigned n, double y0, double dir, double xmin, double xmax,
    double delta, double pad, double leaderPad )
    : Places( n ), Ctr( n % 2 ? n / 2 : n / 2 - 1 ), Y0( y0 ), Delta( delta ),
      Pad( pad ), LeaderPad( leaderPad ), Dir( dir < 0 ? -1. : +1. ), HaveCtr( n % 2 ? true : false )
    {
    this->XBounds[0] = xmin;
    this->XBounds[1] = xmax;
    }

  void Place( unsigned i, double wd, double ht )
    {
    vtkScalarBarHLabelInfo& placement( this->Places[i] );
    unsigned farLo, farHi;
    int medNeighbor;
    int posRelToCenter = ( i == this->Ctr && this->HaveCtr ) ? 0 : ( i > this->Ctr ? +1 : -1 );

    if ( posRelToCenter == 0 )
      { // center label
      double xbar = ( this->XBounds[0] + this->XBounds[1] ) / 2.;
      placement.Y[0] = this->Y0 + this->Dir * ( this->LeaderPad + this->Pad );
      placement.Y[1] = placement.Y[0] + this->Dir * ht; // Note un-padded bounds on distal y axis! Required below.
      placement.X[0] = xbar - wd / 2. - this->Pad;
      placement.X[1] = xbar + wd / 2. + this->Pad;
      placement.Justification = VTK_TEXT_CENTERED;
      placement.Anchor[0] = xbar;
      placement.Anchor[1] = placement.Y[0]; // Vertical justification changes, but Y[0] is always anchor
      }
    else // placing *a lateral* (as opposed to *the medial*) label.
      {
      // First: Horizontal placement. Check immediate medial neighbor to see if placement can occur without more displacement.
      double spotMax = this->XBounds[0] + i * this->Delta;
      bool needToDisplace = false;
      if ( posRelToCenter == +1 )
        { // label is right-justified; placement.X[1] bounded from above by XBounds[1] or right neighbor swatch
        farLo = 2 * this->Ctr + (this->HaveCtr ? 0 : 1) - i; // Furthest label we have placed so far.
        farHi = this->Ctr; // The closest label we might overlap is the center label.
        medNeighbor = i - 1;
        placement.Justification = VTK_TEXT_RIGHT;
        spotMax += this->Delta - this->Pad;
        if ( spotMax > this->XBounds[1] ) spotMax = this->XBounds[1];
        if ( spotMax - wd < this->Places[medNeighbor].X[1] )
          { // We must displace; put the label where it makes sense: bounded on right by swatch edge.
          needToDisplace = true;
          placement.X[1] = this->XBounds[0] + ( i + 1 ) * this->Delta; // NB: Padding included
          placement.X[0] = placement.X[1] - wd - 2 * this->Pad;
          placement.Anchor[0] = placement.X[1] - this->Pad;
          }
        else
          { // There is space for the label without vertical displacement.
          placement.Justification = VTK_TEXT_CENTERED;
          placement.Anchor[0] = this->XBounds[0] + ( i + 0.5 ) * this->Delta;
          if ( placement.Anchor[0] - wd / 2. < this->Places[medNeighbor].X[1] ||
            placement.Anchor[0] + wd / 2. > this->XBounds[1] )
            { // We can't center on the swatch; right justify it to swatch.
            placement.Anchor[0] += this->Delta / 2. - this->Pad;
            placement.Justification = VTK_TEXT_RIGHT;
            }
          placement.X[0] = placement.Justification == VTK_TEXT_CENTERED ?
            ( placement.Anchor[0] - wd / 2. ) : ( placement.Anchor[0] - wd );
          }
        }
      else // posRelToCenter == -1
        { // label is left-justified; placement.X[0] bounded from below by XBounds[0] or left neighbor swatch
        farLo = this->Ctr + (this->HaveCtr ? 0 : 1); // The center label is the closest label we might overlap
        farHi = 2 * this->Ctr - i - (this->HaveCtr ? 1 : 0); // The furthest label to the right we have placed so far.
        medNeighbor = i + 1;
        if ( ! this->HaveCtr && medNeighbor >= static_cast<int>( farHi ) ) medNeighbor = -1;
        placement.Justification = VTK_TEXT_LEFT;
        spotMax += this->Pad;
        if ( spotMax < this->XBounds[0] ) spotMax = this->XBounds[0];
        if ( medNeighbor >= 0 && spotMax + wd > this->Places[medNeighbor].X[0] )
          { // we must displace; put the label where it makes sense: bounded on left by swatch edge.
          needToDisplace = true;
          placement.X[0] = this->XBounds[0] + i * this->Delta; // NB: Padding included
          placement.X[1] = placement.X[0] + wd + 2 * this->Pad;
          placement.Anchor[0] = placement.X[0] + this->Pad;
          }
        else
          { // There is space for the label without vertical displacement.
          placement.Justification = VTK_TEXT_CENTERED;
          placement.Anchor[0] = this->XBounds[0] + ( i + 0.5 ) * this->Delta;
          if ( ( medNeighbor >= 0 && placement.Anchor[0] + wd / 2. > this->Places[medNeighbor].X[0] ) ||
            placement.Anchor[0] - wd / 2. < this->XBounds[0] )
            { // We can't center on the swatch; left-justify it to swatch.
            placement.Anchor[0] -= this->Delta / 2. - this->Pad;
            placement.Justification = VTK_TEXT_LEFT;
            }
          placement.X[0] = placement.Justification == VTK_TEXT_CENTERED ?
            ( placement.Anchor[0] - wd / 2. ) : ( placement.Anchor[0] - this->Pad );
          }
        }
      // Second: Vertical placement. Displace label to avoid overlap if need be.
      if ( ! needToDisplace )
        {
        placement.Y[0] = ( medNeighbor >= 0 ?
          this->Places[medNeighbor].Y[0] :
          this->Y0 + this->Dir * ( this->LeaderPad + this->Pad ) );
        placement.Y[1] = placement.Y[0] + this->Dir * ht;
        placement.X[1] = placement.X[0] + wd;
        placement.Anchor[1] = placement.Y[0];
        }
      else
        { // must displace... find out by how much
        // I. At least as much as immediate medial neighbor
        placement.Y[0] = this->Places[medNeighbor].Y[1] + this->Dir * this->Pad;
        for ( unsigned j = farLo; j <= farHi; ++ j )
          {
          // II. Check whether label has any y overlap && any x overlap.
          // There are 2 cases: one for labels above swatches, the other
          // for labels below swatches.
          if (
            ( this->Dir < 0 && placement.Y[0] > this->Places[j].Y[1] &&
              ( i > j ?
                placement.X[0] <= this->Places[j].X[1] :
                placement.X[1] >= this->Places[j].X[0] ) ) ||
            ( this->Dir > 0 && placement.Y[0] < this->Places[j].Y[1] &&
              ( i > j ?
                placement.X[0] <= this->Places[j].X[1] :
                placement.X[1] >= this->Places[j].X[0] ) ) )
            {
            placement.Y[0] = this->Places[j].Y[1] + this->Dir * this->Pad;
            }
          }
        placement.Y[1] = placement.Y[0] + this->Dir * ht;
        placement.Anchor[1] = placement.Y[0]; // Vertical justification changes, but Y[0] is always anchor
        }
      }
    }

#define VTK_ANN_HLEADER(j) \
  { \
    vtkScalarBarHLabelInfo& other( this->Places[j] ); \
    if ( label.Anchor[0] > other.X[0] && label.Anchor[0] < other.X[1] ) \
      { \
      pt = pts->InsertNextPoint( label.Anchor[0], other.Y[0], 0. ); \
      lines->InsertCellPoint( pt ); \
      lines->InsertNextCell( 2 ); \
      curY = other.Y[1]; \
      pt = pts->InsertNextPoint( label.Anchor[0], curY, 0. ); \
      lines->InsertCellPoint( pt ); \
      } \
  }

  // Only called after all labels are placed
  void AddBrokenLeader( int i, vtkPoints* pts, vtkCellArray* lines )
    {
    vtkIdType pt;
    vtkScalarBarHLabelInfo& label( this->Places[i] );
    // I. Insert first vertex near swatch:
    lines->InsertNextCell( 2 );
    double curY = this->Y0 + this->Dir * this->Pad / 2.;
    pt = pts->InsertNextPoint( label.Anchor[0], curY, 0. );
    lines->InsertCellPoint( pt );
    // II. Loop over all labels checking for interference. Where found, close current line and start new one on the other side.
    int ic = this->Ctr;
    int lf, rt;
    bool done = false;
    if ( 2 * ic == static_cast<int>( this->Places.size() ) )
      {
      lf = ic - 1;
      rt = ic;
      }
    else
      {
      lf = ic - 1;
      rt = ic + 1;
      if ( i == ic )
        {
        done = true;
        }
      else
        {
        VTK_ANN_HLEADER(ic);
        }
      }
    if ( ! done )
      {
      for ( ; lf >= 0; -- lf, ++ rt )
        {
        if ( lf == i ) break;
        VTK_ANN_HLEADER(lf);
        if ( rt == i ) break;
        VTK_ANN_HLEADER(rt);
        }
      }

    // III. Finally, close the open line segment with the label anchor point.
    pt = pts->InsertNextPoint( label.Anchor[0], label.Anchor[1] - this->Dir * this->Pad / 2., 0. );
    lines->InsertCellPoint( pt );
    }
};

/**\brief A non-overlapping label placer for a horizontal array of annotated swatches.
 *
 * A set of rules are enforced during layout:
 * <ul>
 * <li> Any label may be wider than the entire legend.
 * <li> The center label should be centered on the center swatch
 * <li> No other label should extend beyond the legend's matching lateral extent (i.e., a label to the left of center
 * should never extend beyond the left bounds of its swatch).
 * <li> To enforce this, labels may be displaced vertically (distally) away from the legend.
 * <li> Broken leaders should be drawn connecting each displaced label to its swatch, with breaks where
 * long labels from the centerline or beyond obstruct it.
 * </ul>
 *
 * The algorithm for performing the layout enforces these rules as follows:
 * Labels are placed starting with the central (medial) label and moving outwards;
 * this provides a consistent placement as the actor is resized.
 * First the horizontal label position is determined by examining the width of the label and the extents
 * of its medial neighbor (which will have been placed already).
 * The vertical displacement is then computed by either copying the medial neighbor's displacement (if
 * no interference with the neighbor was required) or incrementing the displacement beyond its immediate
 * neighbor and checking all other relevant labels for intereference.
 */
int vtkScalarBarActor::LayoutAnnotationsHorizontally(
  double barX, double barY, double barWidth, double barHeight, double delta, double pad )
{
  if ( ! this->LookupTable || this->LookupTable->GetNumberOfAnnotatedValues() <= 0 )
    {
    return 0;
    }

#define VTK_ANN_HLAYOUT(j,placer) \
  this->AnnotationLabels[j]->GetTextProperty()->SetJustification( placer.Places[j].Justification ); \
  this->AnnotationLabels[j]->GetTextProperty()->SetVerticalJustification( placer.Dir > 0 ? VTK_TEXT_BOTTOM : VTK_TEXT_TOP ); \
  this->AnnotationLabels[j]->SetPosition( placer.Places[j].Anchor ); \
  if ( ! this->LookupTable->GetAnnotation( j ).empty() ) \
    { \
    placer.AddBrokenLeader( j, lpts, llines ); \
    }

  int numNotes = this->AllocateAndSizeAnnotationLabels( this->LookupTable );
  bool precede = this->TextPosition == vtkScalarBarActor::PrecedeScalarBar;
  vtkScalarBarHLabelPlacer placer(
    numNotes, precede ? barY : barY - barHeight, precede ? +1 : -1,
    barX, barX + barWidth, delta, pad, this->AnnotationLeaderPadding );

  vtkPoints* lpts = vtkPoints::New();
  vtkCellArray* llines = vtkCellArray::New();
  lpts->Allocate( numNotes * numNotes ); // TODO: Improve estimates, but we don't know how many breaks there will be.
  llines->Allocate( llines->EstimateSize( numNotes * numNotes, 2 ) );
  this->AnnotationLeaders->Initialize();
  this->AnnotationLeaders->SetPoints( lpts );
  this->AnnotationLeaders->SetLines( llines );
  lpts->Delete(); llines->Delete();

  // Start at the center and move outward (both up and down), accumulating label displacement as we go.
  int ic = numNotes / 2;
  int lf, rt;
  double bds[4];
  if ( 2 * ic == numNotes )
    {
    lf = ic - 1;
    rt = ic;
    }
  else
    {
    lf = ic - 1;
    rt = ic + 1;
    this->AnnotationLabels[ic]->GetBoundingBox( bds );
    placer.Place( ic, bds[1] - bds[0], bds[3] - bds[2] );
    VTK_ANN_HLAYOUT(ic,placer);
    }
  for ( ; lf >= 0; -- lf, ++ rt )
    {
    this->AnnotationLabels[lf]->GetBoundingBox( bds );
    placer.Place( lf, bds[1] - bds[0], bds[3] - bds[2] );
    VTK_ANN_HLAYOUT(lf,placer);
    this->AnnotationLabels[rt]->GetBoundingBox( bds );
    placer.Place( rt, bds[1] - bds[0], bds[3] - bds[2] );
    VTK_ANN_HLAYOUT(rt,placer);
    }
  return numNotes;
}
