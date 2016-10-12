/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCornerAnnotation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkCornerAnnotation.h"

#include "vtkAlgorithmOutput.h"
#include "vtkImageActor.h"
#include "vtkImageData.h"
#include "vtkImageMapper3D.h"
#include "vtkImageMapToWindowLevelColors.h"
#include "vtkObjectFactory.h"
#include "vtkPropCollection.h"
#include "vtkTextMapper.h"
#include "vtkTextProperty.h"
#include "vtkViewport.h"
#include "vtkWindow.h"

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkCornerAnnotation);

vtkSetObjectImplementationMacro(vtkCornerAnnotation,ImageActor,vtkImageActor);
vtkSetObjectImplementationMacro(vtkCornerAnnotation,WindowLevel,
                                vtkImageMapToWindowLevelColors);
vtkCxxSetObjectMacro(vtkCornerAnnotation,TextProperty,vtkTextProperty);

//----------------------------------------------------------------------------
vtkCornerAnnotation::vtkCornerAnnotation()
{
  this->PositionCoordinate->SetCoordinateSystemToNormalizedViewport();
  this->PositionCoordinate->SetValue(0.2,0.85);

  this->LastSize[0] = 0;
  this->LastSize[1] = 0;

  this->MaximumLineHeight = 1.0;
  this->MinimumFontSize = 6;
  this->MaximumFontSize = 200;
  this->LinearFontScaleFactor    = 5.0;
  this->NonlinearFontScaleFactor = 0.35;
  this->FontSize = 15;

  this->TextProperty = vtkTextProperty::New();
  this->TextProperty->ShadowOff();

  for (int i = 0; i < NumTextPositions; i++)
  {
    this->CornerText[i] = NULL;
    this->TextMapper[i] = vtkTextMapper::New();
    this->TextActor[i] = vtkActor2D::New();
    this->TextActor[i]->SetMapper(this->TextMapper[i]);
  }

  this->ImageActor = NULL;
  this->LastImageActor = 0;
  this->WindowLevel = NULL;

  this->LevelShift = 0;
  this->LevelScale = 1;

  this->ShowSliceAndImage = 1;
}

//----------------------------------------------------------------------------
vtkCornerAnnotation::~vtkCornerAnnotation()
{
  this->SetTextProperty(NULL);

  for (int i = 0; i < NumTextPositions; i++)
  {
    delete [] this->CornerText[i];
    this->TextMapper[i]->Delete();
    this->TextActor[i]->Delete();
  }

  this->SetWindowLevel(NULL);
  this->SetImageActor(NULL);
}

//----------------------------------------------------------------------------
// Release any graphics resources that are being consumed by this actor.
// The parameter window could be used to determine which graphic
// resources to release.
void vtkCornerAnnotation::ReleaseGraphicsResources(vtkWindow *win)
{
  this->Superclass::ReleaseGraphicsResources(win);
  for (int i = 0; i < NumTextPositions; i++)
  {
    this->TextActor[i]->ReleaseGraphicsResources(win);
  }
}

