/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImagePainter2D.cxx
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
#include "vtkImagePainter2D.h"

//----------------------------------------------------------------------------
// Description:
// Construct an instance of vtkImagePainter2D with no data.
vtkImagePainter2D::vtkImagePainter2D()
{
  int idx;

  this->ImageRegion = this;
  for (idx = 0; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    this->DrawColor[idx] = 0.0;
    }
}


//----------------------------------------------------------------------------
// Description:
// Destructor: Deleting a vtkImagePainter2D automatically deletes the associated
// vtkImageData.  However, since the data is reference counted, it may not 
// actually be deleted.
vtkImagePainter2D::~vtkImagePainter2D()
{
  this->ReleaseData();
}


//----------------------------------------------------------------------------
void vtkImagePainter2D::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx, num, min, max;
  
  vtkImageRegion::PrintSelf(os,indent);
  os << indent << "ImageRegion: (" << this->ImageRegion << ")\n";
  os << indent << "DrawColor: (" << this->DrawColor[0];
  this->GetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, min, max);
  num = max - min + 1;
  for (idx = 1; idx < num; ++idx)
    {
    os << ", " << this->DrawColor[idx];
    }
  os << ")\n";
}



//----------------------------------------------------------------------------
void vtkImagePainter2D::SetDrawColor(int num, float *color)
{
  int idx, min, max;
  
  this->ImageRegion->GetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, min, max);
  if (num != (max - min + 1))
    {
    vtkErrorMacro(<< "Color dimensions, " << num 
       << ", does not match component extent (" << min << ", " << max << ")");
    return;
    }
  
  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkErrorMacro(<< "Cannot this long of a 'color'");
    return;
    }
    
  for (idx = 0; idx < num; ++idx)
    {
    this->DrawColor[idx] = color[idx];
    }
}

//----------------------------------------------------------------------------
void vtkImagePainter2D::GetDrawColor(int num, float *color)
{
  int idx, min, max;
  
  this->ImageRegion->GetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, min, max);
  if (num != (max - min + 1))
    {
    vtkErrorMacro(<< "Color dimensions, " << num 
       << ", does not match component extent (" << min << ", " << max << ")");
    return;
    }
  
  if (num > VTK_IMAGE_DIMENSIONS)
    {
    vtkErrorMacro(<< "Cannot this long of a 'color'");
    return;
    }
    
  for (idx = 0; idx < num; ++idx)
    {
    color[idx] = this->DrawColor[idx];
    }
}


//----------------------------------------------------------------------------
// Draw a region.  Only implentented for 2D extents.
template <class T>
static void vtkImagePainter2DFillBox(vtkImageRegion *image, 
				 float *drawColor, T *ptr, 
				 int min0, int max0, int min1, int max1)
{
  T *ptr0, *ptr1, *ptrV;
  int idx0, idx1, idxV;
  int inc0, inc1, incV;
  int minV, maxV;
  float *pf;
  
  image->GetIncrements(inc0, inc1);
  image->GetAxisIncrements(VTK_IMAGE_COMPONENT_AXIS, incV);
  image->GetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, minV, maxV);
  ptr1 = ptr;
  for (idx1 = min1; idx1 <= max1; ++idx1)
    {
    ptr0 = ptr1;
    for (idx0 = min0; idx0 <= max0; ++idx0)
      {
      ptrV = ptr0;
      pf = drawColor;
      
      // Assign color to pixel.
      for (idxV = minV; idxV <= maxV; ++idxV)
	{
	*ptrV = (T)(*pf++);
	ptrV += incV;
	}
      
      ptr0 += inc0;
      }
    ptr1 += inc1;
    }
}

