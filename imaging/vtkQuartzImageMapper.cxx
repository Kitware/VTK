#include "vtkQuartzImageMapper.h"
#include "vtkQuartzImageWindow.h"
#include "vtkProperty2D.h"
#include "vtkObjectFactory.h"



//----------------------------------------------------------------------------
vtkQuartzImageMapper* vtkQuartzImageMapper::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkQuartzImageMapper");
  if(ret)
    {
    return (vtkQuartzImageMapper*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkQuartzImageMapper;
}




vtkQuartzImageMapper::vtkQuartzImageMapper()
{
  this->HBitmap = 0;
  this->DataOut = NULL;

}

vtkQuartzImageMapper::~vtkQuartzImageMapper()
{
  if (this->HBitmap)
    {
//    DeleteObject(this->HBitmap);
    this->HBitmap = 0;
    }
}

/* 
 * This templated routine calculates effective lower and upper limits 
 * for a window of values of type T, lower and upper. 
 */
template <class T>
static void vtkQuartzImageMapperClamps ( vtkImageData *data, float w, 
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
static void vtkQuartzImageMapperRenderGray(vtkQuartzImageMapper *self, 
					  vtkImageData *data,
					  T *inPtr, unsigned char *outPtr,
					  float shift, float scale)
{
  unsigned char colorIdx;
  T *inPtr0, *inPtr1, *endPtr;
  int inMin0, inMax0, inMin1, inMax1;
  int inInc0, inInc1;
  int idx1;
  T   lower, upper;
  int rowAdder;
  int *Size;
  unsigned char lower_val, upper_val;
  
  vtkQuartzImageMapperClamps ( data, self->GetColorWindow(), 
			      self->GetColorLevel(), 
			      lower, upper, lower_val, upper_val );
  
  inMin0 = self->DisplayExtent[0];
  inMax0 = self->DisplayExtent[1];
  inMin1 = self->DisplayExtent[2];
  inMax1 = self->DisplayExtent[3];

  // data->GetIncrements(inInc0, inInc1);
  int* tempIncs = data->GetIncrements();
  inInc0 = tempIncs[0];
  inInc1 = tempIncs[1];

  // Loop through in regions pixels
  rowAdder = (4 - ((inMax0-inMin0 + 1)*3)%4)%4;
  inPtr1 = inPtr;

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
    outPtr += rowAdder;
    inPtr1 += inInc1;
    }
}


//----------------------------------------------------------------------------
// A templated function that handles color images. (only True Color 24 bit)
template <class T>
static void vtkQuartzImageMapperRenderColor(vtkQuartzImageMapper *self, 
					   vtkImageData *data, T *redPtr,
					   int bpp, unsigned char *outPtr,
					   float shift, float scale)
{
  unsigned char red, green, blue;
  T *redPtr0, *redPtr1;
  T *bluePtr0, *bluePtr1;
  T *greenPtr0, *greenPtr1;
  int inMin0, inMax0, inMin1, inMax1;
  int inInc0, inInc1;
  int idx0, idx1;
  T   lower, upper;
  int rowAdder;
  T *greenPtr; 
  T *bluePtr;  
  unsigned char lower_val, upper_val;
  
  // data->GetExtent(inMin0, inMax0, inMin1, inMax1);
  inMin0 = self->DisplayExtent[0];
  inMax0 = self->DisplayExtent[1];
  inMin1 = self->DisplayExtent[2];
  inMax1 = self->DisplayExtent[3];

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

  vtkQuartzImageMapperClamps ( data, self->GetColorWindow(), 
			      self->GetColorLevel(), 
			      lower, upper, lower_val, upper_val );

  // Loop through in regions pixels
  redPtr1 = redPtr;
  greenPtr1 = greenPtr;
  bluePtr1 = bluePtr;
  rowAdder = (4 - ((inMax0-inMin0 + 1)*3)%4)%4;
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
      *outPtr++ = blue;
      *outPtr++ = green;
      *outPtr++ = red;

      redPtr0 += inInc0;
      greenPtr0 += inInc0;
      bluePtr0 += inInc0;
      }
    // rows must be a multiple of four bytes
    // so pad it if neccessary
    outPtr += rowAdder;

    redPtr1 += inInc1;
    greenPtr1 += inInc1;
    bluePtr1 += inInc1;
    }
}







