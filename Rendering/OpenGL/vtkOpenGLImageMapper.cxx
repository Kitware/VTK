/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLImageMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkOpenGLImageMapper.h"

#include "vtkActor2D.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkProperty2D.h"
#include "vtkViewport.h"
#include "vtkWindow.h"
#include "vtkgluPickMatrix.h"

#include "vtkOpenGL.h"
#include "vtkOpenGLError.h"
#include <climits>
#include <cassert>

vtkStandardNewMacro(vtkOpenGLImageMapper);

vtkOpenGLImageMapper::vtkOpenGLImageMapper()
{
}

vtkOpenGLImageMapper::~vtkOpenGLImageMapper()
{
}

//----------------------------------------------------------------------------
// I know #define can be evil, but this macro absolutely ensures
// that the code will be inlined.  The macro expects 'val' to
// be predefined to the same type as y

#define vtkClampToUnsignedChar(x,y) \
{ \
  val = (y); \
  if (val < 0) \
  { \
    val = 0; \
  } \
  if (val > 255) \
  { \
    val = 255; \
  } \
  (x) = static_cast<unsigned char>(val); \
}
/* should do proper rounding, as follows:
  (x) = (unsigned char)(val + 0.5f); \
*/

// the bit-shift must be done after the comparison to zero
// because bit-shift is implemenation dependent for negative numbers
#define vtkClampIntToUnsignedChar(x,y,shift) \
{ \
  val = (y); \
  if (val < 0) \
  { \
    val = 0; \
  } \
  val >>= shift; \
  if (val > 255) \
  { \
    val = 255; \
  } \
  (x) = static_cast<unsigned char>(val); \
}

// pad an integer to a multiply of four, for OpenGL
inline int vtkPadToFour(int n)
{
  return ((n+3)/4)*4;
}

//---------------------------------------------------------------
// render the image by doing the following:
// 1) apply shift and scale to pixel values
// 2) clamp to [0,255] and convert to unsigned char
// 3) draw using glDrawPixels

template <class T>
void vtkOpenGLImageMapperRenderDouble(vtkOpenGLImageMapper *self, vtkImageData *data,
                                      T *dataPtr, double shift, double scale,
                                      int *actorPos, int *actorPos2, int front, int *vsize)
{
  vtkOpenGLClearErrorMacro();

  int inMin0 = self->DisplayExtent[0];
  int inMax0 = self->DisplayExtent[1];
  int inMin1 = self->DisplayExtent[2];
  int inMax1 = self->DisplayExtent[3];

  int width = inMax0 - inMin0 + 1;
  int height = inMax1 - inMin1 + 1;

  vtkIdType tempIncs[3];
  data->GetIncrements(tempIncs);
  vtkIdType inInc1 = tempIncs[1];

  int bpp = data->GetNumberOfScalarComponents();
  double range[2];
  data->GetPointData()->GetScalars()->GetDataTypeRange( range );

  // the value .999 is sensitive to z-buffer depth
  glRasterPos3f((2.0 * actorPos[0]) / vsize[0] - 1.0,
                (2.0 * actorPos[1]) / vsize[1] - 1.0,
                (front)?(-1.0):(.999));

  glPixelStorei( GL_UNPACK_ALIGNMENT, 1);

  // reformat data into unsigned char

  T *inPtr = dataPtr;
  T *inPtr1 = inPtr;

  int i;
  int j = height;

  unsigned char *newPtr;
  if (bpp < 4)
  {
    newPtr = new unsigned char[vtkPadToFour(3*width*height)];
  }
  else
  {
    newPtr = new unsigned char[4*width*height];
  }

  unsigned char *ptr = newPtr;
  double val;
  unsigned char tmp;

  while (--j >= 0)
  {
    inPtr = inPtr1;
    i = width;
    switch (bpp)
    {
      case 1:
        while (--i >= 0)
        {
          vtkClampToUnsignedChar(tmp,((*inPtr++ + shift)*scale));
          *ptr++ = tmp;
          *ptr++ = tmp;
          *ptr++ = tmp;
        }
        break;

      case 2:
        while (--i >= 0)
        {
          vtkClampToUnsignedChar(tmp,((*inPtr++ + shift)*scale));
          *ptr++ = tmp;
          vtkClampToUnsignedChar(*ptr++,((*inPtr++ + shift)*scale));
          *ptr++ = tmp;
        }
        break;

      case 3:
        while (--i >= 0)
        {
          vtkClampToUnsignedChar(*ptr++,((*inPtr++ + shift)*scale));
          vtkClampToUnsignedChar(*ptr++,((*inPtr++ + shift)*scale));
          vtkClampToUnsignedChar(*ptr++,((*inPtr++ + shift)*scale));
        }
        break;

      default:
        while (--i >= 0)
        {
          vtkClampToUnsignedChar(*ptr++,((*inPtr++ + shift)*scale));
          vtkClampToUnsignedChar(*ptr++,((*inPtr++ + shift)*scale));
          vtkClampToUnsignedChar(*ptr++,((*inPtr++ + shift)*scale));
          vtkClampToUnsignedChar(*ptr++,((*inPtr++ + shift)*scale));
          inPtr += bpp-4;
        }
        break;
    }
    inPtr1 += inInc1;
  }

  if (self->GetRenderToRectangle())
  {
    int rectwidth  = (actorPos2[0] - actorPos[0]) + 1;
    int rectheight = (actorPos2[1] - actorPos[1]) + 1;
    float xscale = static_cast<float>(rectwidth)/width;
    float yscale = static_cast<float>(rectheight)/height;
    glPixelZoom(xscale, yscale);
  }

  glDrawPixels(width, height, ((bpp < 4) ? GL_RGB : GL_RGBA),
               GL_UNSIGNED_BYTE, static_cast<void *>(newPtr));

  if (self->GetRenderToRectangle())
  {
    // restore zoom to 1,1 otherwise other glDrawPixels cals may be affected
    glPixelZoom(1.0, 1.0);
  }
  delete [] newPtr;

 vtkOpenGLStaticCheckErrorMacro("failed after ImageMapperRenderDouble");
}

