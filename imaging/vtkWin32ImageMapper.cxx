#include "vtkWin32ImageMapper.h"
#include "vtkWin32ImageWindow.h"
#include "vtkProperty2D.h"

vtkWin32ImageMapper::vtkWin32ImageMapper()
{
  this->HBitmap = (HBITMAP)0;
  this->DataOut = NULL;

}

vtkWin32ImageMapper::~vtkWin32ImageMapper()
{
  if (this->HBitmap)
    {
    DeleteObject(this->HBitmap);
    this->HBitmap = (HBITMAP)0;
    }
}

int vtkWin32ImageMapper::GetCompositingMode(vtkActor2D* actor)
{

  vtkProperty2D* tempProp = actor->GetProperty();
  int compositeMode = tempProp->GetCompositingOperator();

  switch (compositeMode)
    {
    case VTK_BLACK:
      return BLACKNESS;
      break;
    case VTK_NOT_DEST:
      return DSTINVERT;
      break;
    case VTK_SRC_AND_DEST:
      return SRCAND;
      break;
    case VTK_SRC_OR_DEST:
      return  SRCPAINT;
      break;
    case VTK_NOT_SRC:
      return NOTSRCCOPY;
      break;
    case VTK_SRC_XOR_DEST:
      return SRCINVERT;
      break;
    case VTK_SRC_AND_notDEST:
      return SRCERASE;
      break;
    case VTK_SRC:
      return SRCCOPY;
      break;
    case VTK_WHITE:
      return WHITENESS;
      break;
    default:
      return SRCCOPY;
      break;
    }

}

//----------------------------------------------------------------------------
/* 
 * This templated routine calculates effective lover and upper limits 
 * for a window of values of type T, lower and upper. 
 * It also returns in variable hit the flag with value 1, if the 
 * interval [f_lower, f_upper) is above the type range, -1,  if the 
 * interval [f_lower, f_upper) is below the type range, and 0, if 
 * there is intersection between this interval and the type range.
 */
template <class T>
static void clamps ( vtkImageData *data, float w, float l, T& lower, T& upper, int& hit)
{
  double f_lower, f_upper;
  double range[2];

  data->GetPointData()->GetScalars()->GetDataTypeRange( range );

  f_lower = l - fabs(w) / 2.0;
  f_upper = f_lower + fabs(w);

  // Look if we above the type range
  if ( f_lower > range[1] )
    {
    hit = 1;
    return;
    }
  // Look if we below the type range
  if ( f_upper <= range[0] )
    {
    hit = -1;
    return;
    }

  // Set the correct lower value
  if ( f_lower >= range[0] )
    {
    lower = (T) f_lower;
    }
  else
    {
    lower = (T) range[0];
    }

  // Set the correct upper value
  if ( f_upper <= range[1] )
    {
    upper = (T) f_upper;
    }
  else
    {
    upper = (T) range[1];
    }

  hit = 0;
}

