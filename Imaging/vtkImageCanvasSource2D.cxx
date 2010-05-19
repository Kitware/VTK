/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageCanvasSource2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageCanvasSource2D.h"

#include "vtkImageCast.h"
#include "vtkImageClip.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkImageData.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <math.h>

//
// Special classes for manipulating data
//
// For the fill functionality (use connector ??)
class vtkImageCanvasSource2DPixel { //;prevent man page generation
public:
  static vtkImageCanvasSource2DPixel *New()
    { return new vtkImageCanvasSource2DPixel ;}
  int X;
  int Y;
  void *Pointer;
  vtkImageCanvasSource2DPixel *Next;
};



vtkStandardNewMacro(vtkImageCanvasSource2D);

//----------------------------------------------------------------------------
// Construct an instance of vtkImageCanvasSource2D with no data.
vtkImageCanvasSource2D::vtkImageCanvasSource2D()
{
  this->SetNumberOfInputPorts(0);

  this->ImageData = vtkImageData::New();
  this->ImageData->SetScalarType(VTK_DOUBLE);

  this->WholeExtent[0] = 0;
  this->WholeExtent[1] = 0;
  this->WholeExtent[2] = 0;
  this->WholeExtent[3] = 0;
  this->WholeExtent[4] = 0;
  this->WholeExtent[5] = 0;

  this->DrawColor[0] = this->DrawColor[1] =
    this->DrawColor[2] = this->DrawColor[3] = 0.0;

  this->DefaultZ = 0;

  this->Ratio[0] = this->Ratio[1] = this->Ratio[2] = 1.0;
}


//----------------------------------------------------------------------------
// Destructor: Deleting a vtkImageCanvasSource2D automatically
// deletes the associated
// vtkImageData.  However, since the data is reference counted, it may not
// actually be deleted.
vtkImageCanvasSource2D::~vtkImageCanvasSource2D()
{
  this->ImageData->Delete();
}


//----------------------------------------------------------------------------
void vtkImageCanvasSource2D::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  int idx;

  os << indent << "ImageData: (" << this->ImageData << ")\n";
  os << indent << "DefaultZ: " << this->DefaultZ << endl;
  os << indent << "DrawColor: (" << this->DrawColor[0];
  for (idx = 1; idx < 4; ++idx)
    {
    os << ", " << this->DrawColor[idx];
    }
  os << ")\n";
  os << indent << "Ratio: ("
     << this->Ratio[0] << ", " << this->Ratio[1] << ", " << this->Ratio[2]
     << ")\n";
}


#define vtkMAX(x, y) (((x)>(y))?(x):(y))
#define vtkMIN(x, y) (((x)<(y))?(x):(y))

//----------------------------------------------------------------------------
// Draw a data.  Only implentented for 2D extents.
template <class T>
void vtkImageCanvasSource2DDrawImage(vtkImageData *image, vtkImageData *simage,
                                     T *ptr, T *sptr,
                                     int min0, int max0, int min1, int max1)
{
  T *ptr0, *ptr1, *ptrV;
  T *sptr0, *sptr1, *sptrV;
  int idx0, idx1, idxV;
  vtkIdType inc0, inc1, inc2;
  vtkIdType sinc0, sinc1, sinc2;
  int maxV, smaxV;
  int sinc;

  image->GetIncrements(inc0, inc1, inc2);
  simage->GetIncrements(sinc0, sinc1, sinc2);

  maxV = image->GetNumberOfScalarComponents() - 1;
  smaxV = simage->GetNumberOfScalarComponents() - 1;

  ptr1 = ptr;
  sptr1 = sptr;
  for (idx1 = min1; idx1 <= max1; ++idx1)
    {
    ptr0 = ptr1;
    sptr0 = sptr1;
    for (idx0 = min0; idx0 <= max0; ++idx0)
      {
      ptrV = ptr0;
      sptrV = sptr0;

      sinc = 0;

      // Assign color to pixel.
      for (idxV = 0; idxV <= maxV; ++idxV)
        {
        *ptrV = *(sptrV + sinc);
        ptrV++;
        if ( sinc < smaxV )
          {
          sinc++;
          }
        }
      //sptrV += smaxV;

      ptr0 += inc0;
      sptr0 += sinc0;
      }
    ptr1 += inc1;
    sptr1 += sinc1;
    }
}