//---------------------------------------------------------------
// Same as above, but uses fixed-point math for shift and scale.
// The number of bits used for the fraction is determined from the
// scale.  Enough bits are always left over for the integer that
// overflow cannot occur.

template <class T>
void vtkOpenGLImageMapperRenderShort(vtkOpenGLImageMapper *self, vtkImageData *data,
                                     T *dataPtr, double shift, double scale,
                                     int *actorPos, int *actorPos2, int front,
                                     int *vsize)
{
  vtkOpenGLClearErrorMacro();

  int inMin0 = self->DisplayExtent[0];
  int inMax0 = self->DisplayExtent[1];
  int inMin1 = self->DisplayExtent[2];
  int inMax1 = self->DisplayExtent[3];

  int width = inMax0 - inMin0 + 1;
  int height = inMax1 - inMin1 + 1;

  vtkIdType tempIncs[3];
  data->GetIncrements(tempIncs);
  vtkIdType inInc1 = tempIncs[1];

  int bpp = data->GetNumberOfScalarComponents();

  double range[2];
  data->GetPointData()->GetScalars()->GetDataTypeRange( range );

  // the value .999 is sensitive to z-buffer depth
  glRasterPos3f((2.0*actorPos[0]) / vsize[0] - 1.0,
                (2.0*actorPos[1]) / vsize[1] - 1.0,
                (front)?(-1.0):(.999));

  glPixelStorei( GL_UNPACK_ALIGNMENT, 1);

  // Find the number of bits to use for the fraction:
  // continue increasing the bits until there is an overflow
  // in the worst case, then decrease by 1.
  // The "*2.0" and "*1.0" ensure that the comparison is done
  // with double-precision math.
  int bitShift = 0;
  double absScale = ((scale < 0) ? -scale : scale);

  while ((1UL << bitShift)*absScale*2.0*USHRT_MAX < INT_MAX*1.0)
  {
    bitShift++;
  }
  bitShift--;
  assert(bitShift >= 0);
  assert(bitShift <= 30);

  long sscale = static_cast<long>(scale*(1 << bitShift));
  long sshift = static_cast<long>(sscale*shift);
  /* should do proper rounding, as follows:
  long sscale = (long) floor(scale*(1 << bitShift) + 0.5);
  long sshift = (long) floor((scale*shift + 0.5)*(1 << bitShift));
  */
  long val;
  unsigned char tmp;

  T *inPtr = dataPtr;
  T *inPtr1 = inPtr;

  int i;
  int j = height;

  unsigned char *newPtr;
  if (bpp < 4)
  {
    newPtr = new unsigned char[vtkPadToFour(3*width*height)];
  }
  else
  {
    newPtr = new unsigned char[4*width*height];
  }

  unsigned char *ptr = newPtr;

  while (--j >= 0)
  {
    inPtr = inPtr1;
    i = width;

    switch (bpp)
    {
      case 1:
        while (--i >= 0)
        {
          vtkClampIntToUnsignedChar(tmp,(*inPtr++*sscale+sshift),bitShift);
          *ptr++ = tmp;
          *ptr++ = tmp;
          *ptr++ = tmp;
        }
        break;

      case 2:
        while (--i >= 0)
        {
          vtkClampIntToUnsignedChar(tmp,(*inPtr++*sscale+sshift),bitShift);
          *ptr++ = tmp;
          vtkClampIntToUnsignedChar(*ptr++,(*inPtr++*sscale+sshift),bitShift);
          *ptr++ = tmp;
        }
        break;

      case 3:
        while (--i >= 0)
        {
          vtkClampIntToUnsignedChar(*ptr++,(*inPtr++*sscale+sshift),bitShift);
          vtkClampIntToUnsignedChar(*ptr++,(*inPtr++*sscale+sshift),bitShift);
          vtkClampIntToUnsignedChar(*ptr++,(*inPtr++*sscale+sshift),bitShift);
        }
        break;

      default:
        while (--i >= 0)
        {
          vtkClampIntToUnsignedChar(*ptr++,(*inPtr++*sscale+sshift),bitShift);
          vtkClampIntToUnsignedChar(*ptr++,(*inPtr++*sscale+sshift),bitShift);
          vtkClampIntToUnsignedChar(*ptr++,(*inPtr++*sscale+sshift),bitShift);
          vtkClampIntToUnsignedChar(*ptr++,(*inPtr++*sscale+sshift),bitShift);
          inPtr += bpp-4;
        }
        break;
    }
    inPtr1 += inInc1;
  }

  if (self->GetRenderToRectangle())
  {
    int rectwidth  = (actorPos2[0] - actorPos[0]) + 1;
    int rectheight = (actorPos2[1] - actorPos[1]) + 1;
    float xscale = static_cast<float>(rectwidth)/width;
    float yscale = static_cast<float>(rectheight)/height;
    glPixelZoom(xscale, yscale);
  }

  glDrawPixels(width, height, ((bpp < 4) ? GL_RGB : GL_RGBA),
               GL_UNSIGNED_BYTE, static_cast<void *>(newPtr));

  if (self->GetRenderToRectangle())
  {
    // restore zoom to 1,1 otherwise other glDrawPixels cals may be affected
    glPixelZoom(1.0, 1.0);
  }
  delete [] newPtr;

  vtkOpenGLStaticCheckErrorMacro("failed after ImageMapperRenderShort");
}

