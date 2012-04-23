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
#include "vtkToolkits.h"  // for VTK_USE_GL2PS

#include "vtkFreeTypeUtilities.h"
#include "vtkftglConfig.h"

#include "vtkgluPickMatrix.h"

#include "FTFont.h"

#ifdef VTK_USE_GL2PS
#include "vtk_gl2ps.h"
#endif // VTK_USE_GL2PS

#ifdef FTGL_USE_NAMESPACE
using namespace ftgl;
#endif

//----------------------------------------------------------------------------
// Print debug info

#define VTK_FTTM_DEBUG 0
#define VTK_FTTM_DEBUG_CD 0

//----------------------------------------------------------------------------
// GL2PS related internal helper functions.

#ifdef VTK_USE_GL2PS
static void
vtkOpenGLFreeTypeTextMapper_GetGL2PSFontName(vtkTextProperty *tprop,
                                             char *ps_font)
{
 // For speed we use ARIAL == 0, COURIER == 1, TIMES == 2
  static char const *family[] = {"Helvetica", "Courier", "Times"};
  static char const *italic[] = {"Oblique", "Oblique", "Italic"};
  static char const *base[] = {"", "", "-Roman"};

  int font = tprop->GetFontFamily();

  if (font > 2)
    {
    sprintf(ps_font, "%s", tprop->GetFontFamilyAsString());
    if (tprop->GetBold())
      {
      sprintf(ps_font, "%s%s", ps_font, "Bold");
      }
    if (tprop->GetItalic())
      {
      sprintf(ps_font, "%s%s", ps_font, "Italic");
      }
      return;
    }

  if (tprop->GetBold())
    {
    sprintf(ps_font, "%s-%s", family[font], "Bold");
    if (tprop->GetItalic())
      {
      sprintf(ps_font, "%s%s", ps_font, italic[font]);
      }
    }
  else if (tprop->GetItalic())
    {
    sprintf(ps_font, "%s-%s", family[font], italic[font]);
    }
  else
    {
    sprintf(ps_font, "%s%s", family[font], base[font]);
    }
}
#endif

//----------------------------------------------------------------------------
vtkStandardNewMacro(vtkOpenGLFreeTypeTextMapper);

//----------------------------------------------------------------------------
vtkOpenGLFreeTypeTextMapper::vtkOpenGLFreeTypeTextMapper()
{
  this->LastSize[0] = 0;
  this->LastSize[1] = 0;
}

//----------------------------------------------------------------------------
vtkOpenGLFreeTypeTextMapper::~vtkOpenGLFreeTypeTextMapper()
{
  if (this->LastWindow)
    {
    this->ReleaseGraphicsResources(this->LastWindow);
    }
}

//----------------------------------------------------------------------------
void vtkOpenGLFreeTypeTextMapper::ReleaseGraphicsResources(vtkWindow *vtkNotUsed(win))
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
  actorPos=
    actor->GetActualPositionCoordinate()->GetComputedViewportValue(viewport);

  // Define bounding rectangle

  int pos[2];
  pos[0] = actorPos[0];
  pos[1] =  static_cast<int>(actorPos[1] - tprop->GetLineOffset());

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
  glPushMatrix();
  glLoadIdentity();

  if(viewport->GetIsPicking())
    {
    vtkgluPickMatrix(viewport->GetPickX(), viewport->GetPickY(),
                     viewport->GetPickWidth(),
                     viewport->GetPickHeight(),
                     viewport->GetOrigin(), viewport->GetSize());
    }

  glMatrixMode(GL_MODELVIEW);
  glPushMatrix();
  glLoadIdentity();

  // Store the state of the attributes we are about to change
  GLboolean lightingEnabled = glIsEnabled(GL_LIGHTING);
  GLint depthFunc;
  glGetIntegerv(GL_DEPTH_FUNC, &depthFunc);
  glDisable(GL_LIGHTING);
  glDepthFunc(GL_ALWAYS);

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

  if(viewport->GetIsPicking())
    {
    float x1 = (2.0 * actorPos[0]) / vsize[0] - 1.0;
    float y1 = 2.0 * (actorPos[1] - tprop->GetLineOffset())/vsize[1] - 1.0;
    float width = (2.0 * size[0]) / vsize[0];
    float height = (2.0 * size[1]) / vsize[1];
    glRectf(x1, y1, x1 + width, y1 + height);

    // Clean up and return after drawing the rectangle
    // Restore the original state
    glMatrixMode(GL_PROJECTION);
    glPopMatrix();
    glMatrixMode(GL_MODELVIEW);
    glPopMatrix();
    if (lightingEnabled)
      {
      glEnable(GL_LIGHTING);
      }
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

  // Setup the fonts for GL2PS output.

#ifdef VTK_USE_GL2PS
  char ps_font[64];
  vtkOpenGLFreeTypeTextMapper_GetGL2PSFontName(tprop, ps_font);
#endif // VTK_USE_GL2PS

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

    // Shadow text for GL2PS.

#ifdef VTK_USE_GL2PS
    glRasterPos2i(xoff + tprop->GetShadowOffset()[0],
                  yoff + tprop->GetShadowOffset()[1]);
    gl2psText(this->Input, ps_font, tprop->GetFontSize());
#endif // VTK_USE_GL2PS
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

  // Normal text for GL2PS.

#ifdef VTK_USE_GL2PS
  glRasterPos2i(xoff, yoff);
  gl2psText(this->Input, ps_font, tprop->GetFontSize());
#endif // VTK_USE_GL2PS

  // Restore the original GL state
  glMatrixMode(GL_PROJECTION);
  glPopMatrix();
  glMatrixMode(GL_MODELVIEW);
  glPopMatrix();
  if (lightingEnabled)
    {
    glEnable(GL_LIGHTING);
    }
  glDepthFunc(depthFunc);
}

void vtkOpenGLFreeTypeTextMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
