/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCanvasSource2D.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

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
#include <math.h>
#include "vtkImageCanvasSource2D.h"
#include "vtkObjectFactory.h"



//------------------------------------------------------------------------------
vtkImageCanvasSource2D* vtkImageCanvasSource2D::New()
{
  // First try to create the object from the vtkObjectFactory
  vtkObject* ret = vtkObjectFactory::CreateInstance("vtkImageCanvasSource2D");
  if(ret)
    {
    return (vtkImageCanvasSource2D*)ret;
    }
  // If the factory was unable to create the object, then create it here.
  return new vtkImageCanvasSource2D;
}




//----------------------------------------------------------------------------
// Construct an instance of vtkImageCanvasSource2D with no data.
vtkImageCanvasSource2D::vtkImageCanvasSource2D()
{
  int idx;
  
  for (idx = 0; idx < 4; ++idx)
    {
    this->DrawColor[idx] = 0.0;
    }
  this->SetNumberOfScalarComponents(1);
  this->ImageData = this;
  this->DefaultZ = 0;
}


//----------------------------------------------------------------------------
// Destructor: Deleting a vtkImageCanvasSource2D automatically 
// deletes the associated
// vtkImageData.  However, since the data is reference counted, it may not 
// actually be deleted.
vtkImageCanvasSource2D::~vtkImageCanvasSource2D()
{
  if (this->ImageData != NULL && this->ImageData != this)
    {
    this->ImageData->UnRegister(this);
    }
  
  this->ReleaseData();
}


//----------------------------------------------------------------------------
void vtkImageCanvasSource2D::PrintSelf(ostream& os, vtkIndent indent)
{
  int idx;
  
  vtkStructuredPoints::PrintSelf(os,indent);
  os << indent << "ImageData: (" << this->ImageData << ")\n";
  os << indent << "DefaultZ: " << this->DefaultZ << endl;
  os << indent << "DrawColor: (" << this->DrawColor[0];
  for (idx = 1; idx < 4; ++idx)
    {
    os << ", " << this->DrawColor[idx];
    }
  os << ")\n";
}


//----------------------------------------------------------------------------
// Normal reference counting, but do not reference count "this".
void vtkImageCanvasSource2D::SetImageData(vtkImageData *image)
{
  if (this->ImageData == image)
    {
    return;
    }
  
  if (this->ImageData != NULL && this->ImageData != this)
    {
    this->ImageData->UnRegister(this);
    }
  
  this->ImageData = image;
  this->Modified();
  
  if (this->ImageData != NULL && this->ImageData != this)
    {
    this->ImageData->Register(this);
    }
  
}


//----------------------------------------------------------------------------
// Draw a data.  Only implentented for 2D extents.
template <class T>
static void vtkImageCanvasSource2DFillBox(vtkImageData *image, 
					  float *drawColor, T *ptr, 
					  int min0, int max0, int min1, int max1)
{
  T *ptr0, *ptr1, *ptrV;
  int idx0, idx1, idxV;
  int inc0, inc1, inc2;
  int maxV;
  float *pf;
  
  image->GetIncrements(inc0, inc1, inc2);
  maxV = image->GetNumberOfScalarComponents() - 1;

  ptr1 = ptr;
  for (idx1 = min1; idx1 <= max1; ++idx1)
    {
    ptr0 = ptr1;
    for (idx0 = min0; idx0 <= max0; ++idx0)
      {
      ptrV = ptr0;
      pf = drawColor;
      
      // Assign color to pixel.
      for (idxV = 0; idxV <= maxV; ++idxV)
        {
        *ptrV = (T)(*pf++);
        ptrV++;
        }
      
      ptr0 += inc0;
      }
    ptr1 += inc1;
    }
}

//----------------------------------------------------------------------------
// Draw a data.  Only implentented for 2D extents.
void vtkImageCanvasSource2D::FillBox(int min0, int max0, int min1, int max1)
{
  int *extent;
  void *ptr;
  int z = this->DefaultZ;
  
  // Clip the data to keep in in bounds
  extent = this->ImageData->GetExtent();
  min0 = (min0 < extent[0]) ? extent[0] : min0;
  max0 = (max0 < extent[0]) ? extent[0] : max0;
  min0 = (min0 > extent[1]) ? extent[1] : min0;
  max0 = (max0 > extent[1]) ? extent[1] : max0;
  min1 = (min1 < extent[2]) ? extent[2] : min1;
  max1 = (max1 < extent[2]) ? extent[2] : max1;
  min1 = (min1 > extent[3]) ? extent[3] : min1;
  max1 = (max1 > extent[3]) ? extent[3] : max1;   
  z = (z < extent[4]) ? extent[4] : z;
  z = (z > extent[5]) ? extent[5] : z;

  ptr = this->ImageData->GetScalarPointer(min0, min1, z);
  switch (this->ImageData->GetScalarType())
    {
    vtkTemplateMacro7(vtkImageCanvasSource2DFillBox, this->ImageData, 
                      this->DrawColor, (VTK_TT *)(ptr), 
                      min0,max0, min1,max1);
    default:
      vtkErrorMacro(<< "FillBox: Cannot handle ScalarType.");
    }   
}