//----------------------------------------------------------------------------
void vtkCornerAnnotation::TextReplace(vtkImageActor *ia,
                                      vtkImageMapToWindowLevelColors *wl)
{
  int i;
  char *text, *text2;
  int slice = 0, slice_max = 0;
  char *rpos, *tmp;
  double window = 0, level = 0;
  long int windowi = 0, leveli = 0;
  vtkImageData *wl_input = NULL, *ia_input = NULL;
  int input_type_is_float = 0;

  if (wl)
  {
    window = wl->GetWindow();
    window *= this->LevelScale;
    level = wl->GetLevel();
    level = level * this->LevelScale + this->LevelShift;
    windowi = (long int)window;
    leveli = (long int)level;
    wl_input = vtkImageData::SafeDownCast(wl->GetInput());
    if (wl_input)
    {
      input_type_is_float = (wl_input->GetScalarType() == VTK_FLOAT ||
                             wl_input->GetScalarType() == VTK_DOUBLE);
    }
  }
  if (ia)
  {
    slice = ia->GetSliceNumber() - ia->GetSliceNumberMin() + 1;
    slice_max = ia->GetSliceNumberMax() - ia->GetSliceNumberMin() + 1;
    ia_input = ia->GetInput();
    if (!wl_input && ia_input)
    {
      input_type_is_float = (ia_input->GetScalarType() == VTK_FLOAT ||
                             ia_input->GetScalarType() == VTK_DOUBLE);
    }
  }


  // search for tokens, replace and then assign to TextMappers
  for (i = 0; i < NumTextPositions; i++)
  {
    if (this->CornerText[i] && strlen(this->CornerText[i]))
    {
      text = new char [strlen(this->CornerText[i])+1000];
      text2 = new char [strlen(this->CornerText[i])+1000];
      strcpy(text,this->CornerText[i]);

      // now do the replacements

      rpos = strstr(text,"<image>");
      while (rpos)
      {
        *rpos = '\0';
        if (ia && this->ShowSliceAndImage)
        {
          sprintf(text2,"%sImage: %i%s",text,slice,rpos+7);
        }
        else
        {
          sprintf(text2,"%s%s",text,rpos+7);
        }
        tmp = text;
        text = text2;
        text2 = tmp;
        rpos = strstr(text,"<image>");
      }

      rpos = strstr(text,"<image_and_max>");
      while (rpos)
      {
        *rpos = '\0';
        if (ia && this->ShowSliceAndImage)
        {
          sprintf(text2,"%sImage: %i / %i%s",text,slice,slice_max,rpos+15);
        }
        else
        {
          sprintf(text2,"%s%s",text,rpos+15);
        }
        tmp = text;
        text = text2;
        text2 = tmp;
        rpos = strstr(text,"<image_and_max>");
      }

      rpos = strstr(text,"<slice>");
      while (rpos)
      {
        *rpos = '\0';
        if (ia && this->ShowSliceAndImage)
        {
          sprintf(text2,"%sSlice: %i%s",text,slice,rpos+7);
        }
        else
        {
          sprintf(text2,"%s%s",text,rpos+7);
        }
        tmp = text;
        text = text2;
        text2 = tmp;
        rpos = strstr(text,"<slice>");
      }

      rpos = strstr(text,"<slice_and_max>");
      while (rpos)
      {
        *rpos = '\0';
        if (ia && this->ShowSliceAndImage)
        {
          sprintf(text2,"%sSlice: %i / %i%s",text,slice,slice_max,rpos+15);
        }
        else
        {
          sprintf(text2,"%s%s",text,rpos+15);
        }
        tmp = text;
        text = text2;
        text2 = tmp;
        rpos = strstr(text,"<slice_and_max>");
      }

      rpos = strstr(text,"<slice_pos>");
      while (rpos)
      {
        *rpos = '\0';
        if (ia && this->ShowSliceAndImage)
        {
          double *dbounds = ia->GetDisplayBounds();
          int *dext = ia->GetDisplayExtent();
          double pos;
          if (dext[0] == dext[1])
          {
            pos = dbounds[0];
          }
          else if (dext[2] == dext[3])
          {
            pos = dbounds[2];
          }
          else
          {
            pos = dbounds[4];
          }
          sprintf(text2,"%s%g%s",text,pos,rpos+11);
        }
        else
        {
          sprintf(text2,"%s%s",text,rpos+11);
        }
        tmp = text;
        text = text2;
        text2 = tmp;
        rpos = strstr(text,"<slice_pos>");
      }

      rpos = strstr(text,"<window>");
      while (rpos)
      {
        *rpos = '\0';
        if (wl)
        {
          if (input_type_is_float)
          {
            sprintf(text2,"%sWindow: %g%s",text,window,rpos+8);
          }
          else
          {
            sprintf(text2,"%sWindow: %li%s",text,windowi,rpos+8);
          }
        }
        else
        {
          sprintf(text2,"%s%s",text,rpos+8);
        }
        tmp = text;
        text = text2;
        text2 = tmp;
        rpos = strstr(text,"<window>");
      }

      rpos = strstr(text,"<level>");
      while (rpos)
      {
        *rpos = '\0';
        if (wl)
        {
          if (input_type_is_float)
          {
            sprintf(text2,"%sLevel: %g%s",text,level,rpos+7);
          }
          else
          {
            sprintf(text2,"%sLevel: %li%s",text,leveli,rpos+7);
          }
        }
        else
        {
          sprintf(text2,"%s%s",text,rpos+7);
        }
        tmp = text;
        text = text2;
        text2 = tmp;
        rpos = strstr(text,"<level>");
      }

      rpos = strstr(text,"<window_level>");
      while (rpos)
      {
        *rpos = '\0';
        if (wl)
        {
          if (input_type_is_float)
          {
            sprintf(text2,"%sWW/WL: %g / %g%s",text,window,level,rpos+14);
          }
          else
          {
            sprintf(text2,"%sWW/WL: %li / %li%s",text,windowi,leveli,rpos+14);
          }
        }
        else
        {
          sprintf(text2,"%s%s",text,rpos+14);
        }
        tmp = text;
        text = text2;
        text2 = tmp;
        rpos = strstr(text,"<window_level>");
      }

      this->TextMapper[i]->SetInput(text);
      delete [] text;
      delete [] text2;
    }
    else
    {
      this->TextMapper[i]->SetInput("");
    }
  }
}

