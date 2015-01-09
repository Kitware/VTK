/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLFreeTypeTextMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLFreeTypeTextMapper.h"

#include "vtkActor2D.h"
#include "vtkObjectFactory.h"
#include "vtkProperty2D.h"
#include "vtkTextProperty.h"
#include "vtkViewport.h"
#include "vtkWindow.h"
#include "vtkOpenGLError.h"

#include "vtkFreeTypeUtilities.h"
#include "vtkftglConfig.h"

#include "FTFont.h"

#include "vtkOpenGLShaderCache.h"
#include "vtkOpenGLRenderWindow.h"

#ifdef FTGL_USE_NAMESPACE
using namespace ftgl;
#endif

namespace {
inline int GetNumberOfLinesImpl(const char *str)
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
// Print debug info

#define VTK_FTTM_DEBUG 0

//----------------------------------------------------------------------------
// GL2PS related internal helper functions.

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLFreeTypeTextMapper);

//----------------------------------------------------------------------------
vtkOpenGLFreeTypeTextMapper::vtkOpenGLFreeTypeTextMapper()
{
  this->LastSize[0] = 0;
  this->LastSize[1] = 0;
  this->TextLines = NULL;
  this->NumberOfLines = 0;
  this->NumberOfLinesAllocated = 0;
}

//----------------------------------------------------------------------------
vtkOpenGLFreeTypeTextMapper::~vtkOpenGLFreeTypeTextMapper()
{
  if (this->TextLines != NULL)
    {
    for (int i=0; i < this->NumberOfLinesAllocated; i++)
      {
      this->TextLines[i]->Delete();
      }
    delete [] this->TextLines;
    }

  if (this->LastWindow)
    {
    this->ReleaseGraphicsResources(this->LastWindow);
    }
}

//----------------------------------------------------------------------------
char *vtkOpenGLFreeTypeTextMapper::NextLine(const char *input, int lineNum)
{
  const char *ptr, *ptrEnd;
  int strLen;
  char *line;

  ptr = input;
  for (int i=0; i != lineNum; i++)
    {
    ptr = strstr(ptr,"\n");
    ptr++;
    }
  ptrEnd = strstr(ptr,"\n");
  if ( ptrEnd == NULL )
    {
    ptrEnd = strchr(ptr, '\0');
    }

  strLen = ptrEnd - ptr;
  line = new char[strLen+1];
  strncpy(line, ptr, strLen);
  line[strLen] = '\0';

  return line;
}

//----------------------------------------------------------------------------
void vtkOpenGLFreeTypeTextMapper::GetMultiLineSize(vtkViewport *viewport,
                                                   int size[])
{
  int i;
  int lineSize[2];

  vtkTextProperty *tprop = this->GetTextProperty();
  if (!tprop)
    {
    vtkErrorMacro(<<"Need text property to get multiline size of mapper");
    size[0] = size[1] = 0;
    return;
    }

  lineSize[0] = lineSize[1] = size[0] = size[1] = 0;
  for ( i=0; i < this->NumberOfLines; i++ )
    {
    this->TextLines[i]->GetTextProperty()->ShallowCopy(tprop);
    this->TextLines[i]->GetSize(viewport, lineSize);
    size[0] = (lineSize[0] > size[0] ? lineSize[0] : size[0]);
    size[1] = (lineSize[1] > size[1] ? lineSize[1] : size[1]);
    }

  // add in the line spacing
  this->LineSize = size[1];
  size[1] = static_cast<int>(
    size[1] * (1.0 + (this->NumberOfLines - 1) * tprop->GetLineSpacing()));
}

//----------------------------------------------------------------------------
void vtkOpenGLFreeTypeTextMapper::RenderOverlayMultipleLines(
    vtkViewport *viewport, vtkActor2D *actor)
{
  float offset = 0.0f;
  int size[2];
  // make sure LineSize is up to date
  this->GetMultiLineSize(viewport,size);

  vtkTextProperty *tprop = this->GetTextProperty();
  if (!tprop)
    {
    vtkErrorMacro(<<"Need text property to render multiple lines of mapper");
    return;
    }

  switch (tprop->GetVerticalJustification())
    {
    case VTK_TEXT_TOP:
      offset = 0.0f;
      break;
    case VTK_TEXT_CENTERED:
      offset = (-this->NumberOfLines + 1.0f) / 2.0f;
      break;
    case VTK_TEXT_BOTTOM:
      offset = -this->NumberOfLines + 1.0f;
      break;
    }

  for (int lineNum=0; lineNum < this->NumberOfLines; lineNum++)
    {
    this->TextLines[lineNum]->GetTextProperty()->ShallowCopy(tprop);
    this->TextLines[lineNum]->GetTextProperty()->SetLineOffset
      (tprop->GetLineOffset() +
       static_cast<int>(this->LineSize * (lineNum + offset)
                        * tprop->GetLineSpacing()));
    this->TextLines[lineNum]->RenderOverlay(viewport,actor);
    }
}