//---------------------------------------------------------------
// render unsigned char data without any shift/scale

template <class T>
void vtkOpenGLImageMapperRenderChar(vtkOpenGLImageMapper *self, vtkImageData *data,
                                    T *dataPtr, int *actorPos, int *actorPos2,
                                    int front, int *vsize)
{
  vtkOpenGLClearErrorMacro();

  int inMin0 = self->DisplayExtent[0];
  int inMax0 = self->DisplayExtent[1];
  int inMin1 = self->DisplayExtent[2];
  int inMax1 = self->DisplayExtent[3];

  int width = inMax0 - inMin0 + 1;
  int height = inMax1 - inMin1 + 1;

  vtkIdType tempIncs[3];
  data->GetIncrements(tempIncs);
  vtkIdType inInc1 = tempIncs[1];

  int bpp = data->GetPointData()->GetScalars()->GetNumberOfComponents();

  double range[2];
  data->GetPointData()->GetScalars()->GetDataTypeRange( range );

  // the value .999 is sensitive to z-buffer depth
  glRasterPos3f((2.0*actorPos[0]) / vsize[0] - 1.0,
                (2.0*actorPos[1]) / vsize[1] - 1.0,
                (front)?(-1):(.999));


  glPixelStorei( GL_UNPACK_ALIGNMENT, 1);

  if (self->GetRenderToRectangle())
  {
    int rectwidth  = (actorPos2[0] - actorPos[0]) + 1;
    int rectheight = (actorPos2[1] - actorPos[1]) + 1;
    float xscale = static_cast<float>(rectwidth)/width;
    float yscale = static_cast<float>(rectheight)/height;
    glPixelZoom(xscale, yscale);
  }
  //
  if (bpp == 3)
  { // feed through RGB bytes without reformatting
    if (inInc1 != width*bpp)
    {
      glPixelStorei( GL_UNPACK_ROW_LENGTH, inInc1/bpp );
    }
    glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE,
                 static_cast<void *>(dataPtr));
  }
  else if (bpp == 4)
  { // feed through RGBA bytes without reformatting
    if (inInc1 != width*bpp)
    {
      glPixelStorei( GL_UNPACK_ROW_LENGTH, inInc1/bpp );
    }
    glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE,
                 static_cast<void *>(dataPtr));
  }
  else
  { // feed through other bytes without reformatting
    T *inPtr = dataPtr;
    T *inPtr1 = inPtr;
    unsigned char tmp;

    int i;
    int j = height;

    unsigned char *newPtr;
    if (bpp < 4)
    {
      newPtr = new unsigned char[vtkPadToFour(3*width*height)];
    }
    else
    {
      newPtr = new unsigned char[4*width*height];
    }

    unsigned char *ptr = newPtr;

    while (--j >= 0)
    {
      inPtr = inPtr1;
      i = width;

      switch (bpp)
      {
        case 1:
          while (--i >= 0)
          {
            *ptr++ = tmp = *inPtr++;
            *ptr++ = tmp;
            *ptr++ = tmp;
          }
          break;

        case 2:
          while (--i >= 0)
          {
            *ptr++ = tmp = *inPtr++;
            *ptr++ = *inPtr++;
            *ptr++ = tmp;
          }
          break;

        case 3:
          while (--i >= 0)
          {
            *ptr++ = *inPtr++;
            *ptr++ = *inPtr++;
            *ptr++ = *inPtr++;
          }
          break;

        default:
          while (--i >= 0)
          {
            *ptr++ = *inPtr++;
            *ptr++ = *inPtr++;
            *ptr++ = *inPtr++;
            *ptr++ = *inPtr++;
            inPtr += bpp-4;
          }
          break;
      }
      inPtr1 += inInc1;
    }

    glDrawPixels(width, height, ((bpp < 4) ? GL_RGB : GL_RGBA),
                 GL_UNSIGNED_BYTE, static_cast<void *>(newPtr));

    delete [] newPtr;
  }

  if (self->GetRenderToRectangle())
  {
    // restore zoom to 1,1 otherwise other glDrawPixels cals may be affected
    glPixelZoom(1.0, 1.0);
  }

  glPixelStorei( GL_UNPACK_ROW_LENGTH, 0);

  vtkOpenGLStaticCheckErrorMacro("failed after ImageMapperRenderChar");
}

