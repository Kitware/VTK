/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOpenGLImageMapper.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/

#include "vtkOpenGLImageMapper.h"
#include "vtkProperty2D.h"
#include "vtkWindow.h"
#include "vtkViewport.h"
#include "vtkActor2D.h"
#include <GL/gl.h>
#include <limits.h>
#include "vtkObjectFactory.h"



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

//---------------------------------------------------------------
// render the image by doing the following:
// 1) apply shift and scale to pixel values
// 2) clamp to [0,255] and convert to unsigned char
// 3) draw using glDrawPixels

template <class T>
static void vtkOpenGLImageMapperRender(vtkOpenGLImageMapper *self, 
				       vtkImageData *data, 
				       T *dataPtr,
				       float shift, float scale,
				       int *actorPos, int *vsize)
{
  int* tempExt = self->GetInput()->GetUpdateExtent();
  int inMin0 = tempExt[0];
  int inMax0 = tempExt[1];
  int inMin1 = tempExt[2];
  int inMax1 = tempExt[3];

  int width = inMax0 - inMin0 + 1;
  int height = inMax1 - inMin1 + 1;

  int* tempIncs = data->GetIncrements();
  int inInc1 = tempIncs[1];

  int bpp = data->GetNumberOfScalarComponents();
  double range[2];
  data->GetPointData()->GetScalars()->GetDataTypeRange( range );

  glRasterPos3f((2.0 * (GLfloat)(actorPos[0]) / vsize[0] - 1), 
		(2.0 * (GLfloat)(actorPos[1]) / vsize[1] - 1), -1);
  
  glPixelStorei( GL_UNPACK_ALIGNMENT, 1);

  // reformat data into unsigned char

  T *inPtr = (T *)dataPtr;
  T *inPtr1 = inPtr;

  int i = width;
  int j = height;
 
  unsigned char *newPtr;
  if (bpp < 4)
    {
    newPtr = new unsigned char[3*width*height + (3*width*height)%4];
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
static void vtkOpenGLImageMapperRenderShort(vtkOpenGLImageMapper *self, 
					    vtkImageData *data, 
					    T *dataPtr,
					    float shift, float scale,
					    int *actorPos, int *vsize)
{
  int* tempExt = self->GetInput()->GetUpdateExtent();
  int inMin0 = tempExt[0];
  int inMax0 = tempExt[1];
  int inMin1 = tempExt[2];
  int inMax1 = tempExt[3];

  int width = inMax0 - inMin0 + 1;
  int height = inMax1 - inMin1 + 1;

  int* tempIncs = data->GetIncrements();
  int inInc1 = tempIncs[1];

  int bpp = data->GetNumberOfScalarComponents();

  double range[2];
  data->GetPointData()->GetScalars()->GetDataTypeRange( range );

  glRasterPos3f((2.0 * (GLfloat)(actorPos[0]) / vsize[0] - 1), 
		(2.0 * (GLfloat)(actorPos[1]) / vsize[1] - 1), -1);

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
  long val;
  unsigned char tmp;
  
  T *inPtr = (T *)dataPtr;
  T *inPtr1 = inPtr;
  
  int i = width;
  int j = height;
  
  unsigned char *newPtr;
  if (bpp < 4)
    {
    newPtr = new unsigned char[3*width*height + (3*width*height)%4];
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
  
  glDrawPixels(width, height, ((bpp < 4) ? GL_RGB : GL_RGBA), 
	       GL_UNSIGNED_BYTE, (void *)newPtr);
  
  delete [] newPtr;    
}

//---------------------------------------------------------------
// render unsigned char data without any shift/scale

template <class T>
static void vtkOpenGLImageMapperRenderChar(vtkOpenGLImageMapper *self, 
					   vtkImageData *data, 
					   T *dataPtr,
					   int *actorPos, int *vsize)
{
  int* tempExt = self->GetInput()->GetUpdateExtent();
  int inMin0 = tempExt[0];
  int inMax0 = tempExt[1];
  int inMin1 = tempExt[2];
  int inMax1 = tempExt[3];

  int width = inMax0 - inMin0 + 1;
  int height = inMax1 - inMin1 + 1;

  int* tempIncs = data->GetIncrements();
  int inInc1 = tempIncs[1];

  int bpp = data->GetNumberOfScalarComponents();

  double range[2];
  data->GetPointData()->GetScalars()->GetDataTypeRange( range );

  glRasterPos3f((2.0 * (GLfloat)(actorPos[0]) / vsize[0] - 1), 
		(2.0 * (GLfloat)(actorPos[1]) / vsize[1] - 1), -1);

  glPixelStorei( GL_UNPACK_ALIGNMENT, 1);

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
      newPtr = new unsigned char[3*width*height + (3*width*height)%4];
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

  // Determine the size of the displayed data.
  int* extent = this->Input->GetUpdateExtent();
  
  shift = this->GetColorShift();
  scale = this->GetColorScale();
  
  ptr0 = data->GetScalarPointer(extent[0], extent[2], extent[4]);

  // push a 2D matrix on the stack
  int *vsize = viewport->GetSize();
  glMatrixMode( GL_PROJECTION);
  glPushMatrix();
  glLoadIdentity();
  glMatrixMode( GL_MODELVIEW );
  glPushMatrix();
  glLoadIdentity();

  glDisable( GL_LIGHTING);

  // Get the position of the text actor
  int* actorPos = 
    actor->GetPositionCoordinate()->GetComputedViewportValue(viewport);
  // negative positions will already be clipped to viewport
  actorPos[0] += this->PositionAdjustment[0]; 
  actorPos[1] += this->PositionAdjustment[1];

  switch (data->GetScalarType())
    {
    case VTK_DOUBLE:  
      vtkOpenGLImageMapperRender(this, data,
				 (double *)(ptr0), 
				 shift, scale, actorPos, vsize);
      break;
    case VTK_FLOAT:  
      vtkOpenGLImageMapperRender(this, data,
				 (float *)(ptr0), 
				 shift, scale, actorPos, vsize);
      break;
    case VTK_LONG:    
      vtkOpenGLImageMapperRender(this, data,
				 (long *)(ptr0),
				 shift, scale, actorPos, vsize);
      break;
    case VTK_UNSIGNED_LONG:    
      vtkOpenGLImageMapperRender(this, data,
				 (unsigned long *)(ptr0),
				 shift, scale, actorPos, vsize);
      break;
    case VTK_INT:    
      vtkOpenGLImageMapperRender(this, data,
				 (int *)(ptr0),
				 shift, scale, actorPos, vsize);
      break;
    case VTK_UNSIGNED_INT: 
      vtkOpenGLImageMapperRender(this, data,
                                 (unsigned int *)(ptr0),  
                                 shift, scale, actorPos, vsize);
    
      break; 
    case VTK_SHORT:  
      vtkOpenGLImageMapperRenderShort(this, data,
				      (short *)(ptr0),
				      shift, scale, actorPos, vsize);
      
      break; 
    case VTK_UNSIGNED_SHORT: 
      vtkOpenGLImageMapperRenderShort(this, data,
				      (unsigned short *)(ptr0),  
				      shift, scale, actorPos, vsize);
    
      break; 
    case VTK_UNSIGNED_CHAR:  
      if (shift == 0.0 && scale == 1.0)
	{
	vtkOpenGLImageMapperRenderChar(this, data,
				       (unsigned char *)(ptr0),  
				       actorPos, vsize);
	}
      else
	{
	// RenderShort is Templated, so we can pass unsigned char
	vtkOpenGLImageMapperRenderShort(this, data,
				   (unsigned char *)(ptr0),  
				   shift, scale, actorPos, vsize);
	}
      break;
    case VTK_CHAR: 
      if (shift == 0.0 && scale == 1.0)
	{
	vtkOpenGLImageMapperRenderChar(this, data,
				       (char *)(ptr0),  
				       actorPos, vsize);
	}
      else
	{
	// RenderShort is Templated, so we can pass unsigned char
	vtkOpenGLImageMapperRenderShort(this, data,
				   (char *)(ptr0),  
				   shift, scale, actorPos, vsize);
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










