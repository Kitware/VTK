/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagePaint.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

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
#include <math.h>
#include "vtkImagePaint.h"

//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImagePaint with no data.
vtkImagePaint::vtkImagePaint()
{
  this->DrawColor[0] = 0.0;
  this->DrawColor[1] = 0.0;
  this->DrawColor[2] = 0.0;
}


//----------------------------------------------------------------------------
// Description:
// Destructor: Deleting a vtkImagePaint automatically deletes the associated
// vtkImageData.  However, since the data is reference counted, it may not 
// actually be deleted.
vtkImagePaint::~vtkImagePaint()
{
  this->ReleaseData();
}


//----------------------------------------------------------------------------
void vtkImagePaint::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageRegion::PrintSelf(os,indent);
  os << indent << "DrawColor: (R=" << this->DrawColor[0] << ", G=" 
     <<this->DrawColor[1] << ", B=" <<this->DrawColor[2] << "/n";
}

//----------------------------------------------------------------------------
// Draw a region.  Only implentented for 2D extents.
template <class T>
void vtkImagePaintFillBox(vtkImagePaint *self, T *ptr, 
			 int min0, int max0, int min1, int max1)
{
  T *ptr0, *ptr1, *ptrV;
  int idx0, idx1;
  int inc0, inc1;
  int incV;
  float *drawColor;
  T   r, g, b;
  
  drawColor = self->GetDrawColor();
  r = (T)(drawColor[0]);
  g = (T)(drawColor[1]);
  b = (T)(drawColor[2]);
  
  self->GetIncrements(inc0, inc1);
  self->GetAxisIncrements(VTK_IMAGE_COMPONENT_AXIS, incV);
  ptr1 = ptr;
  for (idx1 = min1; idx1 <= max1; ++idx1)
    {
    ptr0 = ptr1;
    for (idx0 = min0; idx0 <= max0; ++idx0)
      {
      ptrV = ptr0;
      
      // Assign color to pixel.
      *ptrV = r;
      ptrV += incV;
      *ptrV = g;
      ptrV += incV;
      *ptrV = b;

      ptr0 += inc0;
      }
    ptr1 += inc1;
    }
}

//----------------------------------------------------------------------------
// Description:
// Draw a region.  Only implentented for 2D extents.
void vtkImagePaint::FillBox(int min0, int max0, int min1, int max1)
{
  int *extent;
  void *ptr;
  
  // Clip the region to keep in in bounds
  extent = this->GetExtent();
  min0 = (min0 < extent[0]) ? extent[0] : min0;
  max0 = (max0 < extent[0]) ? extent[0] : max0;
  min0 = (min0 > extent[1]) ? extent[1] : min0;
  max0 = (max0 > extent[1]) ? extent[1] : max0;
  min1 = (min1 < extent[0]) ? extent[0] : min1;
  max1 = (max1 < extent[0]) ? extent[0] : max1;
  min1 = (min1 > extent[1]) ? extent[1] : min1;
  max1 = (max1 > extent[1]) ? extent[1] : max1;
  
  ptr = this->GetScalarPointer(min0, min1);
  switch (this->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImagePaintFillBox(this, (float *)(ptr), min0,max0, min1,max1);
      break;
    case VTK_INT:
      vtkImagePaintFillBox(this, (int *)(ptr), min0,max0, min1,max1);
      break;
    case VTK_SHORT:
      vtkImagePaintFillBox(this, (short *)(ptr), min0,max0, min1,max1);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImagePaintFillBox(this, (unsigned short *)(ptr), min0,max0, min1,max1);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImagePaintFillBox(this, (unsigned char *)(ptr), min0,max0, min1,max1);
      break;
    default:
      vtkErrorMacro(<< "FillBox: Cannot handle ScalarType.");
    }   
}



