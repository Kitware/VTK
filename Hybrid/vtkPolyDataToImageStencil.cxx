/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataToImageStencil.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

Copyright (c) 2004 Atamai, Inc.

Use, modification and redistribution of the software, in source or
binary forms, are permitted provided that the following terms and
conditions are met:

1) Redistribution of the source code, in verbatim or modified
   form, must retain the above copyright notice, this license,
   the following disclaimer, and any notices that refer to this
   license and/or the following disclaimer.  

2) Redistribution in binary form must include the above copyright
   notice, a copy of this license and the following disclaimer
   in the documentation or with other materials provided with the
   distribution.

3) Modified copies of the source code must be clearly marked as such,
   and must not be misrepresented as verbatim copies of the source code.

THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE SOFTWARE "AS IS"
WITHOUT EXPRESSED OR IMPLIED WARRANTY INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  IN NO EVENT SHALL ANY COPYRIGHT HOLDER OR OTHER PARTY WHO MAY
MODIFY AND/OR REDISTRIBUTE THE SOFTWARE UNDER THE TERMS OF THIS LICENSE
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, LOSS OF DATA OR DATA BECOMING INACCURATE
OR LOSS OF PROFIT OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF
THE USE OR INABILITY TO USE THE SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGES.

=========================================================================*/
#include "vtkPolyDataToImageStencil.h"
#include "vtkImageStencilData.h"
#include "vtkObjectFactory.h"

#include "vtkPolyData.h"
#include "vtkCellArray.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkStreamingDemandDrivenPipeline.h"

#include <math.h>
#include <vtkstd/vector>
#include <vtkstd/algorithm>

vtkStandardNewMacro(vtkPolyDataToImageStencil);


// A class to avoid the overhead of small STL vectors
class vtkPolyDataToImageStencilPoint3D
{
public:
  double xyz[3];

  vtkPolyDataToImageStencilPoint3D() {};
  vtkPolyDataToImageStencilPoint3D(const double point[3]) {
     xyz[0] = point[0]; xyz[1] = point[1]; xyz[2] = point[2]; };
  vtkPolyDataToImageStencilPoint3D(const float point[3]) {
     xyz[0] = point[0]; xyz[1] = point[1]; xyz[2] = point[2]; };
  vtkPolyDataToImageStencilPoint3D(
     const vtkPolyDataToImageStencilPoint3D &point) {
     xyz[0] = point[0]; xyz[1] = point[1]; xyz[2] = point[2]; };

  const double& operator[](int i) const { return xyz[i]; };
  double& operator[](int i) { return xyz[i]; };
};

typedef vtkPolyDataToImageStencilPoint3D Point3D;

// A class to avoid the overhead of small STL vectors
class vtkPolyDataToImageStencilPoint2D
{
public:
  double xy[2];

  vtkPolyDataToImageStencilPoint2D() {};
  vtkPolyDataToImageStencilPoint2D(const double point[2]) {
     xy[0] = point[0]; xy[1] = point[1]; };
  vtkPolyDataToImageStencilPoint2D(const float point[2]) {
     xy[0] = point[0]; xy[1] = point[1]; };
  vtkPolyDataToImageStencilPoint2D(
     const vtkPolyDataToImageStencilPoint2D &point) {
     xy[0] = point[0]; xy[1] = point[1]; };

  const double& operator[](int i) const { return xy[i]; };
  double& operator[](int i) { return xy[i]; };
};

typedef vtkPolyDataToImageStencilPoint2D Point2D;

// A class to avoid the overhead of small STL vectors
class vtkPolyDataToImageStencilPoint1D
{
public:
  double x;
  int sign;
  int pad; // pad to 16 bytes

  vtkPolyDataToImageStencilPoint1D() {};
  vtkPolyDataToImageStencilPoint1D(const double p, const int s) {
    x = p; sign = s; };
  vtkPolyDataToImageStencilPoint1D(
    const vtkPolyDataToImageStencilPoint1D &point) {
    x = point.x; sign = point.sign;
  };

  const double getX() const {
    return x; };
  const int getSign() const {
    return sign; };
};

typedef vtkPolyDataToImageStencilPoint1D Point1D;

//----------------------------------------------------------------------------
vtkPolyDataToImageStencil::vtkPolyDataToImageStencil()
{
  this->Tolerance = 1e-5;
}