//----------------------------------------------------------------------------
// A templated function that handles gray scale images.

template <class T>
static void vtkQuartzImageMapperRenderShortGray(vtkQuartzImageMapper *self,
					       vtkImageData *data,
					       T *inPtr, unsigned char *outPtr,
					       float shift, float scale)
{
  unsigned char colorIdx;
  T *inPtr0, *inPtr1, *endPtr;
  int inMin0, inMax0, inMin1, inMax1;
  int inInc0, inInc1;
  int idx1;
  T   lower, upper;
  long sscale, sshift;
  int rowAdder;
  unsigned char lower_val, upper_val;
  
  vtkQuartzImageMapperClamps ( data, self->GetColorWindow(), 
			      self->GetColorLevel(), 
			      lower, upper, lower_val, upper_val );
  
  sscale = (long)scale*4096.0;
  sshift = (long)sscale*shift;
  
  // data->GetExtent(inMin0, inMax0, inMin1, inMax1);
  inMin0 = self->DisplayExtent[0];
  inMax0 = self->DisplayExtent[1];
  inMin1 = self->DisplayExtent[2];
  inMax1 = self->DisplayExtent[3];

  // data->GetIncrements(inInc0, inInc1);
  int* tempIncs = data->GetIncrements();
  inInc0 = tempIncs[0];
  inInc1 = tempIncs[1];

  // Loop through in regions pixels
  rowAdder = (4 - ((inMax0-inMin0 + 1)*3)%4)%4;
  inPtr1 = inPtr;
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
    outPtr += rowAdder;
    inPtr1 += inInc1;
    }

}


//----------------------------------------------------------------------------
// Expects data to be X, Y, components