//----------------------------------------------------------------------------
// Fill a tube (thick line for initial 2D implementation.
template <class T>
void vtkImagePaintFillTube(vtkImagePaint *self, T *ptr, 
			  int a0, int a1, int b0, int b1, float radius)
{
  T *ptr0, *ptr1, *ptrV;
  int idx0, idx1;
  int inc0, inc1, incV;
  int min0, max0, min1, max1;
  float *drawColor;
  T   r, g, b;
  int n0, n1;
  int ak, bk, k;
  float fract;
  float v0, v1;
  
  // Compute vector of tube.
  n0 = a0 - b0;
  n1 = a1 - b1;
  // compute the projects of the two points a and b on this vector.
  ak = n0 * a0 + n1 * a1;
  bk = n0 * b0 + n1 * b1;
  // Make sure the vector is pointing in the correct direction.
  if (ak < bk)
    {
    ak = -ak;
    bk = -bk;
    n0 = -n0;
    n1 = -n1;
    }

  drawColor = self->GetDrawColor();
  r = (T)(drawColor[0]);
  g = (T)(drawColor[1]);
  b = (T)(drawColor[2]);
  
  self->GetExtent(min0, max0, min1, max1);
  // Loop trough whole extent.
  self->GetIncrements(inc0, inc1);
  self->GetAxisIncrements(VTK_IMAGE_COMPONENT_AXIS, incV);
  ptr1 = ptr;
  for (idx1 = min1; idx1 <= max1; ++idx1)
    {
    ptr0 = ptr1;
    for (idx0 = min0; idx0 <= max0; ++idx0)
      {
      // check to see if pixel is in the tube.
      // project point onto normal vector.
      k = n0 * idx0 + n1 * idx1;
      // Check that point is inbetween end points.
      if ( k >= bk && k <= ak)
	{
	// Compute actual projection point.
	fract = (float)(k - bk) / (float)(ak - bk);
	v0 = b0 + fract * (float)(a0 - b0);
	v1 = b1 + fract * (float)(a1 - b1);
	// Compute distance to tube
	v0 -= (float)(idx0);
	v1 -= (float)(idx1);
	if (radius >= sqrt(v0*v0 + v1*v1))
	  {
	  ptrV = ptr0;
	  // Assign color to pixel.
	  *ptrV = r;
	  ptrV += incV;
	  *ptrV = g;
	  ptrV += incV;
	  *ptrV = b;	  
	  }
	}
      
      ptr0 += inc0;
      }
    ptr1 += inc1;
    }
}

//----------------------------------------------------------------------------
// Description:
// Fill a tube (thick line for initial 2D implementation).
void vtkImagePaint::FillTube(int a0, int a1, int b0, int b1, float radius)
{
  void *ptr;
  
  ptr = this->GetScalarPointer();
  switch (this->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImagePaintFillTube(this, (float *)(ptr), a0,a1, b0,b1, radius);
      break;
    case VTK_INT:
      vtkImagePaintFillTube(this, (int *)(ptr), a0,a1, b0,b1, radius);
      break;
    case VTK_SHORT:
      vtkImagePaintFillTube(this, (short *)(ptr), a0,a1, b0,b1, radius);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImagePaintFillTube(this, (unsigned short *)(ptr),
			   a0,a1, b0,b1, radius);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImagePaintFillTube(this, (unsigned char *)(ptr), 
			   a0,a1, b0,b1, radius);
      break;
    default:
      vtkErrorMacro(<< "FillTube: Cannot handle ScalarType.");
    }   
}