//----------------------------------------------------------------------------
// Fill a tube (thick line for initial 2D implementation.
template <class T>
static void vtkImageCanvasSource2DFillTube(vtkImageData *image, 
				  float *drawColor, T *ptr, 
				  int a0, int a1, int b0, int b1, float radius)
{
  T *ptr0, *ptr1, *ptrV;
  int idx0, idx1, idxV;
  int inc0, inc1, inc2;
  int min0, max0, min1, max1, min2, max2, maxV;
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

  image->GetExtent(min0, max0, min1, max1, min2, max2);
  maxV = image->GetNumberOfScalarComponents() - 1;
  // Loop trough whole extent.
  image->GetIncrements(inc0, inc1, inc2);
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
          for (idxV = 0; idxV <= maxV; ++idxV)
            {
            *ptrV = (T)(*pf++);
            ptrV++;
            }
          }
        }
      
      ptr0 += inc0;
      }
    ptr1 += inc1;
    }
}

//----------------------------------------------------------------------------
// Fill a tube (thick line for initial 2D implementation).
void vtkImageCanvasSource2D::FillTube(int a0, int a1, int b0, int b1, float radius)
{
  void *ptr;
  int z = this->DefaultZ;
  int *extent = this->ImageData->GetExtent();
  
  z = (z < extent[4]) ? extent[4] : z;
  z = (z > extent[5]) ? extent[5] : z;

  ptr = this->ImageData->GetScalarPointer(extent[0], extent[2], z);
  switch (this->ImageData->GetScalarType())
    {
    vtkTemplateMacro8(vtkImageCanvasSource2DFillTube, this->ImageData, 
                     this->DrawColor, (VTK_TT *)(ptr), a0,a1, b0,b1, radius);
    default:
      vtkErrorMacro(<< "FillTube: Cannot handle ScalarType.");
    }   
}





//----------------------------------------------------------------------------
// Fill a triangle (rasterize)
template <class T>
static void vtkImageCanvasSource2DFillTriangle(vtkImageData *image, 
				      float *drawColor, T *ptr, int a0, int a1,
				      int b0, int b1, int c0, int c1, int z)
{
  int temp;
  float longT, shortT;  // end points of intersection of trainge and row.
  float longStep, shortStep;
  int left, right;
  int idx0, idx1, idxV;
  int min0, max0, min1, max1, min2, max2;
  int  maxV;
  float *pf;
  
  ptr = ptr;
  maxV = image->GetNumberOfScalarComponents() - 1;
  
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
  
  image->GetExtent(min0, max0, min1, max1, min2, max2);
  z = (z < min2) ? min2 : z;
  z = (z > max2) ? max2 : z;
  
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
        ptr = (T *)(image->GetScalarPointer(idx0, idx1, z));
        if (ptr)
          {
          pf = drawColor;
          // Assign color to pixel.
          for (idxV = 0; idxV <= maxV; ++idxV)
            {
            *ptr = (T)(*pf++);
            ptr++;
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
      if (idx0 >= min0 && idx0 <= max0 && idx1 >= min1 && idx1 <= max1)
        {
        ptr = (T *)(image->GetScalarPointer(idx0, idx1, z));
        if (ptr)
          {
          pf = drawColor;
          // Assign color to pixel.
          for (idxV = 0; idxV <= maxV; ++idxV)
            {
            *ptr = (T)(*pf++);
            ptr++;
            }
          }
        }
      }

    longT += longStep;
    shortT += shortStep;
    }
}

