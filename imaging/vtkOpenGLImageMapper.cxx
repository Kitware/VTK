#include "vtkOpenGLImageMapper.h"
#include "vtkProperty2D.h"
#include <GL/gl.h>
#include <limits.h>

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
				       int type,
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
  int inInc0 = tempIncs[0];
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
					    int type,
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
  int inInc0 = tempIncs[0];
  int inInc1 = tempIncs[1];

  int bpp = data->GetNumberOfScalarComponents();

  double range[2];
  data->GetPointData()->GetScalars()->GetDataTypeRange( range );

  glRasterPos3f((2.0 * (GLfloat)(actorPos[0]) / vsize[0] - 1), 
		(2.0 * (GLfloat)(actorPos[1]) / vsize[1] - 1), -1);

  glPixelStorei( GL_UNPACK_ALIGNMENT, 1);

  // find the number of bits to use for the fraction:
  // continue increasing the bits until there is an overflow
  // in the worst case, then decrease by 1.
  int bitShift = 0;
  while (((long)(1 << bitShift)*scale)*USHRT_MAX*2.0 < LONG_MAX)
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
	  *ptr++;
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

static void vtkOpenGLImageMapperRenderChar(vtkOpenGLImageMapper *self, 
					   vtkImageData *data, 
					   unsigned char *dataPtr,
					   int type,
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
  int inInc0 = tempIncs[0];
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
    glDrawPixels(width, height, GL_RGB, type, (void *)dataPtr);
    }
  else if (bpp == 4)
    { // feed through RGBA bytes without reformatting
    if (inInc1 != width*bpp)
      {
      glPixelStorei( GL_UNPACK_ROW_LENGTH, inInc1/bpp );
      }
    glDrawPixels(width, height, GL_RGBA, type, (void *)dataPtr);
    }      
  else 
    { // feed through other bytes without reformatting
    unsigned char *inPtr = (unsigned char *)dataPtr;
    unsigned char *inPtr1 = inPtr;
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
  int dataWidth, width, height;
  int size;
  unsigned char *dataOut;
  void *ptr0, *ptr1, *ptr2;
  float shift, scale;

  vtkWindow* window = (vtkWindow *) viewport->GetVTKWindow();
  if (!window)
    {
    vtkErrorMacro (<<"vtkOpenGLImageMapper::RenderData - no window set for viewport");
    return;
    }

  // Determine the size of the displayed data.
  int* extent = this->Input->GetUpdateExtent();
  width = (extent[1] - extent[0] + 1);
  height = (extent[3] - extent[2] + 1);
  
  shift = this->GetColorShift();
  scale = this->GetColorScale();
  
  int dim = 0;
  dim = data->GetNumberOfScalarComponents();
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
    case VTK_FLOAT:  
      vtkOpenGLImageMapperRender(this, data,
				 (float *)(ptr0), 
				 GL_FLOAT,
				 shift, scale, actorPos, vsize);
      break;
    case VTK_INT:    
      vtkOpenGLImageMapperRender(this, data,
				 (int *)(ptr0),
				 GL_INT,
				 shift, scale, actorPos, vsize);
      break;
    case VTK_SHORT:  
      vtkOpenGLImageMapperRenderShort(this, data,
				      (short *)(ptr0),
				      GL_SHORT,
				      shift, scale, actorPos, vsize);
      
      break; 
    case VTK_UNSIGNED_SHORT: 
      vtkOpenGLImageMapperRenderShort(this, data,
				      (unsigned short *)(ptr0),  
				      GL_UNSIGNED_SHORT,
				      shift, scale, actorPos, vsize);
    
      break; 
    case VTK_UNSIGNED_CHAR:  
      if (shift == 0.0 && scale == 1.0)
	{
	vtkOpenGLImageMapperRenderChar(this, data,
				       (unsigned char *)(ptr0),  
				       GL_UNSIGNED_BYTE,
				       actorPos, vsize);
	}
      else
	{
	vtkOpenGLImageMapperRenderShort(this, data,
					(unsigned char *)(ptr0),  
					GL_UNSIGNED_BYTE,
					shift, scale, actorPos, vsize);
	}
      break;
    }

  glMatrixMode( GL_PROJECTION);
  glPopMatrix();
  glMatrixMode( GL_MODELVIEW);
  glPopMatrix();
  glEnable( GL_LIGHTING);
}