//----------------------------------------------------------------------------
// Fill a triangle (rasterize)
template <class T>
void vtkImagePaintFillTriangle(vtkImagePaint *self, T *ptr, 
			      int a0, int a1, int b0, int b1, int c0, int c1)
{
  int temp;
  float longT, shortT;  // end points of intersection of trainge and row.
  float longStep, shortStep;
  int left, right;
  int idx0, idx1;
  float *drawColor;
  T   r, g, b;
  int incV;
  
  self->GetAxisIncrements(VTK_IMAGE_COMPONENT_AXIS, incV);
  ptr = ptr;

  drawColor = self->GetDrawColor();
  r = (T)(drawColor[0]);
  g = (T)(drawColor[1]);
  b = (T)(drawColor[2]);
  
  // index1 of b must be between a, and c
  if((b1 < a1 && a1 < c1) || (b1 > a1 && a1 > c1))
    { // swap b and a 
    temp = b0;  b0 = a0;  a0 = temp;
    temp = b1;  b1 = a1;  a1 = temp;
    }
  if((b1 < c1 && c1 < a1) || (b1 > c1 && c1 > a1))
    { // swap b and c 
    temp = b0;  b0 = c0;  c0 = temp;
    temp = b1;  b1 = c1;  c1 = temp;
    }
  // Make life easier and order points so that ay < by < cy 
  if(c1 < a1)
    { // swap c and a
    temp = a0;  a0 = c0;  c0 = temp;
    temp = a1;  a1 = c1;  c1 = temp;
    }
  
  
  // for all rows: compute 2 points, intersection of triangle edges and row
  longStep = (float)(c0 - a0) / (float)(c1 - a1 + 1);
  longT = (float)(a0) + (0.5 * longStep);
  shortStep = (float)(b0 - a0) / (float)(b1 - a1 + 1);
  shortT = (float)(a0) + (0.5 * shortStep);
  for (idx1 = a1; idx1 < b1; ++idx1)
    {
    // Fill row long to short (y = idx1)
    left = (int)(shortT + 0.5);
    right = (int)(longT + 0.5);
    if (left > right)
      {
      temp = left;   left = right;   right = temp;
      }
    for (idx0 = left; idx0 <= right; ++idx0)
      {
      ptr = (T *)(self->GetScalarPointer(idx0, idx1));
      if (ptr)
	{
	*ptr = r;
	ptr += incV;
	*ptr = g;
	ptr += incV;
	*ptr = b;
	}
      }

    longT += longStep;
    shortT += shortStep;
    }

  // fill the second half of the triangle
  shortStep = (float)(c0 - b0) / (float)(c1 - b1 + 1);
  shortT = (float)(b0) + (0.5 * shortStep);
  for (idx1 = b1; idx1 < c1; ++idx1)
    {
    // Fill row long to short (y = idx1)
    left = (int)(shortT + 0.5);
    right = (int)(longT + 0.5);
    if (left > right)
      {
      temp = left;   left = right;   right = temp;
      }
    for (idx0 = left; idx0 <= right; ++idx0)
      {
      ptr = (T *)(self->GetScalarPointer(idx0, idx1));
      if (ptr)
	{
	*ptr = r;
	ptr += incV;
	*ptr = g;
	ptr += incV;
	*ptr = b;
	}
      }

    longT += longStep;
    shortT += shortStep;
    }
}

//----------------------------------------------------------------------------
// Description:
// Fill a tube (thick line for initial 2D implementation).
void vtkImagePaint::FillTriangle(int a0, int a1, int b0, int b1, int c0, int c1)
{
  void *ptr;
  
  ptr = this->GetScalarPointer();
  switch (this->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImagePaintFillTriangle(this, (float *)(ptr), a0,a1, b0,b1, c0,c1);
      break;
    case VTK_INT:
      vtkImagePaintFillTriangle(this, (int *)(ptr), a0,a1, b0,b1, c0,c1);
      break;
    case VTK_SHORT:
      vtkImagePaintFillTriangle(this, (short *)(ptr), a0,a1, b0,b1, c0,c1);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImagePaintFillTriangle(this, (unsigned short *)(ptr),
			       a0,a1, b0,b1, c0,c1);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImagePaintFillTriangle(this, (unsigned char *)(ptr), 
			       a0,a1, b0,b1, c0,c1);
      break;
    default:
      vtkErrorMacro(<< "FillTriangle: Cannot handle ScalarType.");
    }   
}





//----------------------------------------------------------------------------
// Draw a point.  Only implentented for 2D images.
template <class T>
void vtkImagePaintDrawPoint(vtkImagePaint *self, T *ptr, int p0, int p1)
{
  int min0, max0, min1, max1;
  int incV;
  float *drawColor;
  T   r, g, b;
  
  self->GetExtent(min0, max0, min1, max1);

  drawColor = self->GetDrawColor();
  r = (T)(drawColor[0]);
  g = (T)(drawColor[1]);
  b = (T)(drawColor[2]);

  self->GetAxisIncrements(VTK_IMAGE_COMPONENT_AXIS, incV);

  if (p0 >= min0 && p0 <= max0 && p1 >= min1 && p1 <= max1)
    {
    ptr = (T *)(self->GetScalarPointer(p0, p1));
    // Assign color to pixel.
    *ptr = r;
    ptr += incV;
    *ptr = g;
    ptr += incV;
    }
}



//----------------------------------------------------------------------------
// Description:
// Draw a circle
void vtkImagePaint::DrawPoint(int p0, int p1)
{
  void *ptr = NULL;
  
  vtkDebugMacro(<< "Drawing a point: (" << p0 << ", " << p1 << ")");
  
  switch (this->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImagePaintDrawPoint(this, (float *)(ptr), p0, p1);
      break;
    case VTK_INT:
      vtkImagePaintDrawPoint(this, (int *)(ptr), p0, p1);
      break;
    case VTK_SHORT:
      vtkImagePaintDrawPoint(this, (short *)(ptr), p0, p1);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImagePaintDrawPoint(this, (unsigned short *)(ptr), p0, p1);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImagePaintDrawPoint(this, (unsigned char *)(ptr), p0, p1);
      break;
    default:
      vtkErrorMacro(<< "DrawPoint: Cannot handle ScalarType.");
    }   
}



