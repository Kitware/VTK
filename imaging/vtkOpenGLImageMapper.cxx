#include "vtkOpenGLImageMapper.h"
#include "vtkProperty2D.h"
#include <GL/gl.h>

vtkOpenGLImageMapper::vtkOpenGLImageMapper()
{
}

vtkOpenGLImageMapper::~vtkOpenGLImageMapper()
{
}

//----------------------------------------------------------------------------
/* 
 * This templated routine calculates effective lower and upper limits 
 * for a window of values of type T, lower and upper. 
 */
template <class T>
static void vtkOpenGLImageMapperClamps ( vtkImageData *data, float w, 
					float l, T& lower, T& upper, 
					unsigned char &lower_val, 
					unsigned char &upper_val)
{
  double f_lower, f_upper, f_lower_val, f_upper_val;
  double adjustedLower, adjustedUpper;
  double range[2];

  data->GetPointData()->GetScalars()->GetDataTypeRange( range );

  f_lower = l - fabs(w) / 2.0;
  f_upper = f_lower + fabs(w);

  // Set the correct lower value
  if ( f_lower <= range[1])
    {
    if (f_lower >= range[0])
      {
      lower = (T) f_lower;
      adjustedLower = f_lower;
      }
    else
      {
      lower = (T) range[0];
      adjustedLower = range[0];
      }
    }
  else
    {
    lower = (T) range[1];
    adjustedLower = range[1];
    }
  
  
  // Set the correct upper value
  if ( f_upper >= range[0])
    {
    if (f_upper <= range[1])
      {
      upper = (T) f_upper;
      adjustedUpper = f_upper;
      }
    else
      {
      upper = (T) range[1];
      adjustedUpper = range[1];
      }
    }
  else
    {
    upper = (T) range [0];
    adjustedUpper = range [0];
    }
  
  // now compute the lower and upper values
  if (w >= 0)
    {
    f_lower_val = 255.0*(adjustedLower - f_lower)/w;
    f_upper_val = 255.0*(adjustedUpper - f_lower)/w;
    }
  else
    {
    f_lower_val = 255.0 + 255.0*(adjustedLower - f_lower)/w;
    f_upper_val = 255.0 + 255.0*(adjustedUpper - f_lower)/w;
    }
  
  if (f_upper_val > 255) 
    {
    upper_val = 255;
    }
  else if (f_upper_val < 0)
    {
    upper_val = 0;
    }
  else
    {
    upper_val = (unsigned char)(f_upper_val);
    }
  
  if (f_lower_val > 255) 
    {
    lower_val = 255;
    }
  else if (f_lower_val < 0)
    {
    lower_val = 0;
    }
  else
    {
    lower_val = (unsigned char)(f_lower_val);
    }  
}

//----------------------------------------------------------------------------
// A templated function that handles gray scale images.
template <class T>
static void vtkOpenGLImageMapperRenderGray(vtkOpenGLImageMapper *self, 
					   vtkImageData *data,
					   T *inPtr, float shift, 
					   float scale, int *actorPos,
					   int * vsize)
{
  unsigned char colorIdx;
  T *inPtr0, *inPtr1, *endPtr;
  int inMin0, inMax0, inMin1, inMax1;
  int inInc0, inInc1;
  int idx1;
  T   lower, upper;
  int *Size;
  unsigned char lower_val, upper_val;
  int width, height;
  
  vtkOpenGLImageMapperClamps ( data, self->GetColorWindow(), 
			      self->GetColorLevel(), 
			      lower, upper, lower_val, upper_val );
  
  int* tempExt = self->GetInput()->GetUpdateExtent();
  inMin0 = tempExt[0];
  inMax0 = tempExt[1];
  inMin1 = tempExt[2];
  inMax1 = tempExt[3];

  int* tempIncs = data->GetIncrements();
  inInc0 = tempIncs[0];
  inInc1 = tempIncs[1];

  // Loop through in regions pixels
  inPtr1 = inPtr;
  width = inMax0 - inMin0 + 1;
  height = inMax1 - inMin1 + 1;
  
  unsigned char *outLinePtr = new unsigned char [(3*width)*height];
  unsigned char *outPtr = outLinePtr;

  glRasterPos3f((2.0 * (GLfloat)(actorPos[0]) / vsize[0] - 1), 
		(2.0 * (GLfloat)(actorPos[1]) / vsize[1] - 1), -1);

  for (idx1 = inMin1; idx1 <= inMax1; idx1++)
    {
    inPtr0 = inPtr1;
    endPtr = inPtr0 + inInc0*width;
    while (inPtr0 != endPtr)
      {
      if (*inPtr0 <= lower) 
	{
	*outPtr++ = lower_val;
	*outPtr++ = lower_val;
	*outPtr++ = lower_val;
	}
      else if (*inPtr0 >= upper)
	{
	*outPtr++ = upper_val;
	*outPtr++ = upper_val;
	*outPtr++ = upper_val;
	}
      else
	{
	// Because i_lower and sscale are of integer type
	// this is fast for all types used by this
	// template (float is treated separately).
	colorIdx = (int) ((*inPtr0 + shift)*scale);
	*outPtr++ = colorIdx;
	*outPtr++ = colorIdx;
	*outPtr++ = colorIdx;
	}
      inPtr0 += inInc0;
      }
    // rows must be a multiple of four bytes
    // so pad it if neccessary
    inPtr1 += inInc1;
    }
  glPixelStorei( GL_UNPACK_ALIGNMENT, 1);
  glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, outLinePtr);
  delete [] outLinePtr;
}


