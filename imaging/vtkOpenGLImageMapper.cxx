#include "vtkOpenGLImageMapper.h"
#include "vtkProperty2D.h"
#include <GL/gl.h>

vtkOpenGLImageMapper::vtkOpenGLImageMapper()
{
}

vtkOpenGLImageMapper::~vtkOpenGLImageMapper()
{
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
				       int format, int type,
				       float shift, float scale,
				       int *actorPos, int *vsize)
{
  int* tempExt = self->GetInput()->GetUpdateExtent();
  int width = tempExt[1] - tempExt[0] + 1;
  int height = tempExt[3] - tempExt[2] + 1;
  int bpp = data->GetNumberOfScalarComponents();
  double range[2];
  data->GetPointData()->GetScalars()->GetDataTypeRange( range );

  glRasterPos3f((2.0 * (GLfloat)(actorPos[0]) / vsize[0] - 1), 
		(2.0 * (GLfloat)(actorPos[1]) / vsize[1] - 1), -1);

  if (scale == 255.0/range[1] && shift == 0.0 && 
      (bpp == 1 || bpp == 3))
    { // don't reformat data if no window/level was applied
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1);
    glDrawPixels(width, height, format, type, (void *)dataPtr);
    }
  else
    { // reformat data into unsigned char
    T *oldPtr = (T *)dataPtr;
    int i = width*height;

    unsigned char *newPtr;
    if (format == GL_LUMINANCE)
      newPtr = new unsigned char[i];
    else
      newPtr = new unsigned char[3*i];

    unsigned char *ptr = newPtr;
    float val;
    unsigned char tmp;

    switch (bpp)
      {
      case 1:
	while (--i >= 0)
	  {
	  val = ((*oldPtr++ + shift)*scale);
	  if (val < 0) val = 0;
	  if (val > 255) val = 255;
	  *ptr++ = (unsigned char)val;
	  }
	break;

      case 2:
	while (--i >= 0)
	  {
	  val = ((*oldPtr++ + shift)*scale);
	  if (val < 0) val = 0;
	  if (val > 255) val = 255;
	  *ptr++ = tmp = (unsigned char)val;
	  val = ((*oldPtr++ + shift)*scale);
	  if (val < 0) val = 0;
	  if (val > 255) val = 255;
	  *ptr++ = (unsigned char)val;
	  *ptr++ = tmp;
	  }
	break;

      case 3:
	while (--i >= 0)
	  {
	  val = ((*oldPtr++ + shift)*scale);
	  if (val < 0) val = 0;
	  if (val > 255) val = 255;
	  *ptr++ = (unsigned char)val;
	  val = ((*oldPtr++ + shift)*scale);
	  if (val < 0) val = 0;
	  if (val > 255) val = 255;
	  *ptr++ = (unsigned char)val;
	  val = ((*oldPtr++ + shift)*scale);
	  if (val < 0) val = 0;
	  if (val > 255) val = 255;
	  *ptr++ = (unsigned char)val;
	  }
	break;

      default:
	while (--i >= 0)
	  {
	  val = ((*oldPtr++ + shift)*scale);
	  if (val < 0) val = 0;
	  if (val > 255) val = 255;
	  *ptr++ = (unsigned char)val;
	  val = ((*oldPtr++ + shift)*scale);
	  if (val < 0) val = 0;
	  if (val > 255) val = 255;
	  *ptr++ = (unsigned char)val;
	  val = ((*oldPtr++ + shift)*scale);
	  if (val < 0) val = 0;
	  if (val > 255) val = 255;
	  *ptr++ = (unsigned char)val;
	  oldPtr += bpp-3;
	  }
	break;

      }

    glPixelStorei( GL_UNPACK_ALIGNMENT, 1);
    glDrawPixels(width, height, format, GL_UNSIGNED_BYTE, newPtr);

    delete [] newPtr;    
    }
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
					    int format, int type,
					    float shift, float scale,
					    int *actorPos, int *vsize)
{
  int* tempExt = self->GetInput()->GetUpdateExtent();
  int width = tempExt[1] - tempExt[0] + 1;
  int height = tempExt[3] - tempExt[2] + 1;
  int bpp = data->GetNumberOfScalarComponents();

  double range[2];
  data->GetPointData()->GetScalars()->GetDataTypeRange( range );

  glRasterPos3f((2.0 * (GLfloat)(actorPos[0]) / vsize[0] - 1), 
		(2.0 * (GLfloat)(actorPos[1]) / vsize[1] - 1), -1);


  if (scale == 255.0/range[1] && shift == 0.0 &&
      (bpp == 1 || bpp == 3))
    { // don't reformat data if no window/level was applied
    glPixelStorei( GL_UNPACK_ALIGNMENT, 1);
    glDrawPixels(width, height, format, type, (void *)dataPtr);
    }
  else
    {
    // find the number of bits to use for the fraction:
    // continue increasing the bits until there is an overflow
    // in the worst case, then decrease by 1.
    int fixedPoints = 0;
    while (((long)(1 << fixedPoints)*scale)*65535.0*2 < 2147483647.0)
      fixedPoints++;
    fixedPoints--;

    long sscale = (long) (scale*(1 << fixedPoints));
    long sshift = (long) (sscale*shift);
    long val;
    unsigned char tmp;

    T *oldPtr = (T *)dataPtr;
    int i = width*height;

    unsigned char *newPtr;
    if (format == GL_LUMINANCE)
      newPtr = new unsigned char[i];
    else
      newPtr = new unsigned char[3*i];

    unsigned char *ptr = newPtr;

    switch (bpp)
      {
      case 1:
	while (--i >= 0)
	  {
	  val = (*oldPtr++ * sscale + sshift) >> fixedPoints;
	  if (val < 0) val = 0;
	  if (val > 255) val = 255;	
	  *ptr++ = (unsigned char)val;
	  }
	break;

      case 2:
	while (--i >= 0)
	  {
	  val = (*oldPtr++ * sscale + sshift) >> fixedPoints;
	  if (val < 0) val = 0;
	  if (val > 255) val = 255;	
	  *ptr++ = tmp = (unsigned char)val;
	  val = (*oldPtr++ * sscale + sshift) >> fixedPoints;
	  if (val < 0) val = 0;
	  if (val > 255) val = 255;	
	  *ptr++ = (unsigned char)val;
	  *ptr++ = tmp;
	  }
	break;

      case 3:
	while (--i >= 0)
	  {
	  val = (*oldPtr++ * sscale + sshift) >> fixedPoints;
	  if (val < 0) val = 0;
	  if (val > 255) val = 255;	
	  *ptr++ = (unsigned char)val;
	  val = (*oldPtr++ * sscale + sshift) >> fixedPoints;
	  if (val < 0) val = 0;
	  if (val > 255) val = 255;	
	  *ptr++ = (unsigned char)val;
	  val = (*oldPtr++ * sscale + sshift) >> fixedPoints;
	  if (val < 0) val = 0;
	  if (val > 255) val = 255;	
	  *ptr++ = (unsigned char)val;
	  }
	break;

      default:
	while (--i >= 0)
	  {
	  val = (*oldPtr++ * sscale + sshift) >> fixedPoints;
	  if (val < 0) val = 0;
	  if (val > 255) val = 255;	
	  *ptr++ = (unsigned char)val;
	  val = (*oldPtr++ * sscale + sshift) >> fixedPoints;
	  if (val < 0) val = 0;
	  if (val > 255) val = 255;	
	  *ptr++ = (unsigned char)val;
	  val = (*oldPtr++ * sscale + sshift) >> fixedPoints;
	  if (val < 0) val = 0;
	  if (val > 255) val = 255;	
	  *ptr++ = (unsigned char)val;
	  oldPtr += bpp-3;
	  }
	break;

      }

    glPixelStorei( GL_UNPACK_ALIGNMENT, 1);
    glDrawPixels(width, height, format, GL_UNSIGNED_BYTE, (void *)newPtr);

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

  int format;

  if (dim == 1)
    format = GL_LUMINANCE; 
  else
    format = GL_RGB;

  switch (data->GetScalarType())
    {
    case VTK_FLOAT:  
      vtkOpenGLImageMapperRender(this, data,
				 (float *)(ptr0), format, 
				 GL_FLOAT,
				 shift, scale, actorPos, vsize);
      break;
    case VTK_INT:    
      vtkOpenGLImageMapperRender(this, data,
				 (int *)(ptr0), format, 
				 GL_INT,
				 shift, scale, actorPos, vsize);
      break;
    case VTK_SHORT:  
      vtkOpenGLImageMapperRenderShort(this, data,
				      (short *)(ptr0), format, 
				      GL_SHORT,
				      shift, scale, actorPos, vsize);
      
      break; 
    case VTK_UNSIGNED_SHORT: 
      vtkOpenGLImageMapperRenderShort(this, data,
				      (unsigned short *)(ptr0), format, 
				      GL_UNSIGNED_SHORT,
				      shift, scale, actorPos, vsize);
    
      break; 
    case VTK_UNSIGNED_CHAR:  
      vtkOpenGLImageMapperRenderShort(this, data,
				      (unsigned char *)(ptr0), format, 
				      GL_UNSIGNED_BYTE,
				      shift, scale, actorPos, vsize);

      break;
    }


  glMatrixMode( GL_PROJECTION);
  glPopMatrix();
  glMatrixMode( GL_MODELVIEW);
  glPopMatrix();
  glEnable( GL_LIGHTING);
}