void vtkQuartzImageMapper::RenderData(vtkViewport* viewport, 
				     vtkImageData *data, vtkActor2D *actor)
{
  int dataWidth, width, height;
  int size;
  // int extent[6];
  unsigned char *dataOut;
  void *ptr0, *ptr1, *ptr2;
  float shift, scale;
  void *compatDC;
  void *hOldBitmap;
//  static BITMAPINFO dataHeader;

  vtkWindow* window = (vtkWindow *) viewport->GetVTKWindow();

  if (!window)
    {
    vtkErrorMacro (<<"vtkQuartzImageMapper::RenderData - no window set for viewport");
    return;
    }

  void *hWnd = window->GetGenericWindowId();

  // Get the device context from the window
  void *windowDC = window->GetGenericContext();

  // Determine the size of the displayed data.
  width = (this->DisplayExtent[1] - this->DisplayExtent[0] + 1);
  height = (this->DisplayExtent[3] - this->DisplayExtent[2] + 1);
  
  dataWidth = ((width*3+3)/4)*4;
  
  shift = this->GetColorShift();
  scale = this->GetColorScale();
  
  if (!this->HBitmap)
    {
    vtkDebugMacro (<< "vtkQuartzImageMapper::RenderData - creating HBitmap: " << width << "," << height
    << "(" << dataWidth*height << " bytes)");
    
//    dataHeader.bmiHeader.biSize = 40;
//    dataHeader.bmiHeader.biWidth = width;
//    dataHeader.bmiHeader.biHeight = height;
//    dataHeader.bmiHeader.biPlanes = 1;
//    dataHeader.bmiHeader.biBitCount = 24;
//    dataHeader.bmiHeader.biCompression = BI_RGB;
//    dataHeader.bmiHeader.biSizeImage = dataWidth*height;
//    dataHeader.bmiHeader.biClrUsed = 0;
//    dataHeader.bmiHeader.biClrImportant = 0;

    // try using a DIBsection
//    this->HBitmap = CreateDIBSection(windowDC, &dataHeader,
//				     DIB_RGB_COLORS,
//				     (void **)(&(this->DataOut)), 
//				     NULL, 0);
    }
  else 
    {
    void *bm;
    //GetObject(this->HBitmap, sizeof (BITMAP), (LPSTR) &bm);

//    vtkDebugMacro(<< "vtkQuartzImageMapper::RenderData - Bitmap width: "
//    << bm.bmWidth);
//    vtkDebugMacro(<< "vtkQuartzImageMapper::RenderData - Bitmap height: "
//    << bm.bmHeight);

    // if data size differs from bitmap size, reallocate the bitmap
//    if ((width != bm.bmWidth) || (height != bm.bmHeight))
//      {
//      vtkDebugMacro(<< "vtkQuartzImageMapper::RenderData - Changing bitmap size to: "
//      << width << "," << height << "(" << dataWidth*height << " bytes)");  	

//      DeleteObject(this->HBitmap);

//      dataHeader.bmiHeader.biWidth = width;
//      dataHeader.bmiHeader.biHeight = height;
//      dataHeader.bmiHeader.biSizeImage = dataWidth*height;
//      dataHeader.bmiHeader.biClrUsed = 0;
//      dataHeader.bmiHeader.biClrImportant = 0;  
      // try using a DIBsection
//      this->HBitmap = CreateDIBSection(windowDC, &dataHeader, 
//				       DIB_RGB_COLORS, 
//				       (void **)(&(this->DataOut)), NULL, 0);
//      }
 
    }

  int dim = 0;
  dim = data->GetNumberOfScalarComponents();
  ptr0 = data->GetScalarPointer(this->DisplayExtent[0], 
				this->DisplayExtent[2], 
				this->DisplayExtent[4]);

  if (dim > 1)    

    {
    // Call the appropriate templated function
    switch (data->GetScalarType())
      {
      case VTK_DOUBLE:
        vtkDebugMacro(<<"vtkWin32ImageMappper::RenderData - RenderColor, Double");
        vtkQuartzImageMapperRenderColor(this, data,
				       (double *)(ptr0), dim,
				       this->DataOut, shift, scale);
        break;
      case VTK_FLOAT:
        vtkDebugMacro(<<"vtkWin32ImageMappper::RenderData - RenderColor, Float");
        vtkQuartzImageMapperRenderColor(this, data,
				       (float *)(ptr0), dim,
				       this->DataOut, shift, scale);
        break;
      case VTK_LONG:
        vtkDebugMacro(<<"vtkWin32ImageMappper::RenderData - RenderColor, Long");
        vtkQuartzImageMapperRenderColor(this, data,
				       (long *)(ptr0), dim,
				       this->DataOut, shift, scale);
        break;
      case VTK_UNSIGNED_LONG:
        vtkDebugMacro(<<"vtkWin32ImageMappper::RenderData - RenderColor, Unsigned Long");
        vtkQuartzImageMapperRenderColor(this, data, (unsigned long *)(ptr0),
				       dim, this->DataOut, shift, scale);
        break;
      case VTK_INT:
        vtkDebugMacro(<<"vtkWin32ImageMappper::RenderData - RenderColor, Int");
        vtkQuartzImageMapperRenderColor(this, data,
				       (int *)(ptr0), dim,
				       this->DataOut, shift, scale);
        break;
      case VTK_UNSIGNED_INT:
        vtkDebugMacro(<<"vtkWin32ImageMappper::RenderData - RenderColor, Unsigned Int");
        vtkQuartzImageMapperRenderColor(this, data, (unsigned int *)(ptr0),
				       dim, this->DataOut, shift, scale);
        break;
      case VTK_SHORT:
        vtkDebugMacro(<<"vtkWin32ImageMappper::RenderData - RenderColor, Short");
        vtkQuartzImageMapperRenderColor(this, data,
				       (short *)(ptr0), dim,
				       this->DataOut, shift, scale);
        break;
      case VTK_UNSIGNED_SHORT:
        vtkDebugMacro(<<"vtkWin32ImageMappper::RenderData - RenderColor, Unsigned Short");
        vtkQuartzImageMapperRenderColor(this, data, (unsigned short *)(ptr0),
				       dim, this->DataOut, shift, scale);
        break;
      case VTK_CHAR:
        vtkDebugMacro(<<"vtkWin32ImageMappper::RenderData - RenderColor, Char");
        vtkQuartzImageMapperRenderColor(this, data,
				       (char *)(ptr0), dim,
				       this->DataOut, shift, scale);
        break;
      case VTK_UNSIGNED_CHAR:
        vtkDebugMacro(<<"vtkWin32ImageMappper::RenderData - RenderColor, Unsigned Char");
        vtkQuartzImageMapperRenderColor(this, data, (unsigned char *)(ptr0),
				       dim,this->DataOut, shift, scale);
        break;
      }
    }
  else
    {
    // GrayScale images.
    // Call the appropriate templated function
    switch (data->GetScalarType())
      {
      case VTK_DOUBLE:
	vtkQuartzImageMapperRenderGray(this, data, (double *)(ptr0), 
				      this->DataOut,
				      shift, scale);
	break;
      case VTK_FLOAT:
	vtkQuartzImageMapperRenderGray(this, data, (float *)(ptr0), 
				      this->DataOut,
				      shift, scale);
	break;
      case VTK_LONG:
	vtkQuartzImageMapperRenderGray(this, data, (long *)(ptr0), 
				      this->DataOut,
				      shift, scale);
	break;
      case VTK_UNSIGNED_LONG:
	vtkQuartzImageMapperRenderGray(this, data, 
                                      (unsigned long *)(ptr0),
                                      this->DataOut, 
                                      shift, scale);
	break;
      case VTK_INT:
	vtkQuartzImageMapperRenderGray(this, data, (int *)(ptr0), 
				      this->DataOut,
				      shift, scale);
	break;
      case VTK_UNSIGNED_INT:
	vtkQuartzImageMapperRenderGray(this, data, 
                                      (unsigned int *)(ptr0),
                                      this->DataOut, 
                                      shift, scale);
	break;
      case VTK_SHORT:
	vtkQuartzImageMapperRenderShortGray(this, data, (short *)(ptr0),
					   this->DataOut,
					   shift, scale);
	break;
      case VTK_UNSIGNED_SHORT:
	vtkQuartzImageMapperRenderShortGray(this, data, 
					   (unsigned short *)(ptr0),
					   this->DataOut, 
					   shift, scale);
	break;
      case VTK_CHAR:
	vtkQuartzImageMapperRenderShortGray(this, data, (int *)(ptr0), 
                                           this->DataOut,
                                           shift, scale);
	break;
      case VTK_UNSIGNED_CHAR:
	vtkQuartzImageMapperRenderShortGray(this, data, 
					   (unsigned char *)(ptr0),
					   this->DataOut, 
					   shift, scale);
	break;
      }
    }


  //compatDC = CreateCompatibleDC(windowDC);

 // hOldBitmap = (HBITMAP)SelectObject(compatDC,this->HBitmap);

  //  float* actorScale = actor->GetScale();
  float actorScale[2]; actorScale[0] = actorScale[1] = 1.0;
    
  int xSize= (int) (actorScale[0] * (float) width);
  int ySize = (int) (actorScale[1] * (float) height);

  // Get the position of the text actor
  int* actorPos = 
    actor->GetPositionCoordinate()->GetComputedLocalDisplayValue(viewport);
  // negative positions will already be clipped to viewport
  actorPos[0] += this->PositionAdjustment[0]; 
  actorPos[1] -= this->PositionAdjustment[1];

  actorPos[1] = actorPos[1] - height + 1;

  //StretchBlt(windowDC,actorPos[0],actorPos[1],xSize,ySize,compatDC,0,
//	     0,width,height,SRCCOPY);

  //SelectObject(compatDC, hOldBitmap);
  //DeleteDC(compatDC);

}