//----------------------------------------------------------------------------
// A templated function that handles color images. (only True Color 24 bit)
template <class T>
static void vtkOpenGLImageMapperRenderColor(vtkOpenGLImageMapper *self, 
					    vtkImageData *data, T *redPtr,
					    int bpp,float shift, float scale,
					    int *actorPos, int *vsize)
{
  unsigned char red, green, blue;
  T *redPtr0, *redPtr1;
  T *bluePtr0, *bluePtr1;
  T *greenPtr0, *greenPtr1;
  int inMin0, inMax0, inMin1, inMax1;
  int inInc0, inInc1;
  int idx0, idx1;
  T   lower, upper;
  T *greenPtr; 
  T *bluePtr;  
  unsigned char lower_val, upper_val;
  int width, height;
  
  // data->GetExtent(inMin0, inMax0, inMin1, inMax1);
  int* tempExt = self->GetInput()->GetUpdateExtent();
  inMin0 = tempExt[0];
  inMax0 = tempExt[1];
  inMin1 = tempExt[2];
  inMax1 = tempExt[3];

  // data->GetIncrements(inInc0, inInc1);
  int* tempIncs = data->GetIncrements();
  inInc0 = tempIncs[0];
  inInc1 = tempIncs[1];
  
  if (bpp >= 2)
    {
    greenPtr = redPtr + 1;
    }
  else
    {
    greenPtr = redPtr;
    }
  if (bpp >= 3)
    {
    bluePtr = redPtr + 2;
    }
  else
    {
    bluePtr = redPtr;
    }

  vtkOpenGLImageMapperClamps ( data, self->GetColorWindow(), 
			      self->GetColorLevel(), 
			      lower, upper, lower_val, upper_val );

  width = inMax0 - inMin0 + 1;
  height = inMax1 - inMin1 + 1;
  unsigned char *outLinePtr = new unsigned char [(3*width)*height];
  unsigned char *outPtr = outLinePtr;

  glRasterPos3f((2.0 * (GLfloat)(actorPos[0]) / vsize[0] - 1), 
		(2.0 * (GLfloat)(actorPos[1]) / vsize[1] - 1), -1);

  // Loop through in regions pixels
  redPtr1 = redPtr;
  greenPtr1 = greenPtr;
  bluePtr1 = bluePtr;
  for (idx1 = inMin1; idx1 <= inMax1; idx1++)
    {
    redPtr0 = redPtr1;
    greenPtr0 = greenPtr1;
    bluePtr0 = bluePtr1;
    for (idx0 = inMin0; idx0 <= inMax0; idx0++)
      {
      if (*redPtr0 <= lower)
	{
	red = lower_val;
	}
      else if (*redPtr0 >= upper)
	{
	red = upper_val;
	}
      else
	{
	red = (unsigned char)(((float)(*redPtr0) + shift) * scale);
	}

      if (*greenPtr0 <= lower)
	{
	green = lower_val;
	}
      else if (*greenPtr0 >= upper)
	{
	green = upper_val;
	}
      else
	{
	green = (unsigned char)(((float)(*greenPtr0) + shift) * scale);
	}
  
      if (*bluePtr0 <= lower)
	{
	blue = lower_val;
	}
      else if (*bluePtr0 >= upper)
	{
	blue = upper_val;
	}
      else
	{
	blue = (unsigned char)(((float)(*bluePtr0) + shift) * scale);
	}
      *outPtr++ = red;
      *outPtr++ = green;
      *outPtr++ = blue;

      redPtr0 += inInc0;
      greenPtr0 += inInc0;
      bluePtr0 += inInc0;
      }
    redPtr1 += inInc1;
    greenPtr1 += inInc1;
    bluePtr1 += inInc1;
    }
  glPixelStorei( GL_UNPACK_ALIGNMENT, 1);
  glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, outLinePtr);
  delete [] outLinePtr;
}

//----------------------------------------------------------------------------
// A templated function that handles gray scale images.