//----------------------------------------------------------------------------
void vtkOpenGLFreeTypeTextMapper::ReleaseGraphicsResources(vtkWindow *)
{
#if VTK_FTTM_DEBUG
    printf("vtkOpenGLFreeTypeTextMapper::ReleaseGraphicsResources\n");
#endif

  this->LastWindow = NULL;

  // Very important
  // the release of graphics resources indicates that significant changes have
  // occurred. Old fonts, cached sizes etc are all no longer valid, so we send
  // ourselves a general modified message.

  // this->Modified();
}

//----------------------------------------------------------------------------
void vtkOpenGLFreeTypeTextMapper::GetSize(vtkViewport* viewport, int *size)
{
  // Check for multiline

  if (this->NumberOfLines > 1)
    {
    this->GetMultiLineSize(viewport, size);
    return;
    }

  // Check for input

  if (this->Input == NULL || this->Input[0] == '\0')
    {
    size[0] = size[1] = 0;
    return;
    }

  vtkTextProperty *tprop = this->GetTextProperty();
  if (!tprop)
    {
    vtkErrorMacro(<< "Need a text property to get size");
    size[0] = size[1] = 0;
    return;
    }

  // Check to see whether we have to rebuild anything

  if (this->GetMTime() < this->SizeBuildTime &&
      tprop->GetMTime() < this->SizeBuildTime)
    {
#if VTK_FTTM_DEBUG
  printf("vtkOpenGLFreeTypeTextMapper::GetSize: In cache!\n");
#endif

    size[0] = this->LastSize[0];
    size[1] = this->LastSize[1];
    return;
    }

  // Check for font and try to set the size

  vtkFreeTypeUtilities::Entry *entry =
    vtkFreeTypeUtilities::GetInstance()->GetFont(tprop);
  FTFont *font = entry ? entry->Font : NULL;
  if (!font)
    {
    vtkErrorMacro(<< "Render - No font");
    size[0] = size[1] = 0;
    return;
    }

  // The font global ascender and descender might just be too high
  // for given a face. Let's get a compromise by computing these values
  // from some usual ascii chars.

  if (entry->LargestAscender < 0 || entry->LargestDescender < 0)
    {
    float llx, lly, llz, urx, ury, urz;
    font->BBox("_/7Agfy", llx, lly, llz, urx, ury, urz);
    entry->LargestAscender = ury;
    entry->LargestDescender = lly;
    }

  this->LastSize[0] = size[0] = static_cast<int>(font->Advance(this->Input));
  this->LastSize[1] = size[1] =
    static_cast<int>(entry->LargestAscender - entry->LargestDescender);
  this->LastLargestDescender = static_cast<int>(entry->LargestDescender);

  this->SizeBuildTime.Modified();
}