//----------------------------------------------------------------------------
vtkPolyDataToImageStencil::~vtkPolyDataToImageStencil()
{
}

void vtkPolyDataToImageStencil::SetInput(vtkPolyData *input)
{
  if (input)
    {
    this->SetInputConnection(0, input->GetProducerPort());
    }
  else
    {
    this->SetInputConnection(0, 0);
    }
}

//----------------------------------------------------------------------------
vtkPolyData *vtkPolyDataToImageStencil::GetInput()
{
  if (this->GetNumberOfInputConnections(0) < 1)
    {
    return NULL;
    }
  
  return vtkPolyData::SafeDownCast(
    this->GetExecutive()->GetInputData(0, 0));
}

//----------------------------------------------------------------------------
void vtkPolyDataToImageStencil::PrintSelf(ostream& os,
                                          vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Input: " << this->GetInput() << "\n";
  os << indent << "Tolerance: " << this->Tolerance << "\n";
}

// used by an STL sort
static vtkstd_bool vtkComparePoints2D(const Point2D a, const Point2D b)
{
  // sort xy points by ascending y value, then x
  if (a[1] == b[1])
    {
    return (a[0] < b[0]);
    }
  else
    {
    return (a[1] < b[1]);
    }
}

// used by an STL sort
static vtkstd_bool vtkComparePoints1D(const Point1D a, const Point1D b)
{
  return (a.x < b.x);
}