//----------------------------------------------------------------------------
int vtkCornerAnnotation::RenderOverlay(vtkViewport *viewport)
{
  // Everything is built, just have to render
  // only render if font is at least minimum font
  if (this->FontSize >= this->MinimumFontSize)
  {
    for (int i = 0; i < NumTextPositions; i++)
    {
      this->TextActor[i]->RenderOverlay(viewport);
    }
  }
  return 1;
}

namespace {
// Ported from old vtkTextMapper implementation
int GetNumberOfLines(const char *str)
{
  if (str == NULL || *str == '\0')
  {
    return 0;
  }

  int result = 1;
  while (str != NULL)
  {
    if ((str = strstr(str, "\n")) != NULL)
    {
      result++;
      str++; // Skip '\n'
    }
  }
  return result;
}
}

//----------------------------------------------------------------------------
int vtkCornerAnnotation::RenderOpaqueGeometry(vtkViewport *viewport)
{
  int fontSize;

  // Check to see whether we have to rebuild everything
  // If the viewport has changed we may - or may not need
  // to rebuild, it depends on if the projected coords chage
  int viewport_size_has_changed = 0;
  if (viewport->GetMTime() > this->BuildTime ||
      (viewport->GetVTKWindow() &&
       viewport->GetVTKWindow()->GetMTime() > this->BuildTime))
  {
    int *vSize = viewport->GetSize();
    if (this->LastSize[0] != vSize[0] || this->LastSize[1] != vSize[1])
    {
      viewport_size_has_changed = 1;
    }
  }

  // Is there an image actor ?
  vtkImageMapToWindowLevelColors *wl = this->WindowLevel;
  vtkImageActor *ia = NULL;
  if (this->ImageActor)
  {
    ia = this->ImageActor;
  }
  else
  {
    vtkPropCollection *pc = viewport->GetViewProps();
    int numProps = pc->GetNumberOfItems();
    for (int i = 0; i < numProps; i++)
    {
      ia = vtkImageActor::SafeDownCast(pc->GetItemAsObject(i));
      if (ia)
      {
        if (ia->GetInput() && !wl)
        {
          wl = vtkImageMapToWindowLevelColors::SafeDownCast(
            ia->GetMapper()->GetInputAlgorithm());
        }
        break;
      }
    }
  }

  int tprop_has_changed = (this->TextProperty &&
                           this->TextProperty->GetMTime() > this->BuildTime);

  // Check to see whether we have to rebuild everything
  if (viewport_size_has_changed ||
      tprop_has_changed ||
      (this->GetMTime() > this->BuildTime) ||
      (ia && (ia != this->LastImageActor ||
              ia->GetMTime() > this->BuildTime)) ||
      (wl && wl->GetMTime() > this->BuildTime))
  {
    int *vSize = viewport->GetSize();

    vtkDebugMacro(<<"Rebuilding text");

    // Replace text
    this->TextReplace(ia, wl);

    // Get the viewport size in display coordinates
    this->LastSize[0] = vSize[0];
    this->LastSize[1] = vSize[1];

    // Only adjust size then the text changes due to non w/l slice reasons
    if (viewport_size_has_changed ||
        tprop_has_changed ||
        this->GetMTime() > this->BuildTime)
    {
      // Rebuid text props.
      // Perform shallow copy here since each individual corner has a
      // different aligment/size but they share the other this->TextProperty
      // attributes.
      fontSize = this->TextMapper[0]->GetTextProperty()->GetFontSize();

      if (tprop_has_changed)
      {
        for (int i = 0; i < NumTextPositions; i++)
        {
          vtkTextProperty *tprop = this->TextMapper[i]->GetTextProperty();
          tprop->ShallowCopy(this->TextProperty);
          tprop->SetFontSize(fontSize);
        }
        this->SetTextActorsJustification();
      }

      // Update all the composing objects to find the best size for the font
      // use the last size as a first guess

      /*
          +---------+
          |2   7   3|
          |         |
          |6       5|
          |         |
          |0   4   1|
          +---------+
      */

      int tempi[2*NumTextPositions];
      int allZeros = 1;
      for (int i = 0; i < NumTextPositions; i++)
      {
        this->TextMapper[i]->GetSize(viewport, tempi + i * 2);
        if (tempi[2*i] > 0 || tempi[2*i+1] > 0)
        {
          allZeros = 0;
        }
      }

      if (allZeros)
      {
        return 0;
      }

      int height_02 = tempi[1] + tempi[5];  // total height of text in left top/bottom corners
      int height_13 = tempi[3] + tempi[7];  // total height of text in right top/bottom corners
      int height_47 = tempi[9] + tempi[15]; // total height of text at center of top/bottom edges

      int width_01 = tempi[0] + tempi[2];   // total width of text on botttom left/right corners
      int width_23 = tempi[4] + tempi[6];   // total width of text on top left/right corners
      int width_56 = tempi[10] + tempi[12]; // total width of text at center of left/right edges

      int max_width_corners = (width_01 > width_23) ? width_01 : width_23;
      int max_width         = (width_56 > max_width_corners) ? width_56 : max_width_corners;

      int num_lines_02 = GetNumberOfLines(this->TextMapper[0]->GetInput())
          + GetNumberOfLines(this->TextMapper[2]->GetInput());

      int num_lines_13 = GetNumberOfLines(this->TextMapper[1]->GetInput())
          + GetNumberOfLines(this->TextMapper[3]->GetInput());

      int num_lines_47 = GetNumberOfLines(this->TextMapper[4]->GetInput())
          + GetNumberOfLines(this->TextMapper[7]->GetInput());

      int line_max_02 = (int)(vSize[1] * this->MaximumLineHeight) *
        (num_lines_02 ? num_lines_02 : 1);

      int line_max_13 = (int)(vSize[1] * this->MaximumLineHeight) *
        (num_lines_13 ? num_lines_13 : 1);

      int line_max_47 = (int)(vSize[1] * this->MaximumLineHeight) *
        (num_lines_47 ? num_lines_47 : 1);

      // Target size is to use 90% of x and y

      int tSize[2];
      tSize[0] = (int)(0.9*vSize[0]);
      tSize[1] = (int)(0.9*vSize[1]);

      // While the size is too small increase it

      while (height_02 < tSize[1] &&
             height_13 < tSize[1] &&
             height_47 < tSize[1] &&
             max_width < tSize[0] &&
             height_02 < line_max_02 &&
             height_13 < line_max_13 &&
             height_47 < line_max_47 &&
             fontSize < 100)
      {
        fontSize++;
        for (int i = 0; i < NumTextPositions; i++)
        {
          this->TextMapper[i]->GetTextProperty()->SetFontSize(fontSize);
          this->TextMapper[i]->GetSize(viewport, tempi + i * 2);
        }
        height_02 = tempi[1] + tempi[5];
        height_13 = tempi[3] + tempi[7];
        height_47 = tempi[9] + tempi[15];

        width_01 = tempi[0] + tempi[2];
        width_23 = tempi[4] + tempi[6];
        width_56 = tempi[10] + tempi[12];

        max_width_corners = (width_01 > width_23) ? width_01 : width_23;
        max_width         = (width_56 > max_width_corners) ? width_56 : max_width_corners;
      }

      // While the size is too large decrease it

      while ((height_02 > tSize[1] ||
              height_13 > tSize[1] ||
              height_47 > tSize[1] ||
              max_width > tSize[0] ||
              height_02 > line_max_02 ||
              height_13 > line_max_13 ||
              height_47 > line_max_47) &&
             fontSize > 0)
      {
        fontSize--;
        for (int i = 0; i < NumTextPositions; i++)
        {
          this->TextMapper[i]->GetTextProperty()->SetFontSize(fontSize);
          this->TextMapper[i]->GetSize(viewport, tempi + i * 2);
        }
        height_02 = tempi[1] + tempi[5];
        height_13 = tempi[3] + tempi[7];
        height_47 = tempi[9] + tempi[15];

        width_01 = tempi[0] + tempi[2];
        width_23 = tempi[4] + tempi[6];
        width_56 = tempi[10] + tempi[12];

        max_width_corners = (width_01 > width_23) ? width_01 : width_23;
        max_width         = (width_56 > max_width_corners) ? width_56 : max_width_corners;
      }

      fontSize = static_cast<int>(pow((double)fontSize,
              NonlinearFontScaleFactor)*LinearFontScaleFactor);
      if (fontSize > this->MaximumFontSize)
      {
        fontSize = this->MaximumFontSize;
      }
      this->FontSize = fontSize;
      for (int i = 0; i < NumTextPositions; i++)
      {
        this->TextMapper[i]->GetTextProperty()->SetFontSize(fontSize);
      }

      // Now set the position of the TextActors

      this->SetTextActorsPosition(vSize);

      for (int i = 0; i < NumTextPositions; i++)
      {
        this->TextActor[i]->SetProperty(this->GetProperty());
      }
    }
    this->BuildTime.Modified();
    this->LastImageActor = ia;
  }

  // Everything is built, just have to render

  if (this->FontSize >= this->MinimumFontSize)
  {
    for (int i = 0; i < NumTextPositions; i++)
    {
      this->TextActor[i]->RenderOpaqueGeometry(viewport);
    }
  }

  return 1;
}