//----------------------------------------------------------------------------
// Draw a circle.  Only implentented for 2D images.
template <class T>
void vtkImagePaintDrawCircle(vtkImagePaint *self, T *ptr, 
			     int c0, int c1, float radius)
{
  int min0, max0, min1, max1;
  int incV;
  float *drawColor;
  T   r, g, b;
  int numberOfSteps;
  double thetaCos, thetaSin;
  double x, y, temp;
  int p0, p1;
  int idx;
  
  self->GetExtent(min0, max0, min1, max1);

  numberOfSteps = (int)(ceil(6.2831853 * radius));
  thetaCos = cos(1.0 / radius);
  thetaSin = sin(1.0 / radius);
  x = radius;
  y = 0.0;
  
  drawColor = self->GetDrawColor();
  r = (T)(drawColor[0]);
  g = (T)(drawColor[1]);
  b = (T)(drawColor[2]);
  
  self->GetAxisIncrements(VTK_IMAGE_COMPONENT_AXIS, incV);

  for (idx = 0; idx < numberOfSteps; ++idx)
    {
    p0 = c0+(int)(x);
    p1 = c1+(int)(y);
    if (p0 >= min0 && p0 <= max0 && p1 >= min1 && p1 <= max1)
      {
      ptr = (T *)(self->GetScalarPointer(p0, p1));
      // Assign color to pixel.
      *ptr = r;
      ptr += incV;
      *ptr = g;
      ptr += incV;
      *ptr = b;	  
      }
    
    // rotate the point
    temp = thetaCos * x + thetaSin * y;
    y = thetaCos * y - thetaSin * x;
    x = temp;
    }
}


//----------------------------------------------------------------------------
// Description:
// Draw a circle
void vtkImagePaint::DrawCircle(int c0, int c1, float radius)
{
  void *ptr = NULL;
  
  vtkDebugMacro(<< "Drawing a circle: center = (" << c0 << ", " << c1 
                << "), radius = " << radius);
  
  switch (this->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImagePaintDrawCircle(this, (float *)(ptr), c0, c1, radius);
      break;
    case VTK_INT:
      vtkImagePaintDrawCircle(this, (int *)(ptr), c0, c1, radius);
      break;
    case VTK_SHORT:
      vtkImagePaintDrawCircle(this, (short *)(ptr), c0, c1, radius);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImagePaintDrawCircle(this, (unsigned short *)(ptr), c0, c1, radius);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImagePaintDrawCircle(this, (unsigned char *)(ptr), c0, c1, radius);
      break;
    default:
      vtkErrorMacro(<< "DrawCircle: Cannot handle ScalarType.");
    }   
}



//----------------------------------------------------------------------------
// Draw a line.  Only implentented for 2D images.
// First point is already shifted to origin.
template <class T>
void vtkImagePaintDrawSegment(vtkImagePaint *self, T *ptr, 
			     int p0, int p1)
{
  float f0, f1;
  float s0, s1;
  int numberOfSteps;
  int idx;
  int inc0, inc1, incV;
  float *drawColor;
  T   r, g, b;
  T *ptrV;
  
  
  drawColor = self->GetDrawColor();
  r = (T)(drawColor[0]);
  g = (T)(drawColor[1]);
  b = (T)(drawColor[2]);

  // make sure we are stepping in the positive direction.
  self->GetIncrements(inc0, inc1);
  self->GetAxisIncrements(VTK_IMAGE_COMPONENT_AXIS, incV);
  if (p0 < 0)
    {
    p0 = -p0;
    inc0 = -inc0;
    }
  if (p1 < 0)
    {
    p1 = -p1;
    inc1 = -inc1;
    }
    
  // Compute the number of steps needed.
  if (p0 > p1)
    {
    numberOfSteps = p0;
    }
  else
    {
    numberOfSteps = p1;
    }

  // Compute the step vector.
  s0 = (float)(p0) / (float)(numberOfSteps);
  s1 = (float)(p1) / (float)(numberOfSteps);

  f0 = f1 = 0.5;

  ptrV = ptr;
  // Assign color to pixel.
  *ptrV = r;
  ptrV += incV;
  *ptrV = g;
  ptrV += incV;
  *ptrV = b;	  

  for (idx = 0; idx < numberOfSteps; ++idx)
    {
    f0 += s0;
    if (f0 > 1.0)
      {
      ptr += inc0;
      f0 -= 1.0;
      }
    f1 += s1;
    if (f1 > 1.0)
      {
      ptr += inc1;
      f1 -= 1.0;
      }

    ptrV = ptr;
    // Assign color to pixel.
    *ptrV = r;
    ptrV += incV;
    *ptrV = g;
    ptrV += incV;
    *ptrV = b;	  
    }
}