//----------------------------------------------------------------------------
int vtkPolyDataToImageStencil::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  this->Superclass::RequestData(request, inputVector, outputVector);

  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  vtkPolyData *polyData = vtkPolyData::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkImageStencilData *data = vtkImageStencilData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  int extent[6];
  data->GetExtent(extent);

  double *spacing = data->GetSpacing();
  double *origin = data->GetOrigin();

  // Only divide once
  double invspacing[3];
  invspacing[0] = 1.0/spacing[0];
  invspacing[1] = 1.0/spacing[1];
  invspacing[2] = 1.0/spacing[2];

  // get the points and the cells
  vtkCellArray *polys = polyData->GetPolys();
  vtkCellArray *strips = polyData->GetStrips();
  vtkPoints *points = polyData->GetPoints();

  vtkIdType numPolys = 0;
  if (polys)
    {
    numPolys = polys->GetNumberOfCells();
    }
  vtkIdType numStrips = 0;
  if (strips)
    {
    numStrips = strips->GetNumberOfCells();
    }

  // quit if no polys or strips
  if (numPolys == 0 && numStrips == 0)
    {
    return 1;
    }

  // collect the points and convert coords to image-space
  double point[3];
  vtkIdType nPoints = points->GetNumberOfPoints();
  vtkPoints *newpoints = vtkPoints::New();
  newpoints->SetNumberOfPoints(nPoints);
  for (vtkIdType i = 0; i < nPoints; i++)
    {
    points->GetPoint(i, point);

    point[0] = (point[0]-origin[0])*invspacing[0];
    point[1] = (point[1]-origin[1])*invspacing[1];
    point[2] = (point[2]-origin[2])*invspacing[2];

    newpoints->SetPoint(i, point);
    }
  
  // for cell traversal
  vtkIdType npts = 0;
  vtkIdType *pts;
  int firstStrip = 1;

  // integer points
  int ymin, ymax, y;
  int zmin, zmax, z;

  // double points
  double XY[2];
  double X;
  double X1,Y1,X2,Y2;

  // area, sign and optimizations
  double area;
  double v1y, v1z, v2y, v2z;
  double temp, r, f;
  int sign;

  // my points
  Point3D p0, p1, p2;
  Point2D thisPoint2D;

  // STL vectors 
  int zInc = extent[3]-extent[2]+1;
  vtkstd::vector<Point3D> coords;
  vtkstd::vector<Point2D> xylist;
  vtkstd::vector< vtkstd::vector<Point2D> > matrix(zInc);

  // iterators
  vtkstd::vector<Point3D>::iterator coordIter;

  // the stencil is kept in 'zymatrix' that provides 
  // the x extents for each (y,z) coordinate for which a ray 
  //parallel to the x axis intersects the polydata
  vtkstd::vector< vtkstd::vector<Point1D> > zymatrix(zInc*zInc);

  //loop through the polys and strips
  vtkIdType polyIdx = 0;
  vtkIdType stripIdx = 0;
  int isodd = 0;
  polys->InitTraversal();
  strips->InitTraversal();
  for (;;)
    {
    if (polyIdx < numPolys) // handle the polygons
      {
      polys->GetNextCell(npts, pts);
      polyIdx++;
      // get the converted points
      coords.clear();
      for (vtkIdType j = 0; j < npts; j++)
        {
        coords.push_back( Point3D(newpoints->GetPoint(pts[j])) );
        }
      }
    else // handle the triangle strips
      {
      if (firstStrip || npts < 3) //move on to the next strip
        {
        firstStrip = 0;
        if (stripIdx < numStrips)
          {
          strips->GetNextCell(npts, pts);
          stripIdx++;
          isodd = 0;
          }
        else // no more strips
          {
          break;
          }
        }
      // get the converted points
      coords.clear();
      if (isodd) // if stripSubIdx is odd
        {
        coords.push_back( Point3D(newpoints->GetPoint(pts[0])) );
        coords.push_back( Point3D(newpoints->GetPoint(pts[2])) );
        coords.push_back( Point3D(newpoints->GetPoint(pts[1])) );
        }
      else // if stripSubIdx is even
        {
        coords.push_back( Point3D(newpoints->GetPoint(pts[0])) );
        coords.push_back( Point3D(newpoints->GetPoint(pts[1])) );
        coords.push_back( Point3D(newpoints->GetPoint(pts[2])) );
        }
      pts++;
      npts--;
      isodd = !isodd;
      }

    // clear the matrix 
    for (z = 0; z < (int)(matrix.size()); z++)
      {
      matrix[z].clear();
      }

    // convert the polgon into a rasterizable form by finding its
    // intersection with each z plane, and store the (x,y) coord
    // of each intersection in a dictionary called "matrix"
    p0 = coords[0];
    p1 = coords[(int)(coords.size())-1];
    area = 0.0;
    for (coordIter = coords.begin(); coordIter != coords.end(); coordIter++)
      {
      p2 = *coordIter;
                        
      // calculate area via cross product before swapping
      v1y = p1[1]-p0[1];
      v1z = p1[2]-p0[2];
      v2y = p2[1]-p0[1];
      v2z = p2[2]-p0[2];
      area += (v1y*v2z - v2y*v1z);

      if (p1[2] == p2[2])
        {
        p1 = *coordIter;
        continue;
        }

      if (p1[2] > p2[2])
        {
        vtkstd::swap(p1,p2);
        }

      zmin = (int)(ceil(p1[2]));
      zmax = (int)(ceil(p2[2]));
          
      if (zmin > extent[5] || zmax < extent[4])
        {
        continue;
        }

      // cap to the volume extents
      if (zmin < extent[4])
        {
        zmin = extent[4];
        }
      if (zmax > extent[5])
        {
        zmax = extent[5];
        }

      temp = 1.0/(p2[2] - p1[2]);
      for (z = zmin; z <= zmax; z++)
        {
        r = (p2[2] - (double)z)*temp;
        f = 1.0 - r;
        XY[0] = r*p1[0] + f*p2[0];
        XY[1] = r*p1[1] + f*p2[1];
              
        matrix[z].push_back(Point2D(XY));
        } 
      p1 = *coordIter;
      }

    // area is not really needed, we just need the sign
    if (area < 0.0)
      {
      sign = -1;
      }
    else if (area > 0.0)
      {
      sign = 1;
      }
    else
      {
      continue;
      }
        
    // rasterize the polygon and store the x coord for each (y,z)
    // point that we rasterize, kind of like using a depth buffer
    // except that 'x' is our depth value and we can store multiple
    // 'x' values per (y,z) value.
    for (z = 0; z < (int)(matrix.size()); z++)
      {
      xylist = matrix[z];
          
      if (xylist.empty())
        {
        continue;
        }
          
      // sort by ascending y, then x
      vtkstd::sort(xylist.begin(), xylist.end(), vtkComparePoints2D);
          
      for (int k = 0; k < (int)(xylist.size())/2; k++)
        {
        thisPoint2D = xylist[2*k];
        X1 = thisPoint2D[0]; Y1 = thisPoint2D[1];
        thisPoint2D = xylist[2*k+1];
        X2 = thisPoint2D[0]; Y2 = thisPoint2D[1];
              
        if (Y2 == Y1)
          {
          continue;
          }
              
        temp = 1.0/(Y2 - Y1);
        ymin = (int)(ceil(Y1));
        ymax = (int)(ceil(Y2));
        for (y = ymin; y < ymax; y++)
          { 
          r = (Y2 - y)*temp;
          f = 1.0 - r;
          X = r*X1 + f*X2;
          
          if (extent[2] <= y && y <= extent[3])
            {
            zymatrix[z*zInc+y].push_back( Point1D(X,sign) );
            }
          }
        }
      }
    }

  newpoints->Delete();

  Point1D thisPoint1D;
  vtkstd::vector<Point1D> xlist;
  vtkstd::vector<Point1D>::iterator xIter;
  vtkstd::vector<double> nlist;

  // at this stage, y,z are ints but x is not
  double x, lastx;
  int signproduct, lastsign;
  
  int x1, x2, minx1;
  
  // we only want to generate a max of 1 warning per execute
  int alreadywarned = 1;

  // create the vtkStencilData from our zymatrix
  for (z = extent[4]; z <= extent[5]; z++)
    {
    for (y = extent[2]; y <= extent[3]; y++)
      {
      xlist = zymatrix[z*zInc+y];
      if (xlist.empty())
        {
        continue;
        }

      vtkstd::sort(xlist.begin(), xlist.end(), vtkComparePoints1D);
      nlist.clear();

      // get the first entry
      thisPoint1D = xlist[0];
      lastx = thisPoint1D.x;
      lastsign = thisPoint1D.sign;
      signproduct = 1;

      // now remove the first entry
      xlist.erase(xlist.begin());
          
      // if adjacent x values are within tolerance of each
      // other, check whether the number of 'exits' and
      // 'entrances' are equal (via signproduct) and if so,
      // ignore all x values, but if not, then count
      // them as a single intersection of the ray with the
      // surface
      for (xIter = xlist.begin(); xIter != xlist.end(); xIter++)
        {
        thisPoint1D = *xIter;
        x = thisPoint1D.x;
        sign = thisPoint1D.sign;
        
        // check absolute distance from lastx to x
        if (((x < lastx) ? (lastx - x) : (x - lastx)) > this->Tolerance)
          {
          if (signproduct > 0)
            {
            nlist.push_back(lastx);
            }
          signproduct = 1;
          }
        else
          {
          signproduct *= sign;
          }
        lastx = x;
        }
      if (signproduct > 0)
        {
        nlist.push_back(lastx);
        }
 
      // if xlist length is not divisible by two, then
      // the polydata isn't a closed surface
      if ((int)(nlist.size())%2 != 0 && !alreadywarned)
        {
        alreadywarned = 1;
        vtkWarningMacro("RequestInformation: PolyData does not form a closed surface");
        }

      // create the stencil extents
      minx1 = extent[0]; // minimum allowable x1 value
      int n = (int)(nlist.size())/2;
      for (int i = 0; i < n; i++)
        {
        x1 = (int)(ceil(nlist[2*i]));
        x2 = (int)(floor(nlist[2*i+1]));
        
        if (x2 < extent[0] || x1 > extent[1])
          {
          continue;
          }

        x1 = (x1 > minx1) ? (x1) : (minx1); // max(x1,minx1)
        x2 = (x2 < extent[1]) ? (x2) : (extent[1]); //min(x2,extent[1])

        if (x2 >= x1)
          {
          data->InsertNextExtent(x1,x2,y,z);
          }
        // next x1 value must be at least x2+1
        minx1 = x2+1;
        }   
      }
    }

  return 1;
}

//----------------------------------------------------------------------------
int vtkPolyDataToImageStencil::RequestInformation(
  vtkInformation *,
  vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  
  // this is an odd source that can produce any requested size.  so its whole
  // extent is essentially infinite. This would not be a great source to
  // connect to some sort of writer or viewer. For a sanity check we will
  // limit the size produced to something reasonable (depending on your
  // definition of reasonable)
  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               0, VTK_LARGE_INTEGER >> 2,
               0, VTK_LARGE_INTEGER >> 2,
               0, VTK_LARGE_INTEGER >> 2);
  return 1;
}

//----------------------------------------------------------------------------
int vtkPolyDataToImageStencil::FillInputPortInformation(int,
                                                        vtkInformation* info)
{
  info->Set(vtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "vtkPolyData");
  return 1;
}