//-----------------------------------------------------------------------------
// Description:
// Does this prop have some translucent polygonal geometry?
int vtkCornerAnnotation::HasTranslucentPolygonalGeometry()
{
  return 0;
}

//----------------------------------------------------------------------------
void vtkCornerAnnotation::SetTextActorsPosition(int vsize[2])
{
  this->TextActor[LowerLeft]->SetPosition  ( 5,             5             );
  this->TextActor[LowerRight]->SetPosition ( vsize[0] - 5,  5             );
  this->TextActor[UpperLeft]->SetPosition  ( 5,             vsize[1] - 5  );
  this->TextActor[UpperRight]->SetPosition ( vsize[0] - 5,  vsize[1] - 5  );

  this->TextActor[LowerEdge]->SetPosition  ( vsize[0]/2,    5             );
  this->TextActor[UpperEdge]->SetPosition  ( vsize[0]/2,    vsize[1] - 5  );
  this->TextActor[LeftEdge]->SetPosition   ( 5,             vsize[1]/2    );
  this->TextActor[RightEdge]->SetPosition  ( vsize[0] - 5,  vsize[1]/2    );
}

//----------------------------------------------------------------------------
void vtkCornerAnnotation::SetTextActorsJustification()
{
  vtkTextProperty *tprop = this->TextMapper[LowerLeft]->GetTextProperty();
  tprop->SetJustificationToLeft();
  tprop->SetVerticalJustificationToBottom();

  tprop = this->TextMapper[LowerRight]->GetTextProperty();
  tprop->SetJustificationToRight();
  tprop->SetVerticalJustificationToBottom();

  tprop = this->TextMapper[UpperLeft]->GetTextProperty();
  tprop->SetJustificationToLeft();
  tprop->SetVerticalJustificationToTop();

  tprop = this->TextMapper[UpperRight]->GetTextProperty();
  tprop->SetJustificationToRight();
  tprop->SetVerticalJustificationToTop();

  tprop = this->TextMapper[LowerEdge]->GetTextProperty();
  tprop->SetJustificationToCentered();
  tprop->SetVerticalJustificationToBottom();

  tprop = this->TextMapper[UpperEdge]->GetTextProperty();
  tprop->SetJustificationToCentered();
  tprop->SetVerticalJustificationToTop();

  tprop = this->TextMapper[LeftEdge]->GetTextProperty();
  tprop->SetJustificationToLeft();
  tprop->SetVerticalJustificationToCentered();

  tprop = this->TextMapper[RightEdge]->GetTextProperty();
  tprop->SetJustificationToRight();
  tprop->SetVerticalJustificationToCentered();
}