//----------------------------------------------------------------------------
void vtkImageCanvasSource2D::DrawImage(int x0, int y0,
                                       vtkImageData* image, int sx, int sy,
                                       int width, int height)
{
  if ( !image )
    {
    return;
    }

  vtkImageClip* clip = vtkImageClip::New();
  clip->SetInput(image);

  int *extent;
  int ext[6];
  //  int z = this->DefaultZ;
  image->GetWholeExtent(ext);
  if ( sx < 0 )
    {
    sx = ext[0];
    }
  if ( sy < 0 )
    {
    sy = ext[2];
    }
  if ( width < 0 )
    {
    width = ext[1] - ext[0] + 1;
    }
  else
    {
    width = vtkMIN(width, ext[1] - ext[0] + 1);
    }
  if ( height < 0 )
    {
    height = ext[3] - ext[2] + 1;
    }
  else
    {
    height = vtkMIN(height, ext[3] - ext[2] + 1);
    }
  ext[0] = vtkMAX(sx, ext[0]);
  ext[1] = vtkMAX(sx+width-1, ext[1]);
  ext[2] = vtkMAX(sy, ext[2]);
  ext[3] = vtkMAX(sy+height-1, ext[3]);
  clip->SetOutputWholeExtent(ext);

  vtkImageCast* ic = vtkImageCast::New();
  ic->SetInputConnection(clip->GetOutputPort());
  ic->SetOutputScalarType(this->ImageData->GetScalarType());
  ic->Update();
  int min0, max0, min1, max1;
  min0 = x0;
  min1 = y0;

  max0 = x0 + width -1;
  max1 = y0 + height -1;

  // Pre-multiply coords if needed
  if (this->Ratio[0] != 1.0)
    {
    min0 = int(double(min0) * this->Ratio[0]);
    max0 = int(double(max0) * this->Ratio[0]);
    }
  if (this->Ratio[1] != 1.0)
    {
    min1 = int(double(min1) * this->Ratio[1]);
    max1 = int(double(max1) * this->Ratio[1]);
    }
//  if (this->Ratio[2] != 1.0)
//    {
//    z = int(double(z) * this->Ratio[2]);
//    }
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
  //z = (z < extent[4]) ? extent[4] : z;
  //z = (z > extent[5]) ? extent[5] : z;
  void *ptr;
  void *sptr;
  ptr = this->ImageData->GetScalarPointer(min0, min1, 0);
  sptr = ic->GetOutput()->GetScalarPointer(ext[0], ext[2], 0);
  switch (this->ImageData->GetScalarType())
    {
    vtkTemplateMacro(vtkImageCanvasSource2DDrawImage(this->ImageData,
                                                     ic->GetOutput(),
                                                     static_cast<VTK_TT *>(ptr),
                                                     static_cast<VTK_TT *>(sptr),
                                                     min0,max0, min1,max1));
    default:
      vtkErrorMacro(<< "FillBox: Cannot handle ScalarType.");
    }
  ic->Delete();
  clip->Delete();
  this->Modified();
}

//----------------------------------------------------------------------------
// Draw a data.  Only implentented for 2D extents.
template <class T>
void vtkImageCanvasSource2DFillBox(vtkImageData *image,
                                   double *drawColor, T *ptr,
                                   int min0, int max0, int min1, int max1)
{
  T *ptr0, *ptr1, *ptrV;
  int idx0, idx1, idxV;
  vtkIdType inc0, inc1, inc2;
  int maxV;
  double *pf;

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
        *ptrV = static_cast<T>(*pf++);
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

  // Pre-multiply coords if needed
  if (this->Ratio[0] != 1.0)
    {
    min0 = int(double(min0) * this->Ratio[0]);
    max0 = int(double(max0) * this->Ratio[0]);
    }
  if (this->Ratio[1] != 1.0)
    {
    min1 = int(double(min1) * this->Ratio[1]);
    max1 = int(double(max1) * this->Ratio[1]);
    }
  if (this->Ratio[2] != 1.0)
    {
    z = int(double(z) * this->Ratio[2]);
    }

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
    vtkTemplateMacro(vtkImageCanvasSource2DFillBox(this->ImageData,
                                                   this->DrawColor,
                                                   static_cast<VTK_TT *>(ptr),
                                                   min0,max0, min1,max1));
    default:
      vtkErrorMacro(<< "FillBox: Cannot handle ScalarType.");
    }
  this->Modified();
}