//----------------------------------------------------------------------------
// Fill a tube (thick line for initial 2D implementation).
void vtkImageCanvasSource2D::FillTriangle(int a0,int a1, int b0,int b1, int c0,int c1)
{
  void *ptr;
  
  ptr = this->ImageData->GetScalarPointer();
  switch (this->ImageData->GetScalarType())
    {
    vtkTemplateMacro10(vtkImageCanvasSource2DFillTriangle, this->ImageData, 
                       this->DrawColor, (VTK_TT *)(ptr), 
                       a0,a1, b0,b1, c0,c1, this->DefaultZ);
    default:
      vtkErrorMacro(<< "FillTriangle: Cannot handle ScalarType.");
    }   
}





//----------------------------------------------------------------------------
// Draw a point.  Only implentented for 2D images.
template <class T>
static void vtkImageCanvasSource2DDrawPoint(vtkImageData *image, 
				   float *drawColor, T *ptr, 
				   int p0, int p1, int z)
{
  int min0, max0, min1, max1, min2, max2, maxV;
  int idxV;
  float *pf;
  
  image->GetExtent(min0, max0, min1, max1, min2, max2);
  z = (z < min2) ? min2 : z;
  z = (z > max2) ? max2 : z;
  maxV = image->GetNumberOfScalarComponents() - 1;

  if (p0 >= min0 && p0 <= max0 && p1 >= min1 && p1 <= max1)
    {
    ptr = (T *)(image->GetScalarPointer(p0, p1, z));

    pf = drawColor;
    // Assign color to pixel.
    for (idxV = 0; idxV <= maxV; ++idxV)
      {
      *ptr = (T)(*pf++);
      ptr++;
      }
    
    }
}



//----------------------------------------------------------------------------
// Draw a circle
void vtkImageCanvasSource2D::DrawPoint(int p0, int p1)
{
  void *ptr = NULL;
  
  vtkDebugMacro(<< "Drawing a point: (" << p0 << ", " << p1 << ")");
  
  switch (this->ImageData->GetScalarType())
    {
    vtkTemplateMacro6(vtkImageCanvasSource2DDrawPoint, this->ImageData, 
                      this->DrawColor, (VTK_TT *)(ptr), p0, p1, 
                      this->DefaultZ);
    default:
      vtkErrorMacro(<< "DrawPoint: Cannot handle ScalarType.");
    }   
}



//----------------------------------------------------------------------------
// Draw a circle.  Only implentented for 2D images.
template <class T>
static void vtkImageCanvasSource2DDrawCircle(vtkImageData *image, 
				    float *drawColor, T *ptr, 
				    int c0, int c1, float radius, int z)
{
  int min0, max0, min1, max1, min2, max2, maxV;
  int idxV;
  float *pf;
  int numberOfSteps;
  double thetaCos, thetaSin;
  double x, y, temp;
  int p0, p1;
  int idx;

  radius += 0.1;
  image->GetExtent(min0, max0, min1, max1, min2, max2);
  z = (z < min2) ? min2 : z;
  z = (z > max2) ? max2 : z;
  maxV = image->GetNumberOfScalarComponents() - 1;

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
      ptr = (T *)(image->GetScalarPointer(p0, p1, z));

      pf = drawColor;
      // Assign color to pixel.
      for (idxV = 0; idxV <= maxV; ++idxV)
        {
        *ptr = (T)(*pf++);
        ptr++;
        }
      
      }
    
    // rotate the point
    temp = thetaCos * x + thetaSin * y;
    y = thetaCos * y - thetaSin * x;
    x = temp;
    }
}


//----------------------------------------------------------------------------
// Draw a circle
void vtkImageCanvasSource2D::DrawCircle(int c0, int c1, float radius)
{
  void *ptr = NULL;
  
  vtkDebugMacro(<< "Drawing a circle: center = (" << c0 << ", " << c1 
                << "), radius = " << radius);
  
  switch (this->ImageData->GetScalarType())
    {
    vtkTemplateMacro7(vtkImageCanvasSource2DDrawCircle, this->ImageData, 
                      this->DrawColor, 
                      (VTK_TT *)(ptr), c0, c1, radius, this->DefaultZ);
    default:
      vtkErrorMacro(<< "DrawCircle: Cannot handle ScalarType.");
    }   
}



//----------------------------------------------------------------------------
// Draw a line.  Only implentented for 2D images.
// First point is already shifted to origin.
template <class T>
static void vtkImageCanvasSource2DDrawSegment(vtkImageData *image, 
				     float *drawColor, T *ptr, 
				     int p0, int p1)
{
  float f0, f1;
  float s0, s1;
  int numberOfSteps;
  int  maxV;
  int idx, idxV;
  int inc0, inc1, inc2;
  float *pf;
  T *ptrV;
  
  
  image->GetIncrements(inc0, inc1, inc2);
  maxV = image->GetNumberOfScalarComponents() - 1;

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
  for (idxV = 0; idxV <= maxV; ++idxV)
    {
    *ptrV = (T)(*pf++);
    ptrV++;
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
    for (idxV = 0; idxV <= maxV; ++idxV)
      {
      *ptrV = (T)(*pf++);
      ptrV++;
      }
    

    }
}