//----------------------------------------------------------------------------
// Description:
// Draw a region.  Only implentented for 2D extents.
void vtkImagePainter2D::FillBox(int min0, int max0, int min1, int max1)
{
  int *extent;
  void *ptr;
  
  // Clip the region to keep in in bounds
  extent = this->ImageRegion->GetExtent();
  min0 = (min0 < extent[0]) ? extent[0] : min0;
  max0 = (max0 < extent[0]) ? extent[0] : max0;
  min0 = (min0 > extent[1]) ? extent[1] : min0;
  max0 = (max0 > extent[1]) ? extent[1] : max0;
  min1 = (min1 < extent[2]) ? extent[2] : min1;
  max1 = (max1 < extent[2]) ? extent[2] : max1;
  min1 = (min1 > extent[3]) ? extent[3] : min1;
  max1 = (max1 > extent[3]) ? extent[3] : max1;
  
  ptr = this->ImageRegion->GetScalarPointer(min0, min1);
  switch (this->ImageRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImagePainter2DFillBox(this->ImageRegion, this->DrawColor, 
			   (float *)(ptr), min0,max0, min1,max1);
      break;
    case VTK_INT:
      vtkImagePainter2DFillBox(this->ImageRegion, this->DrawColor, 
			   (int *)(ptr), min0,max0, min1,max1);
      break;
    case VTK_SHORT:
      vtkImagePainter2DFillBox(this->ImageRegion, this->DrawColor, 
			   (short *)(ptr), min0,max0, min1,max1);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImagePainter2DFillBox(this->ImageRegion, this->DrawColor, 
			   (unsigned short *)(ptr), min0,max0, min1,max1);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImagePainter2DFillBox(this->ImageRegion, this->DrawColor, 
			   (unsigned char *)(ptr), min0,max0, min1,max1);
      break;
    default:
      vtkErrorMacro(<< "FillBox: Cannot handle ScalarType.");
    }   
}



//----------------------------------------------------------------------------
// Fill a tube (thick line for initial 2D implementation.
template <class T>
static void vtkImagePainter2DFillTube(vtkImageRegion *image, 
				  float *drawColor, T *ptr, 
				  int a0, int a1, int b0, int b1, float radius)
{
  T *ptr0, *ptr1, *ptrV;
  int idx0, idx1, idxV;
  int inc0, inc1, incV;
  int min0, max0, min1, max1, minV, maxV;
  float *pf;
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

  image->GetExtent(min0, max0, min1, max1);
  image->GetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, minV, maxV);
  // Loop trough whole extent.
  image->GetIncrements(inc0, inc1);
  image->GetAxisIncrements(VTK_IMAGE_COMPONENT_AXIS, incV);
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
	  pf = drawColor;

	  // Assign color to pixel.
	  for (idxV = minV; idxV <= maxV; ++idxV)
	    {
	    *ptrV = (T)(*pf++);
	    ptrV += incV;
	    }
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
void vtkImagePainter2D::FillTube(int a0, int a1, int b0, int b1, float radius)
{
  void *ptr;
  
  ptr = this->ImageRegion->GetScalarPointer();
  switch (this->ImageRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImagePainter2DFillTube(this->ImageRegion, this->DrawColor, 
			    (float *)(ptr), a0,a1, b0,b1, radius);
      break;
    case VTK_INT:
      vtkImagePainter2DFillTube(this->ImageRegion, this->DrawColor, 
			    (int *)(ptr), a0,a1, b0,b1, radius);
      break;
    case VTK_SHORT:
      vtkImagePainter2DFillTube(this->ImageRegion, this->DrawColor,
			    (short *)(ptr), a0,a1, b0,b1, radius);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImagePainter2DFillTube(this->ImageRegion, this->DrawColor, 
			    (unsigned short *)(ptr), a0,a1, b0,b1, radius);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImagePainter2DFillTube(this->ImageRegion, this->DrawColor,
			    (unsigned char *)(ptr), a0,a1, b0,b1, radius);
      break;
    default:
      vtkErrorMacro(<< "FillTube: Cannot handle ScalarType.");
    }   
}





//----------------------------------------------------------------------------
// Fill a triangle (rasterize)
template <class T>
static void vtkImagePainter2DFillTriangle(vtkImageRegion *image, 
				      float *drawColor, T *ptr, int a0, int a1,
				      int b0, int b1, int c0, int c1)
{
  int temp;
  float longT, shortT;  // end points of intersection of trainge and row.
  float longStep, shortStep;
  int left, right;
  int idx0, idx1, idxV;
  int min0, max0, min1, max1;
  int minV, maxV;
  float *pf;
  int incV;
  
  ptr = ptr;
  image->GetAxisIncrements(VTK_IMAGE_COMPONENT_AXIS, incV);
  image->GetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, minV, maxV);
  
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
      if (idx0 >= min0 && idx0 <= max0 && idx1 >= min1 && idx1 <= max1)
	{
	ptr = (T *)(image->GetScalarPointer(idx0, idx1));
	if (ptr)
	  {
	  pf = drawColor;
	  // Assign color to pixel.
	  for (idxV = minV; idxV <= maxV; ++idxV)
	    {
	    *ptr = (T)(*pf++);
	    ptr += incV;
	    }
	  }
	
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
      ptr = (T *)(image->GetScalarPointer(idx0, idx1));
      if (ptr)
	{
	pf = drawColor;
	// Assign color to pixel.
	for (idxV = minV; idxV <= maxV; ++idxV)
	  {
	  *ptr = (T)(*pf++);
	  ptr += incV;
	  }
	
	}
      }

    longT += longStep;
    shortT += shortStep;
    }
}