//----------------------------------------------------------------------------
// Description:
// A templated function that handles gray scale images.
template <class T>
static void vtkWin32ImageMapperRenderGray(vtkWin32ImageMapper *self, 
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
  int hit;
  int sscale;
  int i_lower;

  clamps ( data, self->GetColorWindow(), self->GetColorLevel(), 
	   lower, upper, hit );
  i_lower = -shift;

  // Selection of the constant 4096.0 may be changed in the future
  // or may be type dependent
  sscale = scale*4096.0;

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
  rowAdder = (4 - ((inMax0-inMin0 + 1)*3)%4)%4;
  inPtr1 = inPtr;
  if ( hit == 1 ) 
    {
    // type range is to the left of the window
    for (idx1 = inMin1; idx1 <= inMax1; idx1++)
      {
      inPtr0 = inPtr1;
      endPtr = inPtr0 + inInc0*(inMax0 - inMin0 + 1);
      while (inPtr0 != endPtr)
	{
	*outPtr++ = 0;
	*outPtr++ = 0;
	*outPtr++ = 0;
	inPtr0 += inInc0;
	}
      // rows must be a multiple of four bytes
      // so pad it if neccessary
      outPtr += rowAdder;
      inPtr1 += inInc1;
      } 
    }
  else if ( hit == -1 ) 
    {
    // type range is to the right of the window
    for (idx1 = inMin1; idx1 <= inMax1; idx1++)
      {
      inPtr0 = inPtr1;
      endPtr = inPtr0 + inInc0*(inMax0 - inMin0 + 1);
      while (inPtr0 != endPtr)
	{
	*outPtr++ = 255;
	*outPtr++ = 255;
	*outPtr++ = 255;
	inPtr0 += inInc0;
	}
      // rows must be a multiple of four bytes
      // so pad it if neccessary
      outPtr += rowAdder;
      inPtr1 += inInc1;
      } 
    }
  else // type range intersects with the window
    {
    for (idx1 = inMin1; idx1 <= inMax1; idx1++)
      {
      inPtr0 = inPtr1;
      endPtr = inPtr0 + inInc0*(inMax0 - inMin0 + 1);
      while (inPtr0 != endPtr)
	{
	if (*inPtr0 < lower) 
	  {
	  *outPtr++ = 0;
	  *outPtr++ = 0;
	  *outPtr++ = 0;
	  }
	else if (*inPtr0 >= upper)
	  {
	  *outPtr++ = 255;
	  *outPtr++ = 255;
	  *outPtr++ = 255;
	  }
	else
	  {
	  // Because i_lower and sscale are of integer type
	  // this is fast for all types used by this
	  // template (float is treated separately).
	  colorIdx = ((int) *inPtr0 - i_lower) * sscale >> 12;
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
}

//----------------------------------------------------------------------------
// Description:
// A function that handles gray scale images with float data.
static void vtkWin32ImageMapperRenderFloatGray(vtkWin32ImageMapper *self, 
					       vtkImageData *data,
					       float *inPtr, unsigned char *outPtr,
					       float shift, float scale)
{
  unsigned char colorIdx;
  float *inPtr0, *inPtr1, *endPtr;
  int inMin0, inMax0, inMin1, inMax1;
  int inInc0, inInc1;
  int idx1;
  float  lower, upper;
  int rowAdder;
  int *Size;

  lower = -shift;
  upper = lower + 255.0/scale;

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
  rowAdder = (4 - ((inMax0-inMin0 + 1)*3)%4)%4;
  inPtr1 = inPtr;
  for (idx1 = inMin1; idx1 <= inMax1; idx1++)
    {
    inPtr0 = inPtr1;
    endPtr = inPtr0 + inInc0*(inMax0 - inMin0 + 1);
    while (inPtr0 != endPtr)
      {
      if (*inPtr0 < lower) 
	{
	*outPtr++ = 0;
	*outPtr++ = 0;
	*outPtr++ = 0;
	}
      else if (*inPtr0 >= upper)
	{
	*outPtr++ = 255;
	*outPtr++ = 255;
	*outPtr++ = 255;
	}
      else
	{
	colorIdx = (*inPtr0 - lower) * scale ;
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
// Description:
// A templated function that handles color images. (only True Color 24 bit)
template <class T>
static void vtkWin32ImageMapperRenderColor(vtkWin32ImageMapper *self, 
					   vtkImageData *data, T *redPtr, int bpp,
					   unsigned char *outPtr, float shift, float scale)
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
  int hit;
  T *greenPtr; 
  T *bluePtr;  

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
  
  if (bpp >= 2) greenPtr = redPtr + 1;
  else greenPtr = redPtr;
  if (bpp >= 3) bluePtr = redPtr + 2;
  else bluePtr = redPtr;

  clamps ( data, self->GetColorWindow(), self->GetColorLevel(), 
	   lower, upper, hit );

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
      if (*redPtr0 <= lower) red = 0;
      else if (*redPtr0 >= upper) red = 255;
      else red = (unsigned char)(((float)(*redPtr0) + shift) * scale);

      if (*greenPtr0 <= lower) green = 0;
      else if (*greenPtr0 >= upper) green = 255;
      else green = (unsigned char)(((float)(*greenPtr0) + shift) * scale);
  
      if (*bluePtr0 <= lower) blue = 0;
      else if (*bluePtr0 >= upper) blue = 255;
      else blue = (unsigned char)(((float)(*bluePtr0) + shift) * scale);
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
// Description:
// A templated function that handles gray scale images.

template <class T>
static void vtkWin32ImageMapperRenderShortGray(vtkWin32ImageMapper *self,
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
  int sscale;
  int rowAdder;

  lower = -shift;
  upper = lower + 255.0/scale;
  sscale = scale*4096.0;

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
        *outPtr++ = 0;
        *outPtr++ = 0;
        *outPtr++ = 0;
        }
      else if (*inPtr0 >= upper)
        {
        *outPtr++ = 255;
        *outPtr++ = 255;
        *outPtr++ = 255;
        }
      else
        {
        colorIdx = ((*inPtr0 - lower) * sscale) >> 12;
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

void vtkWin32ImageMapper::RenderData(vtkViewport* viewport, 
				       vtkImageData *data, vtkActor2D *actor)
{
  int dataWidth, width, height;
  int size;
  // int extent[6];
  unsigned char *dataOut;
  void *ptr0, *ptr1, *ptr2;
  float shift, scale;
  HDC compatDC;
  HBITMAP hOldBitmap;
  static BITMAPINFO dataHeader;

  vtkWindow* window = (vtkWindow *) viewport->GetVTKWindow();

  if (!window)
    {
    vtkErrorMacro (<<"vtkWin32ImageMapper::RenderData - no window set for viewport");
    return;
    }

  HWND hWnd = (HWND) window->GetGenericWindowId();

  // Get the device context from the window
  HDC windowDC = (HDC) window->GetGenericContext();

  // Determine the size of the displayed data.
  int* extent = this->Input->GetUpdateExtent();
  width = (extent[1] - extent[0] + 1);
  height = (extent[3] - extent[2] + 1);
  
  dataWidth = ((width*3+3)/4)*4;
  
  shift = this->GetColorShift();
  scale = this->GetColorScale();
  
  if (!this->HBitmap)
    {
    vtkDebugMacro (<< "vtkWin32ImageMapper::RenderData - creating HBitmap: " << width << "," << height
    << "(" << dataWidth*height << " bytes)");
    
    dataHeader.bmiHeader.biSize = 40;
    dataHeader.bmiHeader.biWidth = width;
    dataHeader.bmiHeader.biHeight = height;
    dataHeader.bmiHeader.biPlanes = 1;
    dataHeader.bmiHeader.biBitCount = 24;
    dataHeader.bmiHeader.biCompression = BI_RGB;
    dataHeader.bmiHeader.biSizeImage = dataWidth*height;
    dataHeader.bmiHeader.biClrUsed = 0;
    dataHeader.bmiHeader.biClrImportant = 0;

    // try using a DIBsection
    this->HBitmap = CreateDIBSection(windowDC, &dataHeader,
				     DIB_RGB_COLORS, (void **)(&(this->DataOut)), 
				     NULL, 0);
    }
  else 
	{
    BITMAP bm;
    GetObject(this->HBitmap, sizeof (BITMAP), (LPSTR) &bm);

    vtkDebugMacro(<< "vtkWin32ImageMapper::RenderData - Bitmap width: " << bm.bmWidth);
    vtkDebugMacro(<< "vtkWin32ImageMapper::RenderData - Bitmap height: " << bm.bmHeight);

    // if data size differs from bitmap size, reallocate the bitmap
    if ((width != bm.bmWidth) || (height != bm.bmHeight))
      {
      vtkDebugMacro(<< "vtkWin32ImageMapper::RenderData - Changing bitmap size to: "
      << width << "," << height << "(" << dataWidth*height << " bytes)");  	

      DeleteObject(this->HBitmap);

      dataHeader.bmiHeader.biWidth = width;
      dataHeader.bmiHeader.biHeight = height;
      dataHeader.bmiHeader.biSizeImage = dataWidth*height;
      dataHeader.bmiHeader.biClrUsed = 0;
      dataHeader.bmiHeader.biClrImportant = 0;  
      // try using a DIBsection
      this->HBitmap = CreateDIBSection(windowDC, &dataHeader, 
				       DIB_RGB_COLORS, (void **)(&(this->DataOut)), NULL, 0);
      }
 
     }

  int dim = 0;
  dim = data->GetNumberOfScalarComponents();
  ptr0 = data->GetScalarPointer(extent[0], extent[2], extent[4]);

  if (dim > 1)    

    {
    // Call the appropriate templated function
    switch (data->GetScalarType())
      {
      case VTK_FLOAT:
        vtkDebugMacro(<<"vtkWin32ImageMappper::RenderData - RenderColor, Float");
        vtkWin32ImageMapperRenderColor(this, data,
				       (float *)(ptr0), dim,
				       this->DataOut, shift, scale);
        break;
      case VTK_INT:
        vtkDebugMacro(<<"vtkWin32ImageMappper::RenderData - RenderColor, Int");
        vtkWin32ImageMapperRenderColor(this, data,
				       (int *)(ptr0), dim,
				       this->DataOut, shift, scale);
        break;
      case VTK_SHORT:
        vtkDebugMacro(<<"vtkWin32ImageMappper::RenderData - RenderColor, Short");
        vtkWin32ImageMapperRenderColor(this, data,
				       (short *)(ptr0), dim,
				       this->DataOut, shift, scale);
        break;
      case VTK_UNSIGNED_SHORT:
        vtkDebugMacro(<<"vtkWin32ImageMappper::RenderData - RenderColor, Unsigned Short");
        vtkWin32ImageMapperRenderColor(this, data, (unsigned short *)(ptr0),
										dim, this->DataOut, shift, scale);
        break;
      case VTK_UNSIGNED_CHAR:
        vtkDebugMacro(<<"vtkWin32ImageMappper::RenderData - RenderColor, Unsigned Char");
        vtkWin32ImageMapperRenderColor(this, data, (unsigned char *)(ptr0),
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
      case VTK_FLOAT:
	vtkWin32ImageMapperRenderGray(this, data, (float *)(ptr0), 
				      this->DataOut,
				      shift, scale);
	break;
      case VTK_INT:
	vtkWin32ImageMapperRenderGray(this, data, (int *)(ptr0), 
				      this->DataOut,
				      shift, scale);
	break;
      case VTK_SHORT:
	vtkWin32ImageMapperRenderShortGray(this, data, (short *)(ptr0),
					   this->DataOut,
					   shift, scale);
	break;
      case VTK_UNSIGNED_SHORT:
	vtkWin32ImageMapperRenderGray(this, data, 
					   (unsigned short *)(ptr0),
					   this->DataOut, 
					   shift, scale);
	break;
      case VTK_UNSIGNED_CHAR:
	vtkWin32ImageMapperRenderGray(this, data, 
				      (unsigned char *)(ptr0),
				      this->DataOut, 
				      shift, scale);
	break;
      }
    }


  compatDC = CreateCompatibleDC(windowDC);

  hOldBitmap = (HBITMAP)SelectObject(compatDC,this->HBitmap);

  float* actorScale = actor->GetScale();
    
  int xSize= (int) (actorScale[0] * (float) width);
  int ySize = (int) (actorScale[1] * (float) height);

  // Get the position of the text actor
  int* actorPos = 
    actor->GetPositionCoordinate()->GetComputedLocalDisplayValue(viewport);
  // negative positions will already be clipped to viewport
  actorPos[0] += this->PositionAdjustment[0]; 
  actorPos[1] -= this->PositionAdjustment[1];

  actorPos[1] = actorPos[1] - height + 1;

  // actorPos[1] = actorPos[1] - actorScale[1] * height;
  // Adjust position for image flips - note actorScale is negative in expression
  //if (xSize< 0)  actorPos[0] = actorPos[0] - actorScale[0] * width;
  //if (ySize < 0)  actorPos[1] = actorPos[1] - actorScale[1] * height;

  int compositeMode = this->GetCompositingMode(actor);

  StretchBlt(windowDC,actorPos[0],actorPos[1],xSize,ySize,compatDC,0,
	           0,width,height,compositeMode);

  SelectObject(compatDC, hOldBitmap);
  DeleteDC(compatDC);

}