//----------------------------------------------------------------------------
// Description:
// Draw a Segment from point a to point b.
void vtkImagePaint::DrawSegment(int a0, int a1, int b0, int b1)
{
  int *extent;
  void *ptr;
  
  vtkDebugMacro(<< "Drawing a segment: " << a0 << ", " << a1 << " to "
                << b0 << ", " << b1);
  
  // check to make sure line segment is in bounds.
  extent = this->GetExtent();
  if (a0 < extent[0] || a0 > extent[1] || b0 < extent[0] || b0 > extent[1] ||
      a1 < extent[2] || a1 > extent[3] || b1 < extent[2] || b1 > extent[3])
    {
    if ( ! this->ClipSegment(a0,a1,b0,b1))
      {
      // non of the segment is in the region.
      return;
      }
    }

  ptr = this->GetScalarPointer(b0, b1);
  a0 -= b0;
  a1 -= b1;
  switch (this->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImagePaintDrawSegment(this, (float *)(ptr), a0, a1);
      break;
    case VTK_INT:
      vtkImagePaintDrawSegment(this, (int *)(ptr), a0, a1);
      break;
    case VTK_SHORT:
      vtkImagePaintDrawSegment(this, (short *)(ptr), a0, a1);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImagePaintDrawSegment(this, (unsigned short *)(ptr), a0, a1);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImagePaintDrawSegment(this, (unsigned char *)(ptr), a0, a1);
      break;
    default:
      vtkErrorMacro(<< "DrawSegment: Cannot handle ScalarType.");
    }   
}



//----------------------------------------------------------------------------
// Description:
// Clips a line segment so it will be in bounds.
// If the entire segment is out of bounds, the method returns 0.
int vtkImagePaint::ClipSegment(int &a0, int &a1, int &b0, int &b1)
{
  int min0, max0, min1, max1;
  float fract;

  
  this->GetExtent(min0, max0, min1, max1);
  
  // Check planes
  // Both out of bounds
  if (a0 < min0 && b0 < min0)
    {
    return 0;
    }
  // first out of bounds.
  if (a0 < min0 && b0 >= min0)
    {
    // interpolate to find point on bounding plane.
    fract = (float)(b0 - min0) / (float)(b0 - a0);
    a0 = min0;
    a1 = b1 + (int)(fract * (float)(a1 - b1));
    }
  // second out of bounds.
  if (b0 < min0 && a0 >= min0)
    {
    // interpolate to find point on bounding plane.
    fract = (float)(a0 - min0) / (float)(a0 - b0);
    b0 = min0;
    b1 = a1 + (int)(fract * (float)(b1 - a1));
    }

  // Both out of bounds
  if (a0 > max0 && b0 > max0)
    {
    return 0;
    }
  // first out of bounds.
  if (a0 > max0 && b0 <= max0)
    {
    // interpolate to find point on bounding plane.
    fract = (float)(b0 - max0) / (float)(b0 - a0);
    a0 = max0;
    a1 = b1 + (int)(fract * (float)(a1 - b1));
    }
  // second out of bounds.
  if (b0 > max0 && a0 <= max0)
    {
    // interpolate to find point on bounding plane.
    fract = (float)(a0 - max0) / (float)(a0 - b0);
    b0 = max0;
    b1 = a1 + (int)(fract * (float)(b1 - a1));
    }
  

  // Both out of bounds
  if (a1 < min1 && b1 < min1)
    {
    return 0;
    }
  // first out of bounds.
  if (a1 < min1 && b1 >= min1)
    {
    // interpolate to find point on bounding plane.
    fract = (float)(b1 - min1) / (float)(b1 - a1);
    a1 = min1;
    a0 = b0 + (int)(fract * (float)(a0 - b0));
    }
  // second out of bounds.
  if (b1 < min1 && a1 >= min1)
    {
    // interpolate to find point on bounding plane.
    fract = (float)(a1 - min1) / (float)(a1 - b1);
    b1 = min1;
    b0 = a0 + (int)(fract * (float)(b0 - a0));
    }

  // Both out of bounds
  if (a1 > max1 && b1 > max1)
    {
    return 0;
    }
  // first out of bounds.
  if (a1 > max1 && b1 <= max1)
    {
    // interpolate to find point on bounding plane.
    fract = (float)(b1 - max1) / (float)(b1 - a1);
    a1 = max1;
    a0 = b0 + (int)(fract * (float)(a0 - b0));
    }
  // second out of bounds.
  if (b1 > max1 && a1 <= max1)
    {
    // interpolate to find point on bounding plane.
    fract = (float)(a1 - max1) / (float)(a1 - b1);
    b1 = max1;
    b0 = a0 + (int)(fract * (float)(b0 - a0));
    }
  
  return 1;
}