//----------------------------------------------------------------------------
// Description:
// Fill a tube (thick line for initial 2D implementation).
void vtkImagePainter2D::FillTriangle(int a0,int a1, int b0,int b1, int c0,int c1)
{
  void *ptr;
  
  ptr = this->ImageRegion->GetScalarPointer();
  switch (this->ImageRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImagePainter2DFillTriangle(this->ImageRegion, this->DrawColor, 
				(float *)(ptr), a0,a1, b0,b1, c0,c1);
      break;
    case VTK_INT:
      vtkImagePainter2DFillTriangle(this->ImageRegion, this->DrawColor, 
				(int *)(ptr), a0,a1, b0,b1, c0,c1);
      break;
    case VTK_SHORT:
      vtkImagePainter2DFillTriangle(this->ImageRegion, this->DrawColor, 
				(short *)(ptr), a0,a1, b0,b1, c0,c1);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImagePainter2DFillTriangle(this->ImageRegion, this->DrawColor, 
				(unsigned short *)(ptr),
			       a0,a1, b0,b1, c0,c1);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImagePainter2DFillTriangle(this->ImageRegion, this->DrawColor, 
				(unsigned char *)(ptr), a0,a1, b0,b1, c0,c1);
      break;
    default:
      vtkErrorMacro(<< "FillTriangle: Cannot handle ScalarType.");
    }   
}





//----------------------------------------------------------------------------
// Draw a point.  Only implentented for 2D images.
template <class T>
static void vtkImagePainter2DDrawPoint(vtkImageRegion *image, 
				   float *drawColor, T *ptr, 
				   int p0, int p1)
{
  int min0, max0, min1, max1, minV, maxV;
  int incV, idxV;
  float *pf;
  
  image->GetExtent(min0, max0, min1, max1);
  image->GetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, minV, maxV);
  image->GetAxisIncrements(VTK_IMAGE_COMPONENT_AXIS, incV);

  if (p0 >= min0 && p0 <= max0 && p1 >= min1 && p1 <= max1)
    {
    ptr = (T *)(image->GetScalarPointer(p0, p1));

    pf = drawColor;
    // Assign color to pixel.
    for (idxV = minV; idxV <= maxV; ++idxV)
      {
      *ptr = (T)(*pf++);
      ptr += incV;
      }
    
    }
}



//----------------------------------------------------------------------------
// Description:
// Draw a circle
void vtkImagePainter2D::DrawPoint(int p0, int p1)
{
  void *ptr = NULL;
  
  vtkDebugMacro(<< "Drawing a point: (" << p0 << ", " << p1 << ")");
  
  switch (this->ImageRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImagePainter2DDrawPoint(this->ImageRegion, this->DrawColor, 
			     (float *)(ptr), p0, p1);
      break;
    case VTK_INT:
      vtkImagePainter2DDrawPoint(this->ImageRegion, this->DrawColor, 
			     (int *)(ptr), p0, p1);
      break;
    case VTK_SHORT:
      vtkImagePainter2DDrawPoint(this->ImageRegion, this->DrawColor, 
			     (short *)(ptr), p0, p1);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImagePainter2DDrawPoint(this->ImageRegion, this->DrawColor, 
			     (unsigned short *)(ptr), p0, p1);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImagePainter2DDrawPoint(this->ImageRegion, this->DrawColor, 
			     (unsigned char *)(ptr), p0, p1);
      break;
    default:
      vtkErrorMacro(<< "DrawPoint: Cannot handle ScalarType.");
    }   
}



