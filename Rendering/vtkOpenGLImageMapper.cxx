/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLImageMapper.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

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

#include "vtkOpenGLImageMapper.h"
#include "vtkProperty2D.h"
#include "vtkWindow.h"
#include "vtkViewport.h"
#include "vtkActor2D.h"
#ifdef __APPLE__
 #include <OpenGL/gl.h>
#else
 #include <GL/gl.h>
#endif
#include <limits.h>
#include "vtkObjectFactory.h"
#include "vtkgluPickMatrix.h"


//------------------------------------------------------------------------------
vtkOpenGLImageMapper* vtkOpenGLImageMapper::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkOpenGLImageMapper");
  if(ret)
    {
    return (vtkOpenGLImageMapper*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkOpenGLImageMapper;
}

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
  (x) = (unsigned char)(val); \
} 
/* should do proper rounding, as follows:
  (x) = (unsigned char)(val + 0.5f); \
*/

// the bit-shift must be done after the comparison to zero
// because bit-shift is implemenation dependant for negative numbers
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
  (x) = (unsigned char)(val); \
} 

// pad an integer to a multiply of four, for OpenGL
static inline int vtkPadToFour(int n)
{
  return (((n+3)/4)*4);
}

//---------------------------------------------------------------
// render the image by doing the following:
// 1) apply shift and scale to pixel values
// 2) clamp to [0,255] and convert to unsigned char
// 3) draw using glDrawPixels

template <class T>
static void vtkOpenGLImageMapperRender(
        vtkOpenGLImageMapper *self, vtkImageData *data, T *dataPtr,
        float shift, float scale,
        int *actorPos, int *actorPos2, int front, int *vsize)
{
  int inMin0 = self->DisplayExtent[0];
  int inMax0 = self->DisplayExtent[1];
  int inMin1 = self->DisplayExtent[2];
  int inMax1 = self->DisplayExtent[3];

  int width = inMax0 - inMin0 + 1;
  int height = inMax1 - inMin1 + 1;

  int* tempIncs = data->GetIncrements();
  int inInc1 = tempIncs[1];

  int bpp = data->GetNumberOfScalarComponents();
  double range[2];
  data->GetPointData()->GetScalars()->GetDataTypeRange( range );

  // the value .999 is sensitive to z-buffer depth
  glRasterPos3f((2.0 * (GLfloat)(actorPos[0]) / vsize[0] - 1),
                (2.0 * (GLfloat)(actorPos[1]) / vsize[1] - 1),
                (front)?(-1):(.999));

  glPixelStorei( GL_UNPACK_ALIGNMENT, 1);

  // reformat data into unsigned char

  T *inPtr = (T *)dataPtr;
  T *inPtr1 = inPtr;

  int i = width;
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
  float val;
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
    float xscale = (float)rectwidth/width;
    float yscale = (float)rectheight/height;
    glPixelZoom(xscale, yscale);
    }

  glDrawPixels(width, height, ((bpp < 4) ? GL_RGB : GL_RGBA),
	       GL_UNSIGNED_BYTE, (void *)newPtr);
  
  delete [] newPtr;    
}

//---------------------------------------------------------------
// Same as above, but uses fixed-point math for shift and scale.
// The number of bits used for the fraction is determined from the
// scale.  Enough bits are always left over for the integer that
// overflow cannot occur.