//----------------------------------------------------------------------------
void vtkOpenGLFreeTypeTextMapper::RenderOverlay(vtkViewport* viewport,
                                                vtkActor2D* actor)
{
  vtkDebugMacro (<< "RenderOverlay");

  // Check for input

  if (this->Input == NULL || this->Input[0] == '\0')
    {
    return;
    }

  // Check for multi-lines

  if (this->NumberOfLines > 1)
    {
    this->RenderOverlayMultipleLines(viewport, actor);
    return;
    }

  // Get text property

  vtkTextProperty *tprop = this->GetTextProperty();
  if (!tprop)
    {
    vtkErrorMacro(<< "Need a text property to render mapper");
    return;
    }

  vtkOpenGLClearErrorMacro();

  // Get the window information for display

  vtkWindow* window = viewport->GetVTKWindow();
  if (this->LastWindow && this->LastWindow != window)
    {
    this->ReleaseGraphicsResources(this->LastWindow);
    }
  this->LastWindow = window;

  // Get size of text

  int size[2];
  this->GetSize(viewport, size);

  // Get the position of the text actor

  int* actorPos;
  actorPos =
    actor->GetActualPositionCoordinate()->GetComputedViewportValue(viewport);

  // Define bounding rectangle

  int pos[2];
  pos[0] = actorPos[0];
  pos[1] = static_cast<int>(actorPos[1] - tprop->GetLineOffset());

  switch (tprop->GetJustification())
    {
    case VTK_TEXT_LEFT:
      break;
    case VTK_TEXT_CENTERED:
      pos[0] = pos[0] - size[0] / 2;
      break;
    case VTK_TEXT_RIGHT:
      pos[0] = pos[0] - size[0];
      break;
    }

  switch (tprop->GetVerticalJustification())
    {
    case VTK_TEXT_TOP:
      pos[1] = pos[1] - size[1] - this->LastLargestDescender;
      break;
    case VTK_TEXT_CENTERED:
      pos[1] = pos[1] - size[1] / 2 - this->LastLargestDescender / 2;
      break;
    case VTK_TEXT_BOTTOM:
      break;
    }

  // Push a 2D matrix on the stack

  int *vsize = viewport->GetSize();
  double *vport = viewport->GetViewport();
  double *tileViewport = viewport->GetVTKWindow()->GetTileViewport();
  double visVP[4];

  visVP[0] = (vport[0] >= tileViewport[0]) ? vport[0] : tileViewport[0];
  visVP[1] = (vport[1] >= tileViewport[1]) ? vport[1] : tileViewport[1];
  visVP[2] = (vport[2] <= tileViewport[2]) ? vport[2] : tileViewport[2];
  visVP[3] = (vport[3] <= tileViewport[3]) ? vport[3] : tileViewport[3];

  if (visVP[0] == visVP[2] || visVP[1] == visVP[3])
    {
    return;
    }

  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();

  if (viewport->GetIsPicking())
    {
/*    vtkgluPickMatrix(viewport->GetPickX(), viewport->GetPickY(),
                     viewport->GetPickWidth(),
                     viewport->GetPickHeight(),
                     viewport->GetOrigin(), viewport->GetSize());
*/    }

  glMatrixMode(GL_MODELVIEW);
  glLoadIdentity();

  // Store the state of the attributes we are about to change
  GLint depthFunc;
  glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);
  glDepthFunc(GL_ALWAYS);

  glDepthMask(GL_FALSE);
  // glDisable(GL_DEPTH_TEST);
  glDisable(GL_TEXTURE_2D);
  glEnable (GL_ALPHA_TEST);
  glEnable(GL_BLEND);
  glBlendFunc( GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA );
  // glPixelStorei( GL_UNPACK_ALIGNMENT, 1);
  // glPixelStorei( GL_PACK_ALIGNMENT, 1 );

  //- Make sure no shaders are loaded as freetype uses glDrawPixels which
  // will use a fragment shader if one is loaded.
  vtkOpenGLRenderWindow *renWin = static_cast<vtkOpenGLRenderWindow*>(viewport->GetVTKWindow());
  renWin->GetShaderCache()->ReleaseCurrentShader();

  if (actor->GetProperty()->GetDisplayLocation() == VTK_FOREGROUND_LOCATION)
    {
    glOrtho(0, vsize[0] - 1, 0, vsize[1] - 1, 0, 1);
    }
  else
    {
    glOrtho(0, vsize[0] - 1, 0, vsize[1] - 1, -1, 0);
    }

  int *winSize = viewport->GetVTKWindow()->GetSize();

  int xoff = static_cast<int>(pos[0] - winSize[0] * (visVP[0] - vport[0]));
  int yoff = static_cast<int>(pos[1] - winSize[1] * (visVP[1] - vport[1]));

  // When picking draw the bounds of the text as a rectangle,
  // as text only picks when the pick point is exactly on the
  // origin of the text

  if (viewport->GetIsPicking())
    {
    float x1 = (2.0 * actorPos[0]) / vsize[0] - 1.0;
    float y1 = 2.0 * (actorPos[1] - tprop->GetLineOffset())/vsize[1] - 1.0;
    float width = (2.0 * size[0]) / vsize[0];
    float height = (2.0 * size[1]) / vsize[1];
    glRectf(x1, y1, x1 + width, y1 + height);

    // Clean up and return after drawing the rectangle
    // Restore the original state
    glDepthFunc(depthFunc);

    return;
    }

  double* tprop_color = tprop->GetColor();
  double tprop_opacity = tprop->GetOpacity();

  // Get the font

  vtkFreeTypeUtilities::Entry *entry =
    vtkFreeTypeUtilities::GetInstance()->GetFont(tprop, tprop_color);
  FTFont *font = entry ? entry->Font : NULL;
  if (!font)
    {
    vtkErrorMacro(<< "Render - No font");
    return;
    }

  struct FTGLRenderContext *ftgl_context = 0;

  // Set up the shadow color

  if (tprop->GetShadow())
    {
    double shadow_color[3], rgb;
    rgb = ((tprop_color[0] + tprop_color[1] + tprop_color[2]) / 3.0 > 0.5)
      ? 0.0 : 1.0;
    shadow_color[0] = shadow_color[1] = shadow_color[2] = rgb;

    // Get the shadow font

    vtkFreeTypeUtilities::Entry *shadow_entry =
      vtkFreeTypeUtilities::GetInstance()->GetFont(tprop, shadow_color);
    FTFont *shadow_font = shadow_entry ? shadow_entry->Font : NULL;
    if (!shadow_font)
      {
      vtkErrorMacro(<< "Render - No shadow font");
      return;
      }

    // Set the color here since load/render glyphs is done
    // on demand and this color has to be consistent for a given font entry.

    glColor4ub(static_cast<unsigned char>(shadow_color[0] * 255.0),
               static_cast<unsigned char>(shadow_color[1] * 255.0),
               static_cast<unsigned char>(shadow_color[2] * 255.0),
               static_cast<unsigned char>(tprop_opacity * 255.0));

    // Required for clipping to work correctly

    glRasterPos2i(0, 0);
    glBitmap(0, 0, 0, 0,
             xoff + tprop->GetShadowOffset()[0],
             yoff + tprop->GetShadowOffset()[1], NULL);

    // Draw the shadow text

    shadow_font->render(this->Input, ftgl_context);

    // Get the font again, Duh, since it may have been freed from the
    // cache by the shadow font

    font = vtkFreeTypeUtilities::GetInstance()->GetFont(
      tprop, tprop_color)->Font;
    if (!font)
      {
      vtkErrorMacro(<< "Render - No font");
      return;
      }
    }

  // Set the color here since load/render glyphs is done
  // on demand and this color has to be consistent for a given font entry.

  glColor4ub(static_cast<unsigned char>(tprop_color[0] * 255.0),
             static_cast<unsigned char>(tprop_color[1] * 255.0),
             static_cast<unsigned char>(tprop_color[2] * 255.0),
             static_cast<unsigned char>(tprop_opacity * 255.0));

  // Required for clipping to work correctly

  glRasterPos2i(0, 0);
  glBitmap(0, 0, 0, 0, xoff, yoff, NULL);

  // Display a string

  font->render(this->Input, ftgl_context);

  glFlush();

  // Restore the original GL state
  glDepthFunc(depthFunc);

  vtkOpenGLCheckErrorMacro("failed after RenderOverlay");
}