//----------------------------------------------------------------------------
// Draw a circle.  Only implentented for 2D images.
template <class T>
static void vtkImagePainter2DDrawCircle(vtkImageRegion *image, 
				    float *drawColor, T *ptr, 
				    int c0, int c1, float radius)
{
  int min0, max0, min1, max1, minV, maxV;
  int incV, idxV;
  float *pf;
  int numberOfSteps;
  double thetaCos, thetaSin;
  double x, y, temp;
  int p0, p1;
  int idx;

  radius += 0.1;
  image->GetExtent(min0, max0, min1, max1);
  image->GetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, minV, maxV);
  image->GetAxisIncrements(VTK_IMAGE_COMPONENT_AXIS, incV);

  numberOfSteps = (int)(ceil(6.2831853 * radius));
  thetaCos = cos(1.0 / radius);
  thetaSin = sin(1.0 / radius);
  x = radius;
  y = 0.0;
  
  for (idx = 0; idx < numberOfSteps; ++idx)
    {
    p0 = c0+(int)(x);
    p1 = c1+(int)(y);
    if (p0 >= min0 && p0 <= max0 && p1 >= min1 && p1 <= max1)
      {
      ptr = (T *)(image->GetScalarPointer(p0, p1));

      pf = drawColor;
      // Assign color to pixel.
      for (idxV = minV; idxV <= maxV; ++idxV)
	{
	*ptr = (T)(*pf++);
	ptr += incV;
	}
      
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
void vtkImagePainter2D::DrawCircle(int c0, int c1, float radius)
{
  void *ptr = NULL;
  
  vtkDebugMacro(<< "Drawing a circle: center = (" << c0 << ", " << c1 
                << "), radius = " << radius);
  
  switch (this->ImageRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImagePainter2DDrawCircle(this->ImageRegion, this->DrawColor, 
			      (float *)(ptr), c0, c1, radius);
      break;
    case VTK_INT:
      vtkImagePainter2DDrawCircle(this->ImageRegion, this->DrawColor, 
			      (int *)(ptr), c0, c1, radius);
      break;
    case VTK_SHORT:
      vtkImagePainter2DDrawCircle(this->ImageRegion, this->DrawColor, 
			      (short *)(ptr), c0, c1, radius);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImagePainter2DDrawCircle(this->ImageRegion, this->DrawColor, 
			      (unsigned short *)(ptr), c0, c1, radius);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImagePainter2DDrawCircle(this->ImageRegion, this->DrawColor, 
			      (unsigned char *)(ptr), c0, c1, radius);
      break;
    default:
      vtkErrorMacro(<< "DrawCircle: Cannot handle ScalarType.");
    }   
}



//----------------------------------------------------------------------------
// Draw a line.  Only implentented for 2D images.
// First point is already shifted to origin.
template <class T>
static void vtkImagePainter2DDrawSegment(vtkImageRegion *image, 
				     float *drawColor, T *ptr, 
				     int p0, int p1)
{
  float f0, f1;
  float s0, s1;
  int numberOfSteps;
  int minV, maxV;
  int idx, idxV;
  int inc0, inc1, incV;
  float *pf;
  T *ptrV;
  
  
  image->GetIncrements(inc0, inc1);
  image->GetAxisIncrements(VTK_IMAGE_COMPONENT_AXIS, incV);
  image->GetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, minV, maxV);

  // make sure we are stepping in the positive direction.
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
  pf = drawColor;
  // Assign color to pixel.
  for (idxV = minV; idxV <= maxV; ++idxV)
    {
    *ptrV = (T)(*pf++);
    ptrV += incV;
    }
    

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
    pf = drawColor;
    
    // Assign color to pixel.
    for (idxV = minV; idxV <= maxV; ++idxV)
      {
      *ptrV = (T)(*pf++);
      ptrV += incV;
      }
    

    }
}