template <class T>
static void vtkOpenGLImageMapperRenderShort(
        vtkOpenGLImageMapper *self, vtkImageData *data, T *dataPtr,
        float shift, float scale,
        int *actorPos, int *actorPos2, int front, int *vsize)
{
  int inMin0 = self->DisplayExtent[0];
  int inMax0 = self->DisplayExtent[1];
  int inMin1 = self->DisplayExtent[2];
  int inMax1 = self->DisplayExtent[3];

  int width = inMax0 - inMin0 + 1;
  int height = inMax1 - inMin1 + 1;

  int* tempIncs = data->GetIncrements();
  int inInc1 = tempIncs[1];

  int bpp = data->GetNumberOfScalarComponents();

  double range[2];
  data->GetPointData()->GetScalars()->GetDataTypeRange( range );
  
  // the value .999 is sensitive to z-buffer depth
  glRasterPos3f((2.0 * (GLfloat)(actorPos[0]) / vsize[0] - 1), 
                (2.0 * (GLfloat)(actorPos[1]) / vsize[1] - 1), 
                (front)?(-1):(.999));

  glPixelStorei( GL_UNPACK_ALIGNMENT, 1);

  // Find the number of bits to use for the fraction:
  // continue increasing the bits until there is an overflow
  // in the worst case, then decrease by 1.
  // The "*2.0" and "*1.0" ensure that the comparison is done
  // with double-precision math.
  int bitShift = 0;
  double absScale = ((scale < 0) ? -scale : scale); 

  while (((long)(1 << bitShift)*absScale)*2.0*USHRT_MAX < INT_MAX*1.0)
    {
    bitShift++;
    }
  bitShift--;
  
  long sscale = (long) (scale*(1 << bitShift));
  long sshift = (long) (sscale*shift);
  /* should do proper rounding, as follows:
  long sscale = (long) floor(scale*(1 << bitShift) + 0.5);
  long sshift = (long) floor((scale*shift + 0.5)*(1 << bitShift));
  */
  long val;
  unsigned char tmp;
  
  T *inPtr = (T *)dataPtr;
  T *inPtr1 = inPtr;
  
  int i = width;
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
    float xscale = (float)rectwidth/width;
    float yscale = (float)rectheight/height;
    glPixelZoom(xscale, yscale);
    }

  glDrawPixels(width, height, ((bpp < 4) ? GL_RGB : GL_RGBA),
	       GL_UNSIGNED_BYTE, (void *)newPtr);

  delete [] newPtr;
}

//---------------------------------------------------------------
// render unsigned char data without any shift/scale

template <class T>
static void vtkOpenGLImageMapperRenderChar(
        vtkOpenGLImageMapper *self, vtkImageData *data, T *dataPtr,
        int *actorPos, int *actorPos2, int front, int *vsize)
{
  int inMin0 = self->DisplayExtent[0];
  int inMax0 = self->DisplayExtent[1];
  int inMin1 = self->DisplayExtent[2];
  int inMax1 = self->DisplayExtent[3];

  int width = inMax0 - inMin0 + 1;
  int height = inMax1 - inMin1 + 1;

  int* tempIncs = data->GetIncrements();
  int inInc1 = tempIncs[1];

  int bpp = data->GetNumberOfScalarComponents();

  double range[2];
  data->GetPointData()->GetScalars()->GetDataTypeRange( range );

  // the value .999 is sensitive to z-buffer depth
  glRasterPos3f((2.0 * (GLfloat)(actorPos[0]) / vsize[0] - 1),
                (2.0 * (GLfloat)(actorPos[1]) / vsize[1] - 1),
                (front)?(-1):(.999));


  glPixelStorei( GL_UNPACK_ALIGNMENT, 1);

  if (self->GetRenderToRectangle())
    {
    int rectwidth  = (actorPos2[0] - actorPos[0]) + 1;
    int rectheight = (actorPos2[1] - actorPos[1]) + 1;
    float xscale = (float)rectwidth/width;
    float yscale = (float)rectheight/height;
    glPixelZoom(xscale, yscale);
    }
  //
  if (bpp == 3)
    { // feed through RGB bytes without reformatting
    if (inInc1 != width*bpp)
      {
      glPixelStorei( GL_UNPACK_ROW_LENGTH, inInc1/bpp );
      }
    glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, (void *)dataPtr);
    }
  else if (bpp == 4)
    { // feed through RGBA bytes without reformatting
    if (inInc1 != width*bpp)
      {
      glPixelStorei( GL_UNPACK_ROW_LENGTH, inInc1/bpp );
      }
    glDrawPixels(width, height, GL_RGBA, GL_UNSIGNED_BYTE, (void *)dataPtr);
    }
  else
    { // feed through other bytes without reformatting
    T *inPtr = (T *)dataPtr;
    T *inPtr1 = inPtr;
    unsigned char tmp;

    int i = width;
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
		 GL_UNSIGNED_BYTE, (void *)newPtr);

    delete [] newPtr;
    }
  glPixelStorei( GL_UNPACK_ROW_LENGTH, 0);
}

//----------------------------------------------------------------------------
// Expects data to be X, Y, components