//----------------------------------------------------------------------------
// Draw a Segment from point a to point b.
void vtkImageCanvasSource2D::DrawSegment(int a0, int a1, int b0, int b1)
{
  int *extent;
  void *ptr;
  int z = this->DefaultZ;
  
  vtkDebugMacro(<< "Drawing a segment: " << a0 << ", " << a1 << " to "
                << b0 << ", " << b1);
  
  // check to make sure line segment is in bounds.
  extent = this->ImageData->GetExtent();
  z = (z < extent[4]) ? extent[4] : z;
  z = (z > extent[5]) ? extent[5] : z;
  if (a0 < extent[0] || a0 > extent[1] || b0 < extent[0] || b0 > extent[1] ||
      a1 < extent[2] || a1 > extent[3] || b1 < extent[2] || b1 > extent[3])
    {
    if ( ! this->ClipSegment(a0,a1,b0,b1))
      {
      // non of the segment is in the data.
      return;
      }
    }

  ptr = this->ImageData->GetScalarPointer(b0, b1, z);
  a0 -= b0;
  a1 -= b1;
  switch (this->ImageData->GetScalarType())
    {
    vtkTemplateMacro5(vtkImageCanvasSource2DDrawSegment, this->ImageData, 
                      this->DrawColor, (VTK_TT *)(ptr), a0, a1);
    default:
      vtkErrorMacro(<< "DrawSegment: Cannot handle ScalarType.");
    }   
}