template <class T>
static void vtkOpenGLImageMapperRenderShortGray(vtkOpenGLImageMapper *self,
						vtkImageData *data,
						T *inPtr,
						float shift, float scale,
						int *actorPos, 
						int *vsize)
{
  unsigned char colorIdx;
  T *inPtr0, *inPtr1, *endPtr;
  int inMin0, inMax0, inMin1, inMax1;
  int inInc0, inInc1;
  int idx1;
  T   lower, upper;
  long sscale, sshift;
  unsigned char lower_val, upper_val;
  int width, height;
  
  vtkOpenGLImageMapperClamps ( data, self->GetColorWindow(), 
			      self->GetColorLevel(), 
			      lower, upper, lower_val, upper_val );
  
  sscale = scale*4096.0;
  sshift = sscale*shift;
  
  // data->GetExtent(inMin0, inMax0, inMin1, inMax1);
  int* tempExt = self->GetInput()->GetUpdateExtent();
  inMin0 = tempExt[0];
  inMax0 = tempExt[1];
  inMin1 = tempExt[2];
  inMax1 = tempExt[3];

  // data->GetIncrements(inInc0, inInc1);
  int* tempIncs = data->GetIncrements();
  inInc0 = tempIncs[0];
  inInc1 = tempIncs[1];

  // Loop through in regions pixels
  width = inMax0 - inMin0 + 1;
  height = inMax1 - inMin1 + 1;
  
  unsigned char *outLinePtr = new unsigned char [(3*width)*height];
  unsigned char *outPtr = outLinePtr;

  inPtr1 = inPtr;
  glRasterPos3f((2.0 * (GLfloat)(actorPos[0]) / vsize[0] - 1), 
  	(2.0 * (GLfloat)(actorPos[1]) / vsize[1] - 1), -1);

  for (idx1 = inMin1; idx1 <= inMax1; idx1++)
    {
    inPtr0 = inPtr1;
    endPtr = inPtr0 + inInc0*(inMax0 - inMin0 + 1);
    while (inPtr0 != endPtr)
      {
      if (*inPtr0 <= lower)
        {
        *outPtr++ = lower_val;
        *outPtr++ = lower_val;
        *outPtr++ = lower_val;
        }
      else if (*inPtr0 >= upper)
        {
        *outPtr++ = upper_val;
        *outPtr++ = upper_val;
        *outPtr++ = upper_val;
        }
      else
        {
        colorIdx = ((*inPtr0) * sscale + sshift) >> 12;
        *outPtr++ = colorIdx;
        *outPtr++ = colorIdx;
        *outPtr++ = colorIdx;
        }
      inPtr0 += inInc0;
      }
    // rows must be a multiple of four bytes
    // so pad it if neccessary
    inPtr1 += inInc1;
    }
  glPixelStorei( GL_UNPACK_ALIGNMENT, 1);
  glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, outLinePtr);
  delete [] outLinePtr;
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

  if (dim > 1)    
    {
    // Call the appropriate templated function
    switch (data->GetScalarType())
      {
      case VTK_FLOAT:
        vtkDebugMacro(<<"vtkOpenGLImageMappper::RenderData - RenderColor, Float");
        vtkOpenGLImageMapperRenderColor(this, data,
					(float *)(ptr0), dim,
					shift, scale, actorPos, vsize);
        break;
      case VTK_INT:
        vtkDebugMacro(<<"vtkOpenGLImageMappper::RenderData - RenderColor, Int");
        vtkOpenGLImageMapperRenderColor(this, data,
					(int *)(ptr0), dim,
					shift, scale, actorPos, vsize);
        break;
      case VTK_SHORT:
        vtkDebugMacro(<<"vtkOpenGLImageMappper::RenderData - RenderColor, Short");
        vtkOpenGLImageMapperRenderColor(this, data,
					(short *)(ptr0), dim,
					shift, scale, actorPos, vsize);
        break;
      case VTK_UNSIGNED_SHORT:
        vtkDebugMacro(<<"vtkOpenGLImageMappper::RenderData - RenderColor, Unsigned Short");
        vtkOpenGLImageMapperRenderColor(this, data, (unsigned short *)(ptr0),
					dim, shift, scale, actorPos, vsize);
        break;
      case VTK_UNSIGNED_CHAR:
        vtkDebugMacro(<<"vtkOpenGLImageMappper::RenderData - RenderColor, Unsigned Char");
        vtkOpenGLImageMapperRenderColor(this, data, (unsigned char *)(ptr0),
				       dim, shift, scale, actorPos, vsize);
        break;
      }
    }
  else
    {
    // GrayScale images.
    // Call the appropriate templated function
    switch (data->GetScalarType())
      {
      case VTK_FLOAT:
	vtkOpenGLImageMapperRenderGray(this, data, (float *)(ptr0), 
				       shift, scale, actorPos, vsize);
	break;
      case VTK_INT:
	vtkOpenGLImageMapperRenderGray(this, data, (int *)(ptr0), 
				       shift, scale, actorPos, vsize);
	break;
      case VTK_SHORT:
	vtkOpenGLImageMapperRenderShortGray(this, data, (short *)(ptr0),
					    shift, scale, actorPos, vsize);
	break;
      case VTK_UNSIGNED_SHORT:
	vtkOpenGLImageMapperRenderShortGray(this, data, 
					   (unsigned short *)(ptr0),
					    shift, scale, actorPos, vsize);
	break;
      case VTK_UNSIGNED_CHAR:
	vtkOpenGLImageMapperRenderShortGray(this, data, 
					   (unsigned char *)(ptr0),
					    shift, scale, actorPos, vsize);
	break;
      }
    }

  glMatrixMode( GL_PROJECTION);
  glPopMatrix();
  glMatrixMode( GL_MODELVIEW);
  glPopMatrix();
  glEnable( GL_LIGHTING);
}