//----------------------------------------------------------------------------
// Fill a tube (thick line for initial 2D implementation.
template <class T>
void vtkImageCanvasSource2DFillTube(vtkImageData *image,
                                    double *drawColor, T *ptr,
                                    int a0, int a1, int b0, int b1,
                                    double radius)
{
  T *ptr0, *ptr1, *ptrV;
  int idx0, idx1, idxV;
  vtkIdType inc0, inc1, inc2;
  int min0, max0, min1, max1, min2, max2, maxV;
  double *pf;
  int n0, n1;
  int ak, bk, k;
  double fract;
  double v0, v1;

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
        fract = static_cast<double>(k - bk) / static_cast<double>(ak - bk);
        v0 = b0 + fract * static_cast<double>(a0 - b0);
        v1 = b1 + fract * static_cast<double>(a1 - b1);
        // Compute distance to tube
        v0 -= static_cast<double>(idx0);
        v1 -= static_cast<double>(idx1);
        if (radius >= sqrt(v0*v0 + v1*v1))
          {
          ptrV = ptr0;
          pf = drawColor;
          // Assign color to pixel.
          for (idxV = 0; idxV <= maxV; ++idxV)
            {
            *ptrV = static_cast<T>(*pf++);
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
void vtkImageCanvasSource2D::FillTube(int a0, int a1,
                                      int b0, int b1, double radius)
{
  void *ptr;
  int z = this->DefaultZ;
  int *extent = this->ImageData->GetExtent();

  // Pre-multiply coords if needed
  if (this->Ratio[0] != 1.0)
    {
    a0 = int(double(a0) * this->Ratio[0]);
    b0 = int(double(b0) * this->Ratio[0]);
    radius = int(double(radius) * this->Ratio[0]);
    }
  if (this->Ratio[1] != 1.0)
    {
    a1 = int(double(a1) * this->Ratio[1]);
    b1 = int(double(b1) * this->Ratio[1]);
    }
  if (this->Ratio[2] != 1.0)
    {
    z = int(double(z) * this->Ratio[2]);
    }

  z = (z < extent[4]) ? extent[4] : z;
  z = (z > extent[5]) ? extent[5] : z;

  ptr = this->ImageData->GetScalarPointer(extent[0], extent[2], z);
  switch (this->ImageData->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageCanvasSource2DFillTube(this->ImageData,
                                     this->DrawColor,
                                     static_cast<VTK_TT *>(ptr),
                                     a0,a1, b0,b1,
                                     radius));
    default:
      vtkErrorMacro(<< "FillTube: Cannot handle ScalarType.");
    }
  this->Modified();
}





//----------------------------------------------------------------------------
// Fill a triangle (rasterize)
template <class T>
void vtkImageCanvasSource2DFillTriangle(vtkImageData *image,
                                        double *drawColor, T *ptr,
                                        int a0, int a1, int b0, int b1,
                                        int c0, int c1, int z)
{
  int temp;
  double longT, shortT;  // end points of intersection of trainge and row.
  double longStep, shortStep;
  int left, right;
  int idx0, idx1, idxV;
  int min0, max0, min1, max1, min2, max2;
  int  maxV;
  double *pf;

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
  longStep = static_cast<double>(c0 - a0) / static_cast<double>(c1 - a1 + 1);
  longT = static_cast<double>(a0) + (0.5 * longStep);
  shortStep = static_cast<double>(b0 - a0) / static_cast<double>(b1 - a1 + 1);
  shortT = static_cast<double>(a0) + (0.5 * shortStep);
  for (idx1 = a1; idx1 < b1; ++idx1)
    {
    // Fill row long to short (y = idx1)
    left = static_cast<int>(shortT + 0.5);
    right = static_cast<int>(longT + 0.5);
    if (left > right)
      {
      temp = left;   left = right;   right = temp;
      }
    for (idx0 = left; idx0 <= right; ++idx0)
      {
      if (idx0 >= min0 && idx0 <= max0 && idx1 >= min1 && idx1 <= max1)
        {
        ptr = static_cast<T *>(image->GetScalarPointer(idx0, idx1, z));
        if (ptr)
          {
          pf = drawColor;
          // Assign color to pixel.
          for (idxV = 0; idxV <= maxV; ++idxV)
            {
            *ptr = static_cast<T>(*pf++);
            ptr++;
            }
          }
        }
      }

    longT += longStep;
    shortT += shortStep;
    }

  // fill the second half of the triangle
  shortStep = static_cast<double>(c0 - b0) / static_cast<double>(c1 - b1 + 1);
  shortT = static_cast<double>(b0) + (0.5 * shortStep);
  for (idx1 = b1; idx1 < c1; ++idx1)
    {
    // Fill row long to short (y = idx1)
    left = static_cast<int>(shortT + 0.5);
    right = static_cast<int>(longT + 0.5);
    if (left > right)
      {
      temp = left;   left = right;   right = temp;
      }
    for (idx0 = left; idx0 <= right; ++idx0)
      {
      if (idx0 >= min0 && idx0 <= max0 && idx1 >= min1 && idx1 <= max1)
        {
        ptr = static_cast<T *>(image->GetScalarPointer(idx0, idx1, z));
        if (ptr)
          {
          pf = drawColor;
          // Assign color to pixel.
          for (idxV = 0; idxV <= maxV; ++idxV)
            {
            *ptr = static_cast<T>(*pf++);
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
void vtkImageCanvasSource2D::FillTriangle(int a0,int a1, int b0,int b1,
                                          int c0,int c1)
{
  void *ptr;
  int z = this->DefaultZ;

  // Pre-multiply coords if needed
  if (this->Ratio[0] != 1.0)
    {
    a0 = static_cast<int>(static_cast<double>(a0) * this->Ratio[0]);
    b0 = static_cast<int>(static_cast<double>(b0) * this->Ratio[0]);
    c0 = static_cast<int>(static_cast<double>(c0) * this->Ratio[0]);
    }
  if (this->Ratio[1] != 1.0)
    {
    a1 = static_cast<int>(static_cast<double>(a1) * this->Ratio[1]);
    b1 = static_cast<int>(static_cast<double>(b1) * this->Ratio[1]);
    c1 = static_cast<int>(static_cast<double>(c1) * this->Ratio[1]);
    }
  if (this->Ratio[2] != 1.0)
    {
    z = static_cast<int>(static_cast<double>(z) * this->Ratio[2]);
    }

  ptr = this->ImageData->GetScalarPointer();
  switch (this->ImageData->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageCanvasSource2DFillTriangle(this->ImageData,
                                         this->DrawColor,
                                         static_cast<VTK_TT *>(ptr),
                                         a0,a1, b0,b1, c0,c1, z));
    default:
      vtkErrorMacro(<< "FillTriangle: Cannot handle ScalarType.");
    }
  this->Modified();
}





//----------------------------------------------------------------------------
// Draw a point.  Only implentented for 2D images.
template <class T>
void vtkImageCanvasSource2DDrawPoint(vtkImageData *image,
                                   double *drawColor, T *ptr,
                                   int p0, int p1, int z)
{
  int min0, max0, min1, max1, min2, max2, maxV;
  int idxV;
  double *pf;

  image->GetExtent(min0, max0, min1, max1, min2, max2);
  z = (z < min2) ? min2 : z;
  z = (z > max2) ? max2 : z;
  maxV = image->GetNumberOfScalarComponents() - 1;

  if (p0 >= min0 && p0 <= max0 && p1 >= min1 && p1 <= max1)
    {
    ptr = static_cast<T *>(image->GetScalarPointer(p0, p1, z));

    pf = drawColor;
    // Assign color to pixel.
    for (idxV = 0; idxV <= maxV; ++idxV)
      {
      *ptr = static_cast<T>(*pf++);
      ptr++;
      }
    }
}



//----------------------------------------------------------------------------
// Draw a circle
void vtkImageCanvasSource2D::DrawPoint(int p0, int p1)
{
  void *ptr = NULL;
  int z = this->DefaultZ;

  vtkDebugMacro(<< "Drawing a point: (" << p0 << ", " << p1 << ")");

  // Pre-multiply coords if needed
  if (this->Ratio[0] != 1.0)
    {
    p0 = int(double(p0) * this->Ratio[0]);
    }
  if (this->Ratio[1] != 1.0)
    {
    p1 = int(double(p1) * this->Ratio[1]);
    }
  if (this->Ratio[2] != 1.0)
    {
    z = int(double(z) * this->Ratio[2]);
    }

  switch (this->ImageData->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageCanvasSource2DDrawPoint(this->ImageData,
                                      this->DrawColor,
                                      static_cast<VTK_TT *>(ptr),
                                      p0, p1,
                                      z));
    default:
      vtkErrorMacro(<< "DrawPoint: Cannot handle ScalarType.");
    }
  this->Modified();
}



//----------------------------------------------------------------------------
// Draw a circle.  Only implentented for 2D images.
template <class T>
void vtkImageCanvasSource2DDrawCircle(vtkImageData *image,
                                    double *drawColor, T *ptr,
                                    int c0, int c1, double radius, int z)
{
  int min0, max0, min1, max1, min2, max2, maxV;
  int idxV;
  double *pf;
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

  numberOfSteps = static_cast<int>(ceil(6.2831853 * radius));
  thetaCos = cos(1.0 / radius);
  thetaSin = sin(1.0 / radius);
  x = radius;
  y = 0.0;

  for (idx = 0; idx < numberOfSteps; ++idx)
    {
    p0 = c0+static_cast<int>(x);
    p1 = c1+static_cast<int>(y);
    if (p0 >= min0 && p0 <= max0 && p1 >= min1 && p1 <= max1)
      {
      ptr = static_cast<T *>(image->GetScalarPointer(p0, p1, z));

      pf = drawColor;
      // Assign color to pixel.
      for (idxV = 0; idxV <= maxV; ++idxV)
        {
        *ptr = static_cast<T>(*pf++);
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
void vtkImageCanvasSource2D::DrawCircle(int c0, int c1, double radius)
{
  void *ptr = NULL;
  int z = this->DefaultZ;

  vtkDebugMacro(<< "Drawing a circle: center = (" << c0 << ", " << c1
                << "), radius = " << radius);

  // Pre-multiply coords if needed
  if (this->Ratio[0] != 1.0)
    {
    c0 = int(double(c0) * this->Ratio[0]);
    radius = int(double(radius) * this->Ratio[0]);
    }
  if (this->Ratio[1] != 1.0)
    {
    c1 = int(double(c1) * this->Ratio[1]);
    }
  if (this->Ratio[2] != 1.0)
    {
    z = int(double(z) * this->Ratio[2]);
    }

  switch (this->ImageData->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageCanvasSource2DDrawCircle(this->ImageData,
                                       this->DrawColor,
                                       static_cast<VTK_TT *>(ptr),
                                       c0, c1,
                                       radius,
                                       z));
    default:
      vtkErrorMacro(<< "DrawCircle: Cannot handle ScalarType.");
    }
  this->Modified();
}



//----------------------------------------------------------------------------
// Draw a line.  Only implentented for 2D images.
// First point is already shifted to origin.
template <class T>
void vtkImageCanvasSource2DDrawSegment(vtkImageData *image,
                                     double *drawColor, T *ptr,
                                     int p0, int p1)
{
  double f0, f1;
  double s0, s1;
  int numberOfSteps;
  int  maxV;
  int idx, idxV;
  vtkIdType inc0, inc1, inc2;
  double *pf;
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
  s0 = static_cast<double>(p0) / static_cast<double>(numberOfSteps);
  s1 = static_cast<double>(p1) / static_cast<double>(numberOfSteps);

  f0 = f1 = 0.5;

  ptrV = ptr;
  pf = drawColor;
  // Assign color to pixel.
  for (idxV = 0; idxV <= maxV; ++idxV)
    {
    *ptrV = static_cast<T>(*pf++);
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
      *ptrV = static_cast<T>(*pf++);
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

  // Pre-multiply coords if needed
  if (this->Ratio[0] != 1.0)
    {
    a0 = static_cast<int>(static_cast<double>(a0) * this->Ratio[0]);
    b0 = static_cast<int>(static_cast<double>(b0) * this->Ratio[0]);
    }
  if (this->Ratio[1] != 1.0)
    {
    a1 = static_cast<int>(static_cast<double>(a1) * this->Ratio[1]);
    b1 = static_cast<int>(static_cast<double>(b1) * this->Ratio[1]);
    }
  if (this->Ratio[2] != 1.0)
    {
    z = static_cast<int>(static_cast<double>(z) * this->Ratio[2]);
    }

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
    vtkTemplateMacro(
      vtkImageCanvasSource2DDrawSegment(this->ImageData,
                                        this->DrawColor,
                                        static_cast<VTK_TT *>(ptr),
                                        a0, a1));
    default:
      vtkErrorMacro(<< "DrawSegment: Cannot handle ScalarType.");
    }
  this->Modified();
}



//----------------------------------------------------------------------------
// Clips a line segment so it will be in bounds.
// If the entire segment is out of bounds, the method returns 0.
int vtkImageCanvasSource2D::ClipSegment(int &a0, int &a1, int &b0, int &b1)
{
  int min0, max0, min1, max1, min2, max2;
  double fract;


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
    fract = static_cast<double>(b0 - min0) / static_cast<double>(b0 - a0);
    a0 = min0;
    a1 = b1 + static_cast<int>(fract * static_cast<double>(a1 - b1));
    }
  // second out of bounds.
  if (b0 < min0 && a0 >= min0)
    {
    // interpolate to find point on bounding plane.
    fract = static_cast<double>(a0 - min0) / static_cast<double>(a0 - b0);
    b0 = min0;
    b1 = a1 + static_cast<int>(fract * static_cast<double>(b1 - a1));
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
    fract = static_cast<double>(b0 - max0) / static_cast<double>(b0 - a0);
    a0 = max0;
    a1 = b1 + static_cast<int>(fract * static_cast<double>(a1 - b1));
    }
  // second out of bounds.
  if (b0 > max0 && a0 <= max0)
    {
    // interpolate to find point on bounding plane.
    fract = static_cast<double>(a0 - max0) / static_cast<double>(a0 - b0);
    b0 = max0;
    b1 = a1 + static_cast<int>(fract * static_cast<double>(b1 - a1));
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
    fract = static_cast<double>(b1 - min1) / static_cast<double>(b1 - a1);
    a1 = min1;
    a0 = b0 + static_cast<int>(fract * static_cast<double>(a0 - b0));
    }
  // second out of bounds.
  if (b1 < min1 && a1 >= min1)
    {
    // interpolate to find point on bounding plane.
    fract = static_cast<double>(a1 - min1) / static_cast<double>(a1 - b1);
    b1 = min1;
    b0 = a0 + static_cast<int>(fract * static_cast<double>(b0 - a0));
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
    fract = static_cast<double>(b1 - max1) / static_cast<double>(b1 - a1);
    a1 = max1;
    a0 = b0 + static_cast<int>(fract * static_cast<double>(a0 - b0));
    }
  // second out of bounds.
  if (b1 > max1 && a1 <= max1)
    {
    // interpolate to find point on bounding plane.
    fract = static_cast<double>(a1 - max1) / static_cast<double>(a1 - b1);
    b1 = max1;
    b0 = a0 + static_cast<int>(fract * static_cast<double>(b0 - a0));
    }

  this->Modified();
  return 1;
}

//----------------------------------------------------------------------------
// Draw a line.  Only implentented for 3D images.
// First point is already shifted to origin.
template <class T>
void vtkImageCanvasSource2DDrawSegment3D(vtkImageData *image,
                                       double *drawColor,
                                       T *ptr, int p0, int p1, int p2)
{
  double f0, f1, f2;
  double s0, s1, s2;
  int numberOfSteps;
  int idx, idxV,  maxV;
  vtkIdType inc0, inc1, inc2;
  double *pf;
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
  s0 = static_cast<double>(p0) / static_cast<double>(numberOfSteps);
  s1 = static_cast<double>(p1) / static_cast<double>(numberOfSteps);
  s2 = static_cast<double>(p2) / static_cast<double>(numberOfSteps);

  f0 = f1 = f2 = 0.5;


  ptrV = ptr;
  pf = drawColor;
  // Assign color to pixel.
  for (idxV = 0; idxV <= maxV; ++idxV)
    {
    *ptrV = static_cast<T>(*pf++);
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
      *ptrV = static_cast<T>(*pf++);
      ptrV++;
      }
    }
}


//----------------------------------------------------------------------------
// Draw a Segment from point a to point b.
// No clipping or bounds checking.
void vtkImageCanvasSource2D::DrawSegment3D(double *a, double *b)
{
  void *ptr;
  int a0, a1, a2;

  // Pre-multiply coords if needed
  if (this->Ratio[0] != 1.0)
    {
    a[0] = static_cast<int>(static_cast<double>(a[0]) * this->Ratio[0]);
    b[0] = static_cast<int>(static_cast<double>(b[0]) * this->Ratio[0]);
    }
  if (this->Ratio[1] != 1.0)
    {
    a[1] = static_cast<int>(static_cast<double>(a[1]) * this->Ratio[1]);
    b[1] = static_cast<int>(static_cast<double>(b[1]) * this->Ratio[1]);
    }
  if (this->Ratio[2] != 1.0)
    {
    a[2] = static_cast<int>(static_cast<double>(a[2]) * this->Ratio[2]);
    b[2] = static_cast<int>(static_cast<double>(b[2]) * this->Ratio[2]);
    }

  ptr = this->ImageData->GetScalarPointer(static_cast<int>(b[0] + 0.5),
                                          static_cast<int>(b[1] + 0.5),
                                          static_cast<int>(b[2] + 0.5));
  a0 = static_cast<int>(a[0] - b[0] + 0.5);
  a1 = static_cast<int>(a[1] - b[1] + 0.5);
  a2 = static_cast<int>(a[2] - b[2] + 0.5);
  switch (this->ImageData->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageCanvasSource2DDrawSegment3D(this->ImageData,
                                          this->DrawColor,
                                          static_cast<VTK_TT *>(ptr),
                                          a0, a1, a2));
    default:
      vtkErrorMacro(<< "DrawSegment3D: Cannot handle ScalarType.");
    }
  this->Modified();
}



//----------------------------------------------------------------------------
template <class T>
void vtkImageCanvasSource2DFill(vtkImageData *image, double *color,
                                T *ptr, int x, int y)
{
  vtkImageCanvasSource2DPixel *pixel;
  vtkImageCanvasSource2DPixel *first, *last;
  vtkImageCanvasSource2DPixel *heap = NULL;
  int min0, max0, min1, max1, min2, max2, maxV;
  int idxV;
  vtkIdType inc0, inc1, inc2;
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
    drawColor[idxV] = static_cast<T>(color[idxV]);
    if (*ptrV != drawColor[idxV])
      {
      temp = 0;
      }
    ptrV++;
    }
  if (temp)
    { // fill the same as draw
    vtkGenericWarningMacro(
      "Fill: Cannot handle draw color same as fill color" );
    return;
    }

  // Create the seed
  pixel = vtkImageCanvasSource2DPixel::New();
  pixel->X = x;
  pixel->Y = y;
  pixel->Pointer = static_cast<void *>(ptr);
  pixel->Next = NULL;
  first = last = pixel;
  // change the seeds color
  ptrV = static_cast<T *>(last->Pointer);
  ptrC = drawColor;
  for (idxV = 0; idxV <= maxV; ++idxV)
    {
    *ptrV = *ptrC++;
    ptrV++;
    }

  while (first)
    {
    ptr = static_cast<T *>(first->Pointer);

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
        pixel->Pointer = static_cast<void *>(ptr - inc0);
        pixel->Next = NULL;
        last->Next = pixel;
        last = pixel;
        // change the seeds color
        ptrV = static_cast<T *>(last->Pointer);
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
        pixel->Pointer = static_cast<void *>(ptr + inc0);
        pixel->Next = NULL;
        last->Next = pixel;
        last = pixel;
        // change the seeds color
        ptrV = static_cast<T *>(last->Pointer);
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
        pixel->Pointer = static_cast<void *>(ptr - inc1);
        pixel->Next = NULL;
        last->Next = pixel;
        last = pixel;
        // change the seeds color
        ptrV = static_cast<T *>(last->Pointer);
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
        pixel->Pointer = static_cast<void *>(ptr + inc1);
        pixel->Next = NULL;
        last->Next = pixel;
        last = pixel;
        // change the seeds color
        ptrV = static_cast<T *>(last->Pointer);
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

  // Pre-multiply coords if needed
  if (this->Ratio[0] != 1.0)
    {
    x = static_cast<int>(static_cast<double>(x) * this->Ratio[0]);
    }
  if (this->Ratio[1] != 1.0)
    {
    y = static_cast<int>(static_cast<double>(y) * this->Ratio[1]);
    }
  if (this->Ratio[2] != 1.0)
    {
    z = static_cast<int>(static_cast<double>(z) * this->Ratio[2]);
    }

  z = (z < ext[4]) ? ext[4] : z;
  z = (z > ext[5]) ? ext[5] : z;

  ptr = this->ImageData->GetScalarPointer(x, y, z);

  switch (this->ImageData->GetScalarType())
    {
    vtkTemplateMacro(
      vtkImageCanvasSource2DFill(this->ImageData,
                                 this->DrawColor,
                                 static_cast<VTK_TT *>(ptr),
                                 x, y));
    default:
      vtkErrorMacro(<< "Fill: Cannot handle ScalarType.");
    }
  this->Modified();
}



//----------------------------------------------------------------------------
void vtkImageCanvasSource2D::SetExtent(int *extent)
{
  this->SetExtent(extent[0], extent[1],
                  extent[2], extent[3],
                  extent[4], extent[5]);
}

//----------------------------------------------------------------------------
void vtkImageCanvasSource2D::SetExtent(int xMin, int xMax,
                                       int yMin, int yMax,
                                       int zMin, int zMax)
{
  int modified = 0;

  if (this->WholeExtent[0] != xMin)
    {
    modified = 1;
    this->WholeExtent[0] = xMin ;
    }
  if (this->WholeExtent[1] != xMax)
    {
    modified = 1;
    this->WholeExtent[1] = xMax ;
    }
  if (this->WholeExtent[2] != yMin)
    {
    modified = 1;
    this->WholeExtent[2] = yMin ;
    }
  if (this->WholeExtent[3] != yMax)
    {
    modified = 1;
    this->WholeExtent[3] = yMax ;
    }
  if (this->WholeExtent[4] != zMin)
    {
    modified = 1;
    this->WholeExtent[4] = zMin ;
    }
  if (this->WholeExtent[5] != zMax)
    {
    modified = 1;
    this->WholeExtent[5] = zMax ;
    }
  if (modified)
    {
    this->Modified();
    this->ImageData->SetExtent(this->WholeExtent);
    this->ImageData->AllocateScalars();
    }
}

//----------------------------------------------------------------------------
void vtkImageCanvasSource2D::SetScalarType(int t)
{
  if (this->ImageData->GetScalarType() != t)
    {
    this->Modified();
    this->ImageData->SetScalarType(t);
    this->ImageData->AllocateScalars();
    }
}

//----------------------------------------------------------------------------
int vtkImageCanvasSource2D::GetScalarType() const
{
  return this->ImageData->GetScalarType();
}


//----------------------------------------------------------------------------
void vtkImageCanvasSource2D::SetNumberOfScalarComponents(int t)
{
  if (this->ImageData->GetNumberOfScalarComponents() != t)
    {
    this->Modified();
    this->ImageData->SetNumberOfScalarComponents(t);
    this->ImageData->AllocateScalars();
    }
}

//----------------------------------------------------------------------------
int  vtkImageCanvasSource2D::GetNumberOfScalarComponents() const 
{
  return this->ImageData->GetNumberOfScalarComponents();
}

//----------------------------------------------------------------------------
int vtkImageCanvasSource2D::RequestInformation (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector *outputVector)
{
  // get the info objects
  vtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               this->WholeExtent,6);

  vtkDataObject::SetPointDataActiveScalarInfo
    (outInfo, this->ImageData->GetScalarType(),
     this->ImageData->GetNumberOfScalarComponents());
  return 1;
}

//----------------------------------------------------------------------------
int vtkImageCanvasSource2D::RequestData (
  vtkInformation * vtkNotUsed(request),
  vtkInformationVector** vtkNotUsed( inputVector ),
  vtkInformationVector *outputVector)
{
  // get the data object
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkImageData *output = vtkImageData::SafeDownCast
    (outInfo->Get(vtkDataObject::DATA_OBJECT()) );

  output->ShallowCopy(this->ImageData);

  return 1;
}