//----------------------------------------------------------------------------
// Define overloads to help the template macro below dispatch to a
// suitable implementation for each type.  The last argument is of
// type "long" for the template and of type "int" for the
// non-templates.  The template macro's call to this function always
// passes a literal "1" as the last argument, which requires a
// conversion to produce a long.  This helps broken compilers select
// the non-template even when the template is otherwise an equal
// match.
template <class T>
void vtkOpenGLImageMapperRender(vtkOpenGLImageMapper *self, vtkImageData *data,
                                T *dataPtr, double shift, double scale,
                                int *actorPos, int *actorPos2, int front, int *vsize,
                                long)
{
  vtkOpenGLImageMapperRenderDouble(self, data, dataPtr, shift, scale,
                                   actorPos, actorPos2, front, vsize);
}

static void vtkOpenGLImageMapperRender(vtkOpenGLImageMapper *self, vtkImageData *data,
                                       char* dataPtr, double shift, double scale,
                                       int *actorPos, int *actorPos2, int front, int *vsize,
                                       int)
{
  if(shift == 0.0 && scale == 1.0)
  {
    vtkOpenGLImageMapperRenderChar(self, data, dataPtr,
                                   actorPos, actorPos2, front, vsize);
  }
  else
  {
    vtkOpenGLImageMapperRenderShort(self, data, dataPtr, shift, scale,
                                    actorPos, actorPos2, front, vsize);
  }
}

static void vtkOpenGLImageMapperRender(vtkOpenGLImageMapper *self, vtkImageData *data,
                                       unsigned char* dataPtr, double shift, double scale,
                                       int *actorPos, int *actorPos2, int front, int *vsize,
                                       int)
{
  if(shift == 0.0 && scale == 1.0)
  {
    vtkOpenGLImageMapperRenderChar(self, data, dataPtr,
                                   actorPos, actorPos2, front, vsize);
  }
  else
  {
    vtkOpenGLImageMapperRenderShort(self, data, dataPtr, shift, scale,
                                    actorPos, actorPos2, front, vsize);
  }
}

static void vtkOpenGLImageMapperRender(vtkOpenGLImageMapper *self, vtkImageData *data,
                                       signed char* dataPtr, double shift, double scale,
                                       int *actorPos, int *actorPos2, int front, int *vsize,
                                       int)
{
  if(shift == 0.0 && scale == 1.0)
  {
    vtkOpenGLImageMapperRenderChar(self, data, dataPtr,
                                   actorPos, actorPos2, front, vsize);
  }
  else
  {
    vtkOpenGLImageMapperRenderShort(self, data, dataPtr, shift, scale,
                                    actorPos, actorPos2, front, vsize);
  }
}