//----------------------------------------------------------------------------
// Description:
// Draw a Segment from point a to point b.
void vtkImagePainter2D::DrawSegment(int a0, int a1, int b0, int b1)
{
  int *extent;
  void *ptr;
  
  vtkDebugMacro(<< "Drawing a segment: " << a0 << ", " << a1 << " to "
                << b0 << ", " << b1);
  
  // check to make sure line segment is in bounds.
  extent = this->ImageRegion->GetExtent();
  if (a0 < extent[0] || a0 > extent[1] || b0 < extent[0] || b0 > extent[1] ||
      a1 < extent[2] || a1 > extent[3] || b1 < extent[2] || b1 > extent[3])
    {
    if ( ! this->ClipSegment(a0,a1,b0,b1))
      {
      // non of the segment is in the region.
      return;
      }
    }

  ptr = this->ImageRegion->GetScalarPointer(b0, b1);
  a0 -= b0;
  a1 -= b1;
  switch (this->ImageRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImagePainter2DDrawSegment(this->ImageRegion, this->DrawColor, 
			       (float *)(ptr), a0, a1);
      break;
    case VTK_INT:
      vtkImagePainter2DDrawSegment(this->ImageRegion, this->DrawColor, 
			       (int *)(ptr), a0, a1);
      break;
    case VTK_SHORT:
      vtkImagePainter2DDrawSegment(this->ImageRegion, this->DrawColor, 
			       (short *)(ptr), a0, a1);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImagePainter2DDrawSegment(this->ImageRegion, this->DrawColor, 
			       (unsigned short *)(ptr), a0, a1);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImagePainter2DDrawSegment(this->ImageRegion, this->DrawColor, 
			       (unsigned char *)(ptr), a0, a1);
      break;
    default:
      vtkErrorMacro(<< "DrawSegment: Cannot handle ScalarType.");
    }   
}



//----------------------------------------------------------------------------
// Description:
// Clips a line segment so it will be in bounds.
// If the entire segment is out of bounds, the method returns 0.
int vtkImagePainter2D::ClipSegment(int &a0, int &a1, int &b0, int &b1)
{
  int min0, max0, min1, max1;
  float fract;

  
  this->ImageRegion->GetExtent(min0, max0, min1, max1);
  
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
static void vtkImagePainter2DDrawSegment3D(vtkImageRegion *image, 
				       float *drawColor, 
				       T *ptr, int p0, int p1, int p2)
{
  float f0, f1, f2;
  float s0, s1, s2;
  int numberOfSteps;
  int idx, idxV, minV, maxV;
  int inc0, inc1, inc2, incV;
  float *pf;
  T *ptrV;
  
  
  image->GetIncrements(inc0, inc1, inc2);
  image->GetAxisIncrements(VTK_IMAGE_COMPONENT_AXIS, incV);
  image->GetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, minV, maxV);

  // make sure we are stepping in the positive direction.
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
  pf = drawColor;
  // Assign color to pixel.
  for (idxV = minV; idxV <= maxV; ++idxV)
    {
    *ptrV = (T)(*pf++);
    ptrV += incV;
    }
    
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
    pf = drawColor;
    // Assign color to pixel.
    for (idxV = minV; idxV <= maxV; ++idxV)
      {
      *ptrV = (T)(*pf++);
      ptrV += incV;
      }
    }
}


//----------------------------------------------------------------------------
// Description:
// Draw a Segment from point a to point b.
// No clipping or bounds checking.
void vtkImagePainter2D::DrawSegment3D(float *a, float *b)
{
  void *ptr;
  int a0, a1, a2;
  
  ptr = this->ImageRegion->GetScalarPointer((int)(b[0] + 0.5), 
					    (int)(b[1] + 0.5), 
					    (int)(b[2] + 0.5));
  a0 = (int)(a[0] - b[0] + 0.5);
  a1 = (int)(a[1] - b[1] + 0.5);
  a2 = (int)(a[2] - b[2] + 0.5);
  switch (this->ImageRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImagePainter2DDrawSegment3D(this->ImageRegion, this->DrawColor, 
				 (float *)(ptr), a0, a1, a2);
      break;
    case VTK_INT:
      vtkImagePainter2DDrawSegment3D(this->ImageRegion, this->DrawColor, 
				 (int *)(ptr), a0, a1, a2);
      break;
    case VTK_SHORT:
      vtkImagePainter2DDrawSegment3D(this->ImageRegion, this->DrawColor, 
				 (short *)(ptr), a0, a1, a2);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImagePainter2DDrawSegment3D(this->ImageRegion, this->DrawColor, 
				 (unsigned short *)(ptr), a0, a1, a2);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImagePainter2DDrawSegment3D(this->ImageRegion, this->DrawColor, 
				 (unsigned char *)(ptr), a0, a1, a2);
      break;
    default:
      vtkErrorMacro(<< "DrawSegment3D: Cannot handle ScalarType.");
    }   
}