//----------------------------------------------------------------------------
// Draw a line.  Only implentented for 3D images.
// First point is already shifted to origin.
template <class T>
void vtkImagePaintDrawSegment3D(vtkImagePaint *self, T *ptr, 
			       int p0, int p1, int p2)
{
  float f0, f1, f2;
  float s0, s1, s2;
  int numberOfSteps;
  int idx;
  int inc0, inc1, inc2, incV;
  float *drawColor;
  T   r, g, b;
  T *ptrV;
  
  
  drawColor = self->GetDrawColor();
  r = (T)(drawColor[0]);
  g = (T)(drawColor[1]);
  b = (T)(drawColor[2]);

  // make sure we are stepping in the positive direction.
  self->GetIncrements(inc0, inc1, inc2);
  self->GetAxisIncrements(VTK_IMAGE_COMPONENT_AXIS, incV);
  if (p0 < 0)
    {
    p0 = -p0;
    inc0 = -inc0;
    }
  if (p1 < 0)
    {
    p1 = -p1;
    inc1 = -inc1;
    }
  if (p2 < 0)
    {
    p2 = -p2;
    inc2 = -inc2;
    }
    
  // Compute the number of steps needed.
  numberOfSteps = (p0 > p1) ? p0 : p1;
  numberOfSteps = (numberOfSteps > p2) ? numberOfSteps : p2;

  // Compute the step vector.
  s0 = (float)(p0) / (float)(numberOfSteps);
  s1 = (float)(p1) / (float)(numberOfSteps);
  s2 = (float)(p2) / (float)(numberOfSteps);

  f0 = f1 = f2 = 0.5;

  
  ptrV = ptr;
  // Assign color to pixel.
  *ptrV = r;
  ptrV += incV;
  *ptrV = g;
  ptrV += incV;
  *ptrV = b;	 
  
  for (idx = 0; idx < numberOfSteps; ++idx)
    {
    f0 += s0;
    if (f0 > 1.0)
      {
      ptr += inc0;
      f0 -= 1.0;
      }
    f1 += s1;
    if (f1 > 1.0)
      {
      ptr += inc1;
      f1 -= 1.0;
      }
    f2 += s2;
    if (f2 > 1.0)
      {
      ptr += inc2;
      f2 -= 1.0;
      }

    ptrV = ptr;
    // Assign color to pixel.
    *ptrV = r;
    ptrV += incV;
    *ptrV = g;
    ptrV += incV;
    *ptrV = b;	 
    }
}


//----------------------------------------------------------------------------
// Description:
// Draw a Segment from point a to point b.
// No clipping or bounds checking.
void vtkImagePaint::DrawSegment3D(float *a, float *b)
{
  void *ptr;
  int a0, a1, a2;
  
  ptr = this->GetScalarPointer((int)(b[0] + 0.5), 
			       (int)(b[1] + 0.5), 
			       (int)(b[2] + 0.5));
  a0 = (int)(a[0] - b[0] + 0.5);
  a1 = (int)(a[1] - b[1] + 0.5);
  a2 = (int)(a[2] - b[2] + 0.5);
  switch (this->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImagePaintDrawSegment3D(this, (float *)(ptr), a0, a1, a2);
      break;
    case VTK_INT:
      vtkImagePaintDrawSegment3D(this, (int *)(ptr), a0, a1, a2);
      break;
    case VTK_SHORT:
      vtkImagePaintDrawSegment3D(this, (short *)(ptr), a0, a1, a2);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImagePaintDrawSegment3D(this, (unsigned short *)(ptr), a0, a1, a2);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImagePaintDrawSegment3D(this, (unsigned char *)(ptr), a0, a1, a2);
      break;
    default:
      vtkErrorMacro(<< "DrawSegment3D: Cannot handle ScalarType.");
    }   
}



  