//----------------------------------------------------------------------------
// Clips a line segment so it will be in bounds.
// If the entire segment is out of bounds, the method returns 0.
int vtkImageCanvasSource2D::ClipSegment(int &a0, int &a1, int &b0, int &b1)
{
  int min0, max0, min1, max1, min2, max2;
  float fract;

  
  this->ImageData->GetExtent(min0, max0, min1, max1, min2, max2);
  
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
static void vtkImageCanvasSource2DDrawSegment3D(vtkImageData *image, 
				       float *drawColor, 
				       T *ptr, int p0, int p1, int p2)
{
  float f0, f1, f2;
  float s0, s1, s2;
  int numberOfSteps;
  int idx, idxV,  maxV;
  int inc0, inc1, inc2;
  float *pf;
  T *ptrV;
  
  
  image->GetIncrements(inc0, inc1, inc2);
  maxV = image->GetNumberOfScalarComponents() - 1;

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
  for (idxV = 0; idxV <= maxV; ++idxV)
    {
    *ptrV = (T)(*pf++);
    ptrV++;
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
    for (idxV = 0; idxV <= maxV; ++idxV)
      {
      *ptrV = (T)(*pf++);
      ptrV++;
      }
    }
}


//----------------------------------------------------------------------------
// Draw a Segment from point a to point b.
// No clipping or bounds checking.
void vtkImageCanvasSource2D::DrawSegment3D(float *a, float *b)
{
  void *ptr;
  int a0, a1, a2;
  
  ptr = this->ImageData->GetScalarPointer((int)(b[0] + 0.5), 
					    (int)(b[1] + 0.5), 
					    (int)(b[2] + 0.5));
  a0 = (int)(a[0] - b[0] + 0.5);
  a1 = (int)(a[1] - b[1] + 0.5);
  a2 = (int)(a[2] - b[2] + 0.5);
  switch (this->ImageData->GetScalarType())
    {
    vtkTemplateMacro6(vtkImageCanvasSource2DDrawSegment3D, this->ImageData, 
                      this->DrawColor, (VTK_TT *)(ptr), a0, a1, a2);
    default:
      vtkErrorMacro(<< "DrawSegment3D: Cannot handle ScalarType.");
    }   
}



//----------------------------------------------------------------------------
template <class T>
static void vtkImageCanvasSource2DFill(vtkImageData *image, float *color, 
			      T *ptr, int x, int y)
{
  vtkImageCanvasSource2DPixel *pixel;
  vtkImageCanvasSource2DPixel *first, *last;
  vtkImageCanvasSource2DPixel *heap = NULL;
  int min0, max0, min1, max1, min2, max2, maxV;
  int idxV;
  int inc0, inc1, inc2;
  T fillColor[10];
  T drawColor[10];
  T *ptrV, *ptrC;
  int temp;
  
  image->GetExtent(min0, max0, min1, max1, min2, max2);
  maxV = image->GetNumberOfScalarComponents() - 1;
  image->GetIncrements(inc0, inc1, inc2);
  
  // Copy the fill color and make sure it differs from drawColor.
  ptrV = ptr;
  temp = 1;
  for (idxV = 0; idxV <= maxV; ++idxV)
    {
    // Save the fill color
    fillColor[idxV] = *ptrV;
    drawColor[idxV] = (T)(color[idxV]);
    if (*ptrV != drawColor[idxV])
      {
      temp = 0;
      }
    ptrV++;
    }
  if (temp)
    { // fill the same as draw
    cerr << "Fill: Cannot handle draw color same as fill color\n";
    return;
    }
  
  // Create the seed
  pixel = vtkImageCanvasSource2DPixel::New();
  pixel->X = x;
  pixel->Y = y;
  pixel->Pointer = (void *)(ptr);
  pixel->Next = NULL;
  first = last = pixel;
  // change the seeds color
  ptrV = (T *)(last->Pointer);
  ptrC = drawColor;
  for (idxV = 0; idxV <= maxV; ++idxV)
    {
    *ptrV = *ptrC++;
    ptrV++;
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
      for (idxV = 0; idxV <= maxV; ++idxV)
	{
	if (*ptrV != *ptrC++)
	  {
	  temp = 0;
	  break;
	  }
	ptrV++;
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
	  pixel = vtkImageCanvasSource2DPixel::New();
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
	for (idxV = 0; idxV <= maxV; ++idxV)
	  {
	  *ptrV = *ptrC++;
	  ptrV++;
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
      for (idxV = 0; idxV <= maxV; ++idxV)
	{
	if (*ptrV != *ptrC++)
	  {
	  temp = 0;
	  break;
	  }
	ptrV++;
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
	  pixel = vtkImageCanvasSource2DPixel::New();
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
	for (idxV = 0; idxV <= maxV; ++idxV)
	  {
	  *ptrV = *ptrC++;
	  ptrV++;
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
      for (idxV = 0; idxV <= maxV; ++idxV)
	{
	if (*ptrV != *ptrC++)
	  {
	  temp = 0;
	  break;
	  }
	ptrV++;
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
	  pixel = vtkImageCanvasSource2DPixel::New();
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
	for (idxV = 0; idxV <= maxV; ++idxV)
	  {
	  *ptrV = *ptrC++;
	  ptrV++;
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
      for (idxV = 0; idxV <= maxV; ++idxV)
	{
	if (*ptrV != *ptrC++)
	  {
	  temp = 0;
	  break;
	  }
	ptrV++;
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
	  pixel = vtkImageCanvasSource2DPixel::New();
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
	for (idxV = 0; idxV <= maxV; ++idxV)
	  {
	  *ptrV = *ptrC++;
	  ptrV++;
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
// Fill a colored area with another color. (like connectivity)
// All pixels connected to pixel (x, y) get replaced by draw color.
void vtkImageCanvasSource2D::FillPixel(int x, int y)
{
  void *ptr;
  int *ext = this->ImageData->GetExtent();
  int z = this->DefaultZ;

  z = (z < ext[4]) ? ext[4] : z;
  z = (z > ext[5]) ? ext[5] : z;
  
  ptr = this->ImageData->GetScalarPointer(x, y, z);

  switch (this->ImageData->GetScalarType())
    {
    vtkTemplateMacro5(vtkImageCanvasSource2DFill, this->ImageData, 
                      this->DrawColor, (VTK_TT *)(ptr), x, y);
    default:
      vtkErrorMacro(<< "Fill: Cannot handle ScalarType.");
    }   
}



//----------------------------------------------------------------------------
void vtkImageCanvasSource2D::SetExtent(int *extent)
{
  this->vtkStructuredPoints::SetExtent(extent);
  this->SetWholeExtent(extent);  
}

//----------------------------------------------------------------------------
void vtkImageCanvasSource2D::SetExtent(int x1, int x2, int y1, int y2, 
					int z1, int z2)
{
  this->vtkStructuredPoints::SetExtent(x1, x2, y1, y2, z1, z2);
  this->SetWholeExtent(x1, x2, y1, y2, z1, z2);
}