static void vtkOpenGLImageMapperRender(vtkOpenGLImageMapper *self, vtkImageData *data,
                                       short* dataPtr, double shift, double scale,
                                       int *actorPos, int *actorPos2, int front, int *vsize,
                                       int)
{
  vtkOpenGLImageMapperRenderShort(self, data, dataPtr, shift, scale,
                                  actorPos, actorPos2, front, vsize);
}

static void vtkOpenGLImageMapperRender(vtkOpenGLImageMapper *self, vtkImageData *data,
                                       unsigned short* dataPtr, double shift, double scale,
                                       int *actorPos, int *actorPos2, int front, int *vsize,
                                       int)
{
  vtkOpenGLImageMapperRenderShort(self, data, dataPtr, shift, scale,
                                  actorPos, actorPos2, front, vsize);
}

//----------------------------------------------------------------------------
// Expects data to be X, Y, components

void vtkOpenGLImageMapper::RenderData(vtkViewport* viewport,
                                      vtkImageData *data, vtkActor2D *actor)
{
  void *ptr0;
  double shift, scale;

  vtkWindow* window = static_cast<vtkWindow *>(viewport->GetVTKWindow());
  if (!window)
  {
    vtkErrorMacro (<<"vtkOpenGLImageMapper::RenderData - no window set for viewport");
    return;
  }

  // Make this window current. May have become not current due to
  // data updates since the render started.
  window->MakeCurrent();

  vtkOpenGLClearErrorMacro();

  shift = this->GetColorShift();
  scale = this->GetColorScale();

  ptr0 = data->GetScalarPointer(this->DisplayExtent[0],
                                this->DisplayExtent[2],
                                this->DisplayExtent[4]);

  // push a 2D matrix on the stack
  int *vsize = viewport->GetSize();
  glMatrixMode( GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  if(viewport->GetIsPicking())
  {
    vtkgluPickMatrix(viewport->GetPickX(), viewport->GetPickY(),
                     viewport->GetPickWidth(),
                     viewport->GetPickHeight(),
                     viewport->GetOrigin(), vsize);
  }
  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();
  // If picking then set up a model view matrix
  if(viewport->GetIsPicking())
  {
    glOrtho(0,vsize[0] -1, 0, vsize[1] -1, 0, 1);
  }

  glDisable( GL_LIGHTING);

  // Get the position of the image actor
  int* actorPos =
    actor->GetActualPositionCoordinate()->GetComputedViewportValue(viewport);
  int* actorPos2 =
    actor->GetActualPosition2Coordinate()->GetComputedViewportValue(viewport);
  // negative positions will already be clipped to viewport
  actorPos[0] += this->PositionAdjustment[0];
  actorPos[1] += this->PositionAdjustment[1];
  // if picking then only draw a polygon, since an image can not be picked
  if(viewport->GetIsPicking())
  {
    int inMin0 = this->DisplayExtent[0];
    int inMax0 = this->DisplayExtent[1];
    int inMin1 = this->DisplayExtent[2];
    int inMax1 = this->DisplayExtent[3];

    float width = inMax0 - inMin0 + 1;
    float height = inMax1 - inMin1 + 1;
    float x1 = (2.0 * actorPos[0]) / vsize[0] - 1.0;
    float y1 = (2.0 * actorPos[1]) / vsize[1] - 1.0;
    glRectf(x1, y1, x1+width, y1+height);
    // clean up and return
    glMatrixMode( GL_PROJECTION);
    glPopMatrix();
    glMatrixMode( GL_MODELVIEW);
    glPopMatrix();
    glEnable( GL_LIGHTING);
    vtkOpenGLCheckErrorMacro("failed after RenderData");
    return;
  }

  int front =
    (actor->GetProperty()->GetDisplayLocation() == VTK_FOREGROUND_LOCATION);

  switch (data->GetPointData()->GetScalars()->GetDataType())
  {
    vtkTemplateMacro(
      vtkOpenGLImageMapperRender(this, data, static_cast<VTK_TT*>(ptr0),
                                 shift, scale, actorPos, actorPos2, front, vsize,
                                 1)
      );
    default:
      vtkErrorMacro ( << "Unsupported image type: " << data->GetScalarType());
  }

  glMatrixMode( GL_PROJECTION);
  glPopMatrix();
  glMatrixMode( GL_MODELVIEW);
  glPopMatrix();
  glEnable( GL_LIGHTING);

  vtkOpenGLCheckErrorMacro("failed after RenderData");
}

void vtkOpenGLImageMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
