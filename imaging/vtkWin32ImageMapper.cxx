#include "vtkWin32ImageMapper.h"
#include "vtkWin32ImageWindow.h"
#include "vtkProperty2D.h"
#include "vtkObjectFactory.h"

#ifndef VTK_REMOVE_LEGACY_CODE

//----------------------------------------------------------------------------
vtkWin32ImageMapper* vtkWin32ImageMapper::New()
{
  vtkGenericWarningMacro(<<"Obsolete native imaging class: " 
                         <<"use OpenGL version instead");

  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkWin32ImageMapper");
  if(ret)
    {
    return (vtkWin32ImageMapper*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkWin32ImageMapper;
}

vtkWin32ImageMapper::vtkWin32ImageMapper()
{
  this->HBitmap = (HBITMAP)0;
  this->DataOut = NULL;
  this->LookupTable = NULL;
}

vtkWin32ImageMapper::~vtkWin32ImageMapper()
{
  if (this->HBitmap)
    {
    DeleteObject(this->HBitmap);
    this->HBitmap = (HBITMAP)0;
    }
  if (this->LookupTable)
    {
    this->LookupTable->Delete();
    }
}

void vtkWin32ImageMapper::PrintSelf(ostream& os, vtkIndent indent)
{
  this->vtkImageMapper::PrintSelf(os, indent);

  if ( this->LookupTable )
    {
    os << indent << "Lookup Table:\n";
    this->LookupTable->PrintSelf(os,indent.GetNextIndent());
    }
  else
    {
    os << indent << "Lookup Table: (none)\n";
    }
}

unsigned long int vtkWin32ImageMapper::GetMTime()
{
  unsigned long mTime=this->vtkImageMapper::GetMTime();
  unsigned long time;

  if ( this->LookupTable != NULL )
    {
    time = this->LookupTable->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

/*
 * This templated routine calculates effective lower and upper limits
 * for a window of values of type T, lower and upper.
 */
template <class T>
static void vtkWin32ImageMapperClamps ( vtkImageData *data, float w,
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
static void vtkWin32ImageMapperRenderGray(
        vtkImageData *data, T *inPtr, unsigned char *outPtr, int DisplayExtent[6],
        float cwindow, float clevel, float cshift, float cscale,
        vtkLookupTable *lut)
{
  unsigned char colorIdx;
  T *inPtr0, *inPtr1, *endPtr;
  int inMin0, inMax0, inMin1, inMax1;
  int inInc0, inInc1;
  int idx1;
  T   lower, upper;
  int rowAdder;
  unsigned char lower_val, upper_val;

  vtkWin32ImageMapperClamps ( data, cwindow, clevel,
			      lower, upper, lower_val, upper_val );

  inMin0 = DisplayExtent[0];
  inMax0 = DisplayExtent[1];
  inMin1 = DisplayExtent[2];
  inMax1 = DisplayExtent[3];

  // data->GetIncrements(inInc0, inInc1);
  int* tempIncs = data->GetIncrements();
  inInc0 = tempIncs[0];
  inInc1 = tempIncs[1];

  // Loop through in regions pixels
  rowAdder = (4 - ((inMax0-inMin0 + 1)*3)%4)%4;
  inPtr1 = inPtr;

  if (!lut)
    {
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
	      colorIdx = (int) ((*inPtr0 + cshift)*cscale);
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
  else
    {
    // make a copy of lower and upper RGBA values to save time
    unsigned char *Prgbalower = lut->MapValue(lower);
    unsigned char  rgbalower[4] =
        { Prgbalower[0], Prgbalower[1], Prgbalower[2], Prgbalower[3] };
    unsigned char *Prgbaupper = lut->MapValue(upper);
    unsigned char  rgbaupper[4] =
        { Prgbaupper[0], Prgbaupper[1], Prgbaupper[2], Prgbaupper[3] };

    for (idx1 = inMin1; idx1 <= inMax1; idx1++)
      {
      inPtr0 = inPtr1;
      endPtr = inPtr0 + inInc0*(inMax0 - inMin0 + 1);
      while (inPtr0 != endPtr)
        {
        if (*inPtr0 <= lower)
	      {
	      *outPtr++ = rgbalower[2];
	      *outPtr++ = rgbalower[1];
	      *outPtr++ = rgbalower[0];
	      }
        else if (*inPtr0 >= upper)
	      {
	      *outPtr++ = rgbaupper[2];
	      *outPtr++ = rgbaupper[1];
	      *outPtr++ = rgbaupper[0];
	      }
        else
	      {
          // Because i_lower and sscale are of integer type
	      // this is fast for all types used by this
	      // template (float is treated separately).
	      colorIdx = (int) ((*inPtr0 + cshift)*cscale);
          unsigned char *Prgba = lut->MapValue(colorIdx);
	      *outPtr++ = Prgba[2];
	      *outPtr++ = Prgba[1];
	      *outPtr++ = Prgba[0];
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
// A templated function that handles gray scale images.
template <class T>
static void vtkWin32ImageMapperRenderShortGray(
        vtkImageData *data, T *inPtr, unsigned char *outPtr, int DisplayExtent[6],
        float cwindow, float clevel, float cshift, float cscale,
        vtkLookupTable *lut)
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

  vtkWin32ImageMapperClamps ( data, cwindow, clevel,
			      lower, upper, lower_val, upper_val );

  sscale = cscale*4096.0;
  sshift = sscale*cshift;

  // data->GetExtent(inMin0, inMax0, inMin1, inMax1);
  inMin0 = DisplayExtent[0];
  inMax0 = DisplayExtent[1];
  inMin1 = DisplayExtent[2];
  inMax1 = DisplayExtent[3];

  // data->GetIncrements(inInc0, inInc1);
  int* tempIncs = data->GetIncrements();
  inInc0 = tempIncs[0];
  inInc1 = tempIncs[1];

  // Loop through in regions pixels
  rowAdder = (4 - ((inMax0-inMin0 + 1)*3)%4)%4;
  inPtr1 = inPtr;
  if (!lut)
    {
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
  else
    {
    // make a copy of lower and upper RGBA values to save time
    unsigned char *Prgbalower = lut->MapValue(lower);
    unsigned char  rgbalower[4] =
        { Prgbalower[0], Prgbalower[1], Prgbalower[2], Prgbalower[3] };
    unsigned char *Prgbaupper = lut->MapValue(upper);
    unsigned char  rgbaupper[4] =
        { Prgbaupper[0], Prgbaupper[1], Prgbaupper[2], Prgbaupper[3] };

    for (idx1 = inMin1; idx1 <= inMax1; idx1++)
      {
      inPtr0 = inPtr1;
      endPtr = inPtr0 + inInc0*(inMax0 - inMin0 + 1);
      while (inPtr0 != endPtr)
        {
        if (*inPtr0 <= lower)
	      {
	      *outPtr++ = rgbalower[2];
	      *outPtr++ = rgbalower[1];
	      *outPtr++ = rgbalower[0];
	      }
        else if (*inPtr0 >= upper)
	      {
	      *outPtr++ = rgbaupper[2];
	      *outPtr++ = rgbaupper[1];
	      *outPtr++ = rgbaupper[0];
	      }
        else
	      {
          // Because i_lower and sscale are of integer type
	      // this is fast for all types used by this
	      // template (float is treated separately).
          colorIdx = ((*inPtr0) * sscale + sshift) >> 12;
          unsigned char *Prgba = lut->MapValue(colorIdx);
	      *outPtr++ = Prgba[2];
	      *outPtr++ = Prgba[1];
	      *outPtr++ = Prgba[0];
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
// A templated function that handles color images. (only True Color 24 bit)
template <class T>
static void vtkWin32ImageMapperRenderColor(
        vtkImageData *data, T *redPtr, int bpp, unsigned char *outPtr,
        int DisplayExtent[6], float cwindow, float clevel, float cshift, float cscale)
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
  inMin0 = DisplayExtent[0];
  inMax0 = DisplayExtent[1];
  inMin1 = DisplayExtent[2];
  inMax1 = DisplayExtent[3];

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

  vtkWin32ImageMapperClamps ( data, cwindow, clevel,
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
	red = (unsigned char)(((float)(*redPtr0) + cshift) * cscale);
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
	green = (unsigned char)(((float)(*greenPtr0) + cshift) * cscale);
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
	blue = (unsigned char)(((float)(*bluePtr0) + cshift) * cscale);
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

HBITMAP vtkWin32ImageMapper::CreateBitmapObject(
    HBITMAP oldBitmap, BITMAPINFO &dataHeader, HDC windowDC,
    unsigned char *&DataOut, vtkImageData *data, int width, int height)
{
  // make sure width is 4 byte aligned
  int dataWidth = ((width*3+3)/4)*4;
  //
  if (!oldBitmap)
    {
    dataHeader.bmiHeader.biSize         = 40;
    dataHeader.bmiHeader.biWidth        = width;
    dataHeader.bmiHeader.biHeight       = height;
    dataHeader.bmiHeader.biPlanes       = 1;
    dataHeader.bmiHeader.biBitCount     = 24;
    dataHeader.bmiHeader.biCompression  = BI_RGB;
    dataHeader.bmiHeader.biSizeImage    = dataWidth*height;
    dataHeader.bmiHeader.biClrUsed      = 0;
    dataHeader.bmiHeader.biClrImportant = 0;
    // try using a DIBsection
    return CreateDIBSection(windowDC, &dataHeader, DIB_RGB_COLORS,
        (void **)(&(DataOut)), NULL, 0);
    }
  else
    {
    BITMAP bm;
    GetObject(oldBitmap, sizeof (BITMAP), (LPSTR) &bm);

    // if data size differs from bitmap size, reallocate the bitmap
    if ((width != bm.bmWidth) || (height != bm.bmHeight))
      {
      DeleteObject(oldBitmap);
      dataHeader.bmiHeader.biWidth        = width;
      dataHeader.bmiHeader.biHeight       = height;
      dataHeader.bmiHeader.biSizeImage    = dataWidth*height;
      dataHeader.bmiHeader.biClrUsed      = 0;
      dataHeader.bmiHeader.biClrImportant = 0;
      // try using a DIBsection
      return CreateDIBSection(windowDC, &dataHeader, DIB_RGB_COLORS,
        (void **)(&(DataOut)), NULL, 0);
      }
    }
    return oldBitmap;
}
//----------------------------------------------------------------------------
void vtkWin32ImageMapper::GenerateBitmapData(
    vtkImageData *data, void *inptr, unsigned char *DataOut, int dim,
    int DisplayExtent[6], float cwindow, float clevel, float cshift, float cscale,
    vtkLookupTable *lut)
{
  if (dim > 1)
    {
    // Call the appropriate templated function
    switch (data->GetScalarType())
      {
      case VTK_DOUBLE:
//        vtkDebugMacro(<<"vtkWin32ImageMappper::RenderData - RenderColor, Double");
        vtkWin32ImageMapperRenderColor(data, (double *)(inptr), dim, DataOut,
                    DisplayExtent, cwindow, clevel, cshift, cscale);
        break;
      case VTK_FLOAT:
//        vtkDebugMacro(<<"vtkWin32ImageMappper::RenderData - RenderColor, Float");
        vtkWin32ImageMapperRenderColor(data, (float *)(inptr), dim, DataOut,
                    DisplayExtent, cwindow, clevel, cshift, cscale);
        break;
      case VTK_LONG:
//        vtkDebugMacro(<<"vtkWin32ImageMappper::RenderData - RenderColor, Long");
        vtkWin32ImageMapperRenderColor(data, (long *)(inptr), dim, DataOut,
                    DisplayExtent, cwindow, clevel, cshift, cscale);
        break;
      case VTK_UNSIGNED_LONG:
//        vtkDebugMacro(<<"vtkWin32ImageMappper::RenderData - RenderColor, Unsigned Long");
        vtkWin32ImageMapperRenderColor(data, (unsigned long *)(inptr), dim, DataOut,
                    DisplayExtent, cwindow, clevel, cshift, cscale);
        break;
      case VTK_INT:
//        vtkDebugMacro(<<"vtkWin32ImageMappper::RenderData - RenderColor, Int");
        vtkWin32ImageMapperRenderColor(data, (int *)(inptr), dim, DataOut,
                    DisplayExtent, cwindow, clevel, cshift, cscale);
        break;
      case VTK_UNSIGNED_INT:
//        vtkDebugMacro(<<"vtkWin32ImageMappper::RenderData - RenderColor, Unsigned Int");
        vtkWin32ImageMapperRenderColor(data, (unsigned int *)(inptr), dim, DataOut,
                    DisplayExtent, cwindow, clevel, cshift, cscale);
        break;
      case VTK_SHORT:
//        vtkDebugMacro(<<"vtkWin32ImageMappper::RenderData - RenderColor, Short");
        vtkWin32ImageMapperRenderColor(data, (short *)(inptr), dim, DataOut,
                    DisplayExtent, cwindow, clevel, cshift, cscale);
        break;
      case VTK_UNSIGNED_SHORT:
//        vtkDebugMacro(<<"vtkWin32ImageMappper::RenderData - RenderColor, Unsigned Short");
        vtkWin32ImageMapperRenderColor(data, (unsigned short *)(inptr), dim, DataOut,
                    DisplayExtent, cwindow, clevel, cshift, cscale);
        break;
      case VTK_CHAR:
//        vtkDebugMacro(<<"vtkWin32ImageMappper::RenderData - RenderColor, Char");
        vtkWin32ImageMapperRenderColor(data, (char *)(inptr), dim, DataOut,
                    DisplayExtent, cwindow, clevel, cshift, cscale);
        break;
      case VTK_UNSIGNED_CHAR:
//        vtkDebugMacro(<<"vtkWin32ImageMappper::RenderData - RenderColor, Unsigned Char");
        vtkWin32ImageMapperRenderColor(data, (unsigned char *)(inptr), dim, DataOut,
                    DisplayExtent, cwindow, clevel, cshift, cscale);
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
          vtkWin32ImageMapperRenderGray(data, (double *)(inptr), DataOut,
            DisplayExtent, cwindow, clevel, cshift, cscale, lut);
          break;
        case VTK_FLOAT:
          vtkWin32ImageMapperRenderGray(data, (float *)(inptr), DataOut,
            DisplayExtent, cwindow, clevel, cshift, cscale, lut);
          break;
        case VTK_LONG:
          vtkWin32ImageMapperRenderGray(data, (long *)(inptr), DataOut,
            DisplayExtent, cwindow, clevel, cshift, cscale, lut);
          break;
        case VTK_UNSIGNED_LONG:
          vtkWin32ImageMapperRenderGray(data, (unsigned long *)(inptr), DataOut,
            DisplayExtent, cwindow, clevel, cshift, cscale, lut);
          break;
        case VTK_INT:
          vtkWin32ImageMapperRenderGray(data, (int *)(inptr), DataOut,
            DisplayExtent, cwindow, clevel, cshift, cscale, lut);
          break;
        case VTK_UNSIGNED_INT:
          vtkWin32ImageMapperRenderGray(data, (unsigned int *)(inptr), DataOut,
            DisplayExtent, cwindow, clevel, cshift, cscale, lut);
          break;
        case VTK_SHORT:
          vtkWin32ImageMapperRenderShortGray(data, (short *)(inptr), DataOut,
            DisplayExtent, cwindow, clevel, cshift, cscale, lut);
          break;
        case VTK_UNSIGNED_SHORT:
          vtkWin32ImageMapperRenderShortGray(data, (unsigned short *)(inptr), DataOut,
            DisplayExtent, cwindow, clevel, cshift, cscale, lut);
          break;
        case VTK_CHAR:
          vtkWin32ImageMapperRenderShortGray(data, (int *)(inptr), DataOut,
            DisplayExtent, cwindow, clevel, cshift, cscale, lut);
          break;
        case VTK_UNSIGNED_CHAR:
          vtkWin32ImageMapperRenderShortGray(data, (unsigned char *)(inptr), DataOut,
            DisplayExtent, cwindow, clevel, cshift, cscale, lut);
          break;
      }
    }
}
//----------------------------------------------------------------------------
// Expects data to be X, Y, components

void vtkWin32ImageMapper::RenderData(vtkViewport* viewport,
				     vtkImageData *data, vtkActor2D *actor)
{
  int width, height;
  void *inptr;
  float cshift, cscale, cwindow, clevel;
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
  width  = (this->DisplayExtent[1] - this->DisplayExtent[0] + 1);
  height = (this->DisplayExtent[3] - this->DisplayExtent[2] + 1);

  cshift  = this->GetColorShift();
  cscale  = this->GetColorScale();
  cwindow = this->GetColorWindow();
  clevel  = this->GetColorLevel();

  this->HBitmap = this->CreateBitmapObject(this->HBitmap, dataHeader, windowDC,
    this->DataOut, data, width, height);

  int dim = 0;
  dim = data->GetNumberOfScalarComponents();
  inptr = data->GetScalarPointer(
            this->DisplayExtent[0],
            this->DisplayExtent[2],
            this->DisplayExtent[4]);

  this->GenerateBitmapData(data, inptr, this->DataOut, dim,
    this->DisplayExtent, cwindow, clevel, cshift, cscale, this->LookupTable);


  compatDC = CreateCompatibleDC(windowDC);

  hOldBitmap = (HBITMAP)SelectObject(compatDC,this->HBitmap);

  // Get the position of the text actor
  int* actorPos =
    actor->GetPositionCoordinate()->GetComputedLocalDisplayValue(viewport);
  // negative positions will already be clipped to viewport
  actorPos[0] += this->PositionAdjustment[0];
  actorPos[1] -= this->PositionAdjustment[1];
  //
  if (!this->RenderToRectangle)
    {
    actorPos[1] = actorPos[1] - height + 1; // y axis is reversed!
    StretchBlt(windowDC,actorPos[0],actorPos[1],width, height, compatDC, 0,
      0,width,height,SRCCOPY);
    }
  else
    {
    int *topright = actor->GetPosition2Coordinate()->GetComputedLocalDisplayValue(viewport);
    int rectwidth  = (topright[0] - actorPos[0]) + 1;
    int rectheight = (actorPos[1] - topright[1]) + 1; // y axis is reversed!
    actorPos[1] = actorPos[1] - rectheight + 1;
    StretchBlt(windowDC,actorPos[0],actorPos[1],rectwidth, rectheight, compatDC, 0,
      0,width,height,SRCCOPY);
    }

  SelectObject(compatDC, hOldBitmap);
  DeleteDC(compatDC);

}

#endif