//----------------------------------------------------------------------------
void vtkCornerAnnotation::SetText(int i, const char *text)
{
  if (i < 0 || i >= NumTextPositions)
  {
    return;
  }

  if (!text ||
      (this->CornerText[i] && text && (!strcmp(this->CornerText[i],text))))
  {
    return;
  }
  delete [] this->CornerText[i];
  this->CornerText[i] = new char [strlen(text)+1];
  strcpy(this->CornerText[i],text);
  this->Modified();
}

//----------------------------------------------------------------------------
const char* vtkCornerAnnotation::GetText(int i)
{
  if (i < 0 || i >= NumTextPositions)
  {
    return NULL;
  }

  return this->CornerText[i];
}

//----------------------------------------------------------------------------
void vtkCornerAnnotation::ClearAllTexts()
{
  for (int i = 0; i < NumTextPositions; i++)
  {
    this->SetText(i, "");
  }
}

//----------------------------------------------------------------------------
void vtkCornerAnnotation::CopyAllTextsFrom(vtkCornerAnnotation *ca)
{
  for (int i = 0; i < NumTextPositions; i++)
  {
    this->SetText(i, ca->GetText(i));
  }
}

//----------------------------------------------------------------------------
void vtkCornerAnnotation::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "ImageActor: " << this->GetImageActor() << endl;
  os << indent << "MinimumFontSize: " << this->GetMinimumFontSize() << endl;
  os << indent << "MaximumFontSize: " << this->GetMaximumFontSize() << endl;
  os << indent << "LinearFontScaleFactor: " << this->GetLinearFontScaleFactor() << endl;
  os << indent << "NonlinearFontScaleFactor: " << this->GetNonlinearFontScaleFactor() << endl;
  os << indent << "WindowLevel: " << this->GetWindowLevel() << endl;
  os << indent << "Mapper: " << this->GetMapper() << endl;
  os << indent << "MaximumLineHeight: " << this->MaximumLineHeight << endl;
  os << indent << "LevelShift: " << this->LevelShift << endl;
  os << indent << "LevelScale: " << this->LevelScale << endl;
  os << indent << "TextProperty: " << this->TextProperty << endl;
  os << indent << "ShowSliceAndImage: " << this->ShowSliceAndImage << endl;
}