//----------------------------------------------------------------------------
template <class T>
static void vtkImagePainter2DFill(vtkImageRegion *image, float *color, 
			      T *ptr, int x, int y)
{
  vtkImagePainter2DPixel *pixel;
  vtkImagePainter2DPixel *first, *last;
  vtkImagePainter2DPixel *heap = NULL;
  int min0, max0, min1, max1, minV, maxV;
  int idxV;
  int inc0, inc1, incV;
  T fillColor[10];
  T drawColor[10];
  T *ptrV, *ptrC;
  int temp;
  
  image->GetExtent(min0, max0, min1, max1);
  image->GetAxisExtent(VTK_IMAGE_COMPONENT_AXIS, minV, maxV);
  image->GetIncrements(inc0, inc1);
  image->GetAxisIncrements(VTK_IMAGE_COMPONENT_AXIS, incV);
  
  if ((maxV - minV + 1) > 10)
    {
    cerr << "Fill: Color vector too long";
    return;
    }
  
  // Copy the fill color and make sure it differs from drawColor.
  ptrV = ptr;
  temp = 1;
  for (idxV = minV; idxV <= maxV; ++idxV)
    {
    // Save the fill color
    fillColor[idxV-minV] = *ptrV;
    drawColor[idxV-minV] = (T)(color[idxV-minV]);
    if (*ptrV != drawColor[idxV-minV])
      {
      temp = 0;
      }
    ptrV += incV;
    }
  if (temp)
    { // fill the same as draw
    cerr << "Fill: Cannot handle draw color same as fill color";
    return;
    }
  
  // Create the seed
  pixel = new vtkImagePainter2DPixel;
  pixel->X = x;
  pixel->Y = y;
  pixel->Pointer = (void *)(ptr);
  pixel->Next = NULL;
  first = last = pixel;
  // change the seeds color
  ptrV = (T *)(last->Pointer);
  ptrC = drawColor;
  for (idxV = minV; idxV <= maxV; ++idxV)
    {
    *ptrV = *ptrC++;
    ptrV += incV;
    }
    
  
  while (first)
    {
    ptr = (T *)(first->Pointer);

    // check bounds for -x neighbor
    if (first->X > min0)
      {
      // Get the neighbor
      ptrV = ptr - inc0;
      // compare color
      ptrC = fillColor;
      temp = 1;
      for (idxV = minV; idxV <= maxV; ++idxV)
	{
	if (*ptrV != *ptrC++)
	  {
	  temp = 0;
	  break;
	  }
	ptrV += incV;
	}
      if (temp)
	{ // color match add a new seed to end of list
	if (heap)
	  {
	  pixel = heap;
	  heap = heap->Next;
	  }
	else
	  {
	  pixel = new vtkImagePainter2DPixel;
	  }
	pixel->X = first->X-1;
	pixel->Y = first->Y;
	pixel->Pointer = (void *)(ptr - inc0);
	pixel->Next = NULL;
	last->Next = pixel;
	last = pixel;
	// change the seeds color
	ptrV = (T *)(last->Pointer);
	ptrC = drawColor;
	for (idxV = minV; idxV <= maxV; ++idxV)
	  {
	  *ptrV = *ptrC++;
	  ptrV += incV;
	  }
	}
      }
  
    // check bounds for +x neighbor
    if (first->X < max0)
      {
      // Get the neighbor
      ptrV = ptr + inc0;
      // compare color
      ptrC = fillColor;
      temp = 1;
      for (idxV = minV; idxV <= maxV; ++idxV)
	{
	if (*ptrV != *ptrC++)
	  {
	  temp = 0;
	  break;
	  }
	ptrV += incV;
	}
      if (temp)
	{ // color match add a new seed to end of list
	if (heap)
	  {
	  pixel = heap;
	  heap = heap->Next;
	  }
	else
	  {
	  pixel = new vtkImagePainter2DPixel;
	  }
	pixel->X = first->X+1;
	pixel->Y = first->Y;
	pixel->Pointer = (void *)(ptr + inc0);
	pixel->Next = NULL;
	last->Next = pixel;
	last = pixel;
	// change the seeds color
	ptrV = (T *)(last->Pointer);
	ptrC = drawColor;
	for (idxV = minV; idxV <= maxV; ++idxV)
	  {
	  *ptrV = *ptrC++;
	  ptrV += incV;
	  }
	}
      }
  
    // check bounds for -y neighbor
    if (first->Y > min1)
      {
      // Get the neighbor
      ptrV = ptr - inc1;
      // compare color
      ptrC = fillColor;
      temp = 1;
      for (idxV = minV; idxV <= maxV; ++idxV)
	{
	if (*ptrV != *ptrC++)
	  {
	  temp = 0;
	  break;
	  }
	ptrV += incV;
	}
      if (temp)
	{ // color match add a new seed to end of list
	if (heap)
	  {
	  pixel = heap;
	  heap = heap->Next;
	  }
	else
	  {
	  pixel = new vtkImagePainter2DPixel;
	  }
	pixel->X = first->X;
	pixel->Y = first->Y-1;
	pixel->Pointer = (void *)(ptr - inc1);
	pixel->Next = NULL;
	last->Next = pixel;
	last = pixel;
	// change the seeds color
	ptrV = (T *)(last->Pointer);
	ptrC = drawColor;
	for (idxV = minV; idxV <= maxV; ++idxV)
	  {
	  *ptrV = *ptrC++;
	  ptrV += incV;
	  }
	}
      }
  
    // check bounds for +y neighbor
    if (first->Y < max1)
      {
      // Get the neighbor
      ptrV = ptr + inc1;
      // compare color
      ptrC = fillColor;
      temp = 1;
      for (idxV = minV; idxV <= maxV; ++idxV)
	{
	if (*ptrV != *ptrC++)
	  {
	  temp = 0;
	  break;
	  }
	ptrV += incV;
	}
      if (temp)
	{ // color match add a new seed to end of list
	if (heap)
	  {
	  pixel = heap;
	  heap = heap->Next;
	  }
	else
	  {
	  pixel = new vtkImagePainter2DPixel;
	  }
	pixel->X = first->X;
	pixel->Y = first->Y+1;
	pixel->Pointer = (void *)(ptr + inc1);
	pixel->Next = NULL;
	last->Next = pixel;
	last = pixel;
	// change the seeds color
	ptrV = (T *)(last->Pointer);
	ptrC = drawColor;
	for (idxV = minV; idxV <= maxV; ++idxV)
	  {
	  *ptrV = *ptrC++;
	  ptrV += incV;
	  }
	}
      }

    // remove the first from the list.
    pixel = first;
    first = first->Next;
    pixel->Next = heap;
    heap = pixel;
    }
  
  // free the heap
  while (heap)
    {
    pixel = heap;
    heap = heap->Next;
    delete pixel;
    }
}

  
  
  
  
  
  
  
//----------------------------------------------------------------------------
// Description:
// Fill a colored area with another color. (like connectivity)
// All pixels connected to pixel (x, y) get replaced by draw color.
void vtkImagePainter2D::FillPixel(int x, int y)
{
  void *ptr;
  
  ptr = this->ImageRegion->GetScalarPointer(x, y);

  switch (this->ImageRegion->GetScalarType())
    {
    case VTK_FLOAT:
      vtkImagePainter2DFill(this->ImageRegion, this->DrawColor, 
			(float *)(ptr), x, y);
      break;
    case VTK_INT:
      vtkImagePainter2DFill(this->ImageRegion, this->DrawColor, 
			(int *)(ptr), x, y);
      break;
    case VTK_SHORT:
      vtkImagePainter2DFill(this->ImageRegion, this->DrawColor, 
			(short *)(ptr), x, y);
      break;
    case VTK_UNSIGNED_SHORT:
      vtkImagePainter2DFill(this->ImageRegion, this->DrawColor, 
			(unsigned short *)(ptr), x, y);
      break;
    case VTK_UNSIGNED_CHAR:
      vtkImagePainter2DFill(this->ImageRegion, this->DrawColor, 
			(unsigned char *)(ptr), x, y);
      break;
    default:
      vtkErrorMacro(<< "Fill: Cannot handle ScalarType.");
    }   
}