void vtkOpenGLImageMapper::RenderData(vtkViewport* viewport,
				     vtkImageData *data, vtkActor2D *actor)
{
  void *ptr0;
  float shift, scale;

  vtkWindow* window = (vtkWindow *) viewport->GetVTKWindow();
  if (!window)
    {
    vtkErrorMacro (<<"vtkOpenGLImageMapper::RenderData - no window set for viewport");
    return;
    }

  // Make this window current. May have become not current due to
  // data updates since the render started.
  window->MakeCurrent();
  
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
		     1, 1, viewport->GetOrigin(), vsize);
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

  // Get the position of the text actor
  int* actorPos =
    actor->GetPositionCoordinate()->GetComputedViewportValue(viewport);
  int* actorPos2 =
    actor->GetPosition2Coordinate()->GetComputedViewportValue(viewport);
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
    float x1 = (2.0 * (GLfloat)(actorPos[0]) / vsize[0] - 1);
    float y1 = (2.0 * (GLfloat)(actorPos[1]) / vsize[1] - 1);
    glRectf(x1, y1, x1+width, y1+height);
    // clean up and return 
    glMatrixMode( GL_PROJECTION);
    glPopMatrix();
    glMatrixMode( GL_MODELVIEW);
    glPopMatrix();
    glEnable( GL_LIGHTING);
    return;
    }
  
  int front = 
    (actor->GetProperty()->GetDisplayLocation() == VTK_FOREGROUND_LOCATION);

  switch (data->GetScalarType())
    {
    case VTK_DOUBLE:  
      vtkOpenGLImageMapperRender(this, data,
				 (double *)(ptr0),
				 shift, scale, actorPos, actorPos2, front, vsize);
      break;
    case VTK_FLOAT:
      vtkOpenGLImageMapperRender(this, data,
				 (float *)(ptr0),
				 shift, scale, actorPos, actorPos2, front, vsize);
      break;
    case VTK_LONG:
      vtkOpenGLImageMapperRender(this, data,
				 (long *)(ptr0),
				 shift, scale, actorPos, actorPos2, front, vsize);
      break;
    case VTK_UNSIGNED_LONG:
      vtkOpenGLImageMapperRender(this, data,
				 (unsigned long *)(ptr0),
				 shift, scale, actorPos, actorPos2, front, vsize);
      break;
    case VTK_INT:
      vtkOpenGLImageMapperRender(this, data,
				 (int *)(ptr0),
				 shift, scale, actorPos, actorPos2, front, vsize);
      break;
    case VTK_UNSIGNED_INT:
      vtkOpenGLImageMapperRender(this, data,
          (unsigned int *)(ptr0),
          shift, scale, actorPos, actorPos2, front, vsize);

      break;
    case VTK_SHORT:
      vtkOpenGLImageMapperRenderShort(this, data,
          (short *)(ptr0),
          shift, scale, actorPos, actorPos2, front, vsize);

      break;
    case VTK_UNSIGNED_SHORT:
      vtkOpenGLImageMapperRenderShort(this, data,
          (unsigned short *)(ptr0),
          shift, scale, actorPos, actorPos2, front, vsize);

      break;
    case VTK_UNSIGNED_CHAR:
      if (shift == 0.0 && scale == 1.0)
	      {
	      vtkOpenGLImageMapperRenderChar(this, data,
            (unsigned char *)(ptr0),
            actorPos, actorPos2, front, vsize);
	      }
      else
	      {
	      // RenderShort is Templated, so we can pass unsigned char
	      vtkOpenGLImageMapperRenderShort(this, data,
            (unsigned char *)(ptr0),
				    shift, scale, actorPos, actorPos2, front, vsize);
	      }
      break;
    case VTK_CHAR:
      if (shift == 0.0 && scale == 1.0)
	      {
	      vtkOpenGLImageMapperRenderChar(this, data,
          (char *)(ptr0),
          actorPos, actorPos2, front, vsize);
	      }
      else
	      {
	      // RenderShort is Templated, so we can pass unsigned char
	      vtkOpenGLImageMapperRenderShort(this, data,
				   (char *)(ptr0),
				   shift, scale, actorPos, actorPos2, front, vsize);
	      }
      break;
    default:
      vtkErrorMacro ( << "Unsupported image type: " << data->GetScalarType());
    }

  glMatrixMode( GL_PROJECTION);
  glPopMatrix();
  glMatrixMode( GL_MODELVIEW);
  glPopMatrix();
  glEnable( GL_LIGHTING);
}