//----------------------------------------------------------------------------
void vtkOpenGLFreeTypeTextMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "NumberOfLines: " << this->NumberOfLines << "\n";
}

//----------------------------------------------------------------------------
void vtkOpenGLFreeTypeTextMapper::SetInput(const char *input)
{
  if ( this->Input && input && (!strcmp(this->Input,input)))
    {
    return;
    }
  delete [] this->Input;
  if (input)
    {
    this->Input = new char[strlen(input)+1];
    strcpy(this->Input,input);
    }
  else
    {
    this->Input = NULL;
    }
  this->Modified();

  int numLines = GetNumberOfLinesImpl(input);

  if ( numLines <= 1) // a line with no "\n"
    {
    this->NumberOfLines = numLines;
    }

  else //multiple lines
    {
    char *line;
    int i;

    if ( numLines > this->NumberOfLinesAllocated )
      {
      // delete old stuff
      if ( this->TextLines )
        {
        for (i=0; i < this->NumberOfLinesAllocated; i++)
          {
          this->TextLines[i]->Delete();
          }
        delete [] this->TextLines;
        }

      // allocate new text mappers
      this->NumberOfLinesAllocated = numLines;
      this->TextLines = new vtkTextMapper *[numLines];
      for (i=0; i < numLines; i++)
        {
        this->TextLines[i] = vtkTextMapper::New();
        }
      } //if we need to reallocate

    // set the input strings
    this->NumberOfLines = numLines;
    for (i=0; i < this->NumberOfLines; i++)
      {
      line = this->NextLine(input, i);
      this->TextLines[i]->SetInput( line );
      delete [] line;
      }
    }
}
