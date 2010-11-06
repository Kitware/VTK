/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLassooStencilSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLassooStencilSource.h"

#include "vtkMath.h"
#include "vtkPoints.h"
#include "vtkCardinalSpline.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkImageStencilData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkSmartPointer.h"

#include <math.h>
#include <vtkstd/map>
#include <vtkstd/vector>
#include <vtkstd/deque>
#include <vtkstd/algorithm>

vtkStandardNewMacro(vtkLassooStencilSource);
vtkCxxSetObjectMacro(vtkLassooStencilSource, InformationInput, vtkImageData);
vtkCxxSetObjectMacro(vtkLassooStencilSource, Points, vtkPoints);

//----------------------------------------------------------------------------
class vtkLSSPointMap : public vtkstd::map<int, vtkSmartPointer<vtkPoints> >
{
};

//----------------------------------------------------------------------------
vtkLassooStencilSource::vtkLassooStencilSource()
{
  this->SetNumberOfInputPorts(0);

  this->Shape = vtkLassooStencilSource::POLYGON;
  this->SliceOrientation = 2;
  this->Points = NULL;
  this->SplineX = vtkCardinalSpline::New();
  this->SplineY = vtkCardinalSpline::New();

  this->InformationInput = NULL;

  this->OutputOrigin[0] = 0;
  this->OutputOrigin[1] = 0;
  this->OutputOrigin[2] = 0;

  this->OutputSpacing[0] = 1;
  this->OutputSpacing[1] = 1;
  this->OutputSpacing[2] = 1;

  this->OutputWholeExtent[0] = 0;
  this->OutputWholeExtent[1] = 0;
  this->OutputWholeExtent[2] = 0;
  this->OutputWholeExtent[3] = 0;
  this->OutputWholeExtent[4] = 0;
  this->OutputWholeExtent[5] = 0;

  this->PointMap = new vtkLSSPointMap();
}

//----------------------------------------------------------------------------
vtkLassooStencilSource::~vtkLassooStencilSource()
{
  this->SetInformationInput(NULL);
  this->SetPoints(NULL);
  if (this->SplineX)
    {
    this->SplineX->Delete();
    this->SplineX = NULL;
    }
  if (this->SplineY)
    {
    this->SplineY->Delete();
    this->SplineY = NULL;
    }
  if (this->PointMap)
    {
    delete this->PointMap;
    this->PointMap = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkLassooStencilSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "InformationInput: " << this->InformationInput << "\n";

  os << indent << "OutputSpacing: " << this->OutputSpacing[0] << " " <<
    this->OutputSpacing[1] << " " << this->OutputSpacing[2] << "\n";
  os << indent << "OutputOrigin: " << this->OutputOrigin[0] << " " <<
    this->OutputOrigin[1] << " " << this->OutputOrigin[2] << "\n";
  os << indent << "OutputWholeExtent: " << this->OutputWholeExtent[0] << " " <<
    this->OutputWholeExtent[1] << " " << this->OutputWholeExtent[2] << " " <<
    this->OutputWholeExtent[3] << " " << this->OutputWholeExtent[4] << " " <<
    this->OutputWholeExtent[5] << "\n";

  os << indent << "Shape: " << this->GetShapeAsString() << "\n";
  os << indent << "Points: " << this->Points << "\n";
  os << indent << "SliceOrientation: " << this->GetSliceOrientation() << "\n";
  os << indent << "SlicePoints: " << this->PointMap->size() << "\n";
}

//----------------------------------------------------------------------------
const char *vtkLassooStencilSource::GetShapeAsString()
{
  switch (this->Shape)
    {
    case vtkLassooStencilSource::POLYGON:
      return "Polygon";
    case vtkLassooStencilSource::SPLINE:
      return "Spline";
    }
  return "";
}

//----------------------------------------------------------------------------
unsigned long int vtkLassooStencilSource::GetMTime()
{
  unsigned long mTime = this->vtkImageStencilSource::GetMTime();

  if ( this->Points != NULL )
    {
    unsigned long t = this->Points->GetMTime();
    if (t > mTime)
      {
      mTime = t;
      }
    }

  if ( !this->PointMap->empty() )
    {
    vtkLSSPointMap::iterator iter = this->PointMap->begin();
    while ( iter != this->PointMap->end() )
      {
      unsigned long t = iter->second->GetMTime();
      if (t > mTime)
        {
        mTime = t;
        }
      iter++;
      }
    }

  return mTime;
}

//----------------------------------------------------------------------------
void vtkLassooStencilSource::SetSlicePoints(int i, vtkPoints *points)
{
  vtkLSSPointMap::iterator iter = this->PointMap->find(i);
  if (iter != this->PointMap->end())
    {
    if (iter->second == points)
      {
      return;
      }
    else if (points == 0)
      {
      this->PointMap->erase(iter);
      }
    else
      {
      iter->second = points;
      }
    }
  else
    {
    if (points == NULL)
      {
      return;
      }
    else
      {
      this->PointMap->insert(iter, vtkLSSPointMap::value_type(i, points));
      }
    }

  this->Modified();
}

//----------------------------------------------------------------------------
void vtkLassooStencilSource::RemoveAllSlicePoints()
{
  this->PointMap->clear();
}

//----------------------------------------------------------------------------
vtkPoints *vtkLassooStencilSource::GetSlicePoints(int i)
{
  vtkLSSPointMap::iterator iter = this->PointMap->find(i);
  if (iter != this->PointMap->end())
    {
    return iter->second;
    }
  return NULL;
}

//----------------------------------------------------------------------------
// tolerance for stencil operations

#define VTK_STENCIL_TOL 7.62939453125e-06

//----------------------------------------------------------------------------
// Compute a reduced extent based on the bounds of the shape.
static void vtkLassooStencilSourceSubExtent(
  vtkPoints *points,
  const double origin[3], const double spacing[3],
  const int extent[6], int subextent[6])
{
  double bounds[6];
  points->GetBounds(bounds);

  for (int i = 0; i < 3; i++)
    {
    double emin = (bounds[2*i] - origin[i])/spacing[i] - VTK_STENCIL_TOL;
    double emax = (bounds[2*i+1] - origin[i])/spacing[i] + VTK_STENCIL_TOL;

    subextent[2*i] = extent[2*i];
    subextent[2*i+1] = extent[2*i+1];

    if (extent[2*i] < emin)
      {
      subextent[2*i] = VTK_INT_MAX;
      if (extent[2*i+1] >= emin)
        {
        subextent[2*i] = vtkMath::Floor(emin) + 1;
        }
      }

    if (extent[2*i+1] > emax)
      {
      subextent[2*i+1] = VTK_INT_MIN;
      if (extent[2*i] <= emax)
        {
        subextent[2*i+1] = vtkMath::Floor(emax);
        }
      }

    }
}

//----------------------------------------------------------------------------
// Use Bresenham algorithm to draw a line into the stencil
static void vtkLassooStencilSourceBresenham(
  const double pt1[2], const double pt2[2],
  bool inflection1, bool inflection2,
  const int extentY[2], int subextentY[2],
  vtkstd::deque< vtkstd::vector<double> >& raster)
{
  double x1 = pt1[0];
  double x2 = pt2[0];
  double y1 = pt1[1];
  double y2 = pt2[1];

  // swap end points if necessary
  if (y1 > y2)
    {
    x1 = pt2[0];
    x2 = pt1[0];
    y1 = pt2[1];
    y2 = pt1[1];
    bool tmp = inflection1;
    inflection1 = inflection2;
    inflection2 = tmp;
    }

  // find min and max of x values
  double xmin = x1;
  double xmax = x2;
  if (x1 > x2)
    {
    xmin = x2;
    xmax = x1;
    }

  // check for parallel to the x-axis
  if (y1 == y2)
    {
    return;
    }

  double ymin = y1;
  double ymax = y2;

  if (inflection1)
    {
    // if this is a lower inflection point, include a tolerance
    ymin -= VTK_STENCIL_TOL;
    }
  if (inflection2)
    {
    // likewise, if upper inflection, add tolerance at top
    ymax += VTK_STENCIL_TOL;
    }

  // Integer y values for start and end of line
  int iy1, iy2;
  iy1 = vtkMath::Floor(ymin) + 1;
  iy2 = vtkMath::Floor(ymax);

  // Guard against extentY
  if (iy1 < extentY[0])
    {
    iy1 = extentY[0];
    }
  if (iy2 > extentY[1])
    {
    iy2 = extentY[1];
    }

  // Expand subextentY if necessary
  while (iy1 < subextentY[0])
    {
    raster.push_front(vtkstd::vector<double>());
    subextentY[0] = subextentY[0] - 1;
    }
  while (iy2 > subextentY[1])
    {
    raster.push_back(vtkstd::vector<double>());
    subextentY[1] = subextentY[1] + 1;
    }

  // Precompute values for a Bresenham-like line algorithm
  double grad = (x2 - x1)/(y2 - y1);
  double delta = (iy1 - y1)*grad;

  // Go along y and place each x in the proper raster line
  for (int y = iy1; y <= iy2; y++)
    {
    double x = x1 + delta;
    delta += grad;

    // clamp x (because of tolerance, it might not be in range)
    if (x < xmin)
      {
      x = xmin;
      }
    else if (x > xmax)
      {
      x = xmax;
      }

    raster[y - subextentY[0]].push_back(x);
    }
}

//----------------------------------------------------------------------------
// Write the generated raster into the stencil when the raster cuts
// across the stencil. This is not an efficient stencil orientation.
static void vtkLassooStencilSourceCrosscutStencil(
  vtkImageStencilData *data, const int extent[6], const int subextent[6],
  vtkstd::deque< vtkstd::vector<double> > &raster)
{
  int r1 = extent[0];
  int r2 = extent[1];
  int zmin = subextent[4];
  int zmax = subextent[5];

  for (int idZ = zmin; idZ <= zmax; idZ++)
    {
    vtkstd::vector<double> &rline = raster[idZ - zmin];

    // sort the positions where line segments intersected raster lines
    vtkstd::sort(rline.begin(), rline.end());

    int lasts = VTK_INT_MIN;

    size_t l = rline.size();
    l = l - (l & 1); // force l to be an even number
    for (size_t k = 0; k < l; k += 2)
      {
      double y1 = rline[k] - VTK_STENCIL_TOL;
      double y2 = rline[k+1] + VTK_STENCIL_TOL;

      int s1 = vtkMath::Floor(y1) + 1;
      int s2 = vtkMath::Floor(y2);

      // ensure no overlap occurs with previous
      if (s1 <= lasts)
        {
        s1 = lasts + 1;
        }
      lasts = s2;

      for (int idY = s1; idY <= s2; idY++)
        {
        data->InsertNextExtent(r1, r2, idY, idZ);
        }
      }
    }
}

//----------------------------------------------------------------------------
// Write the generated raster into the stencil
static void vtkLassooStencilSourceGenerateStencil(
  vtkImageStencilData *data, int xj, int yj,
  const int extent[6], const int subextent[6],
  vtkstd::deque< vtkstd::vector<double> > &raster)
{
  if (xj != 0)
    {
    vtkLassooStencilSourceCrosscutStencil(
      data, extent, subextent, raster);
    return;
    }

  int zj = 3 - yj;
  int xmin = extent[0];
  int xmax = extent[1];
  int ymin = subextent[2*yj];
  int ymax = subextent[2*yj+1];
  int zmin = extent[2*zj];
  int zmax = extent[2*zj+1];

  // convert each raster line into extents for the stencil
  for (int idY = ymin; idY <= ymax; idY++)
    {
    vtkstd::vector<double> &rline = raster[idY - ymin];

    // sort the positions where line segments intersected raster lines
    vtkstd::sort(rline.begin(), rline.end());

    int lastr = VTK_INT_MIN;

    // go through each raster line and fill the stencil
    size_t l = rline.size();
    l = l - (l & 1); // force l to be an even number
    for (size_t k = 0; k < l; k += 2)
      {
      int yz[2];

      yz[yj-1] = idY;
      yz[zj-1] = zmin;

      double x1 = rline[k] - VTK_STENCIL_TOL;
      double x2 = rline[k+1] + VTK_STENCIL_TOL;

      int r1 = vtkMath::Floor(x1) + 1;
      int r2 = vtkMath::Floor(x2);

      // ensure no overlap occurs between extents
      if (r1 <= lastr)
        {
        r1 = lastr + 1;
        }
      lastr = r2;

      if (r2 >= r1)
        {
        data->InsertNextExtent(r1, r2, yz[0], yz[1]);
        }
      }
    }

  // copy the result to all other slices
  if (zmin < zmax)
    {
    for (int idY = ymin; idY <= ymax; idY++)
      {
      int r1, r2;
      int yz[2];

      yz[yj-1] = idY;
      yz[zj-1] = zmin;

      int iter = 0;
      while (data->GetNextExtent(r1, r2, xmin, xmax, yz[0], yz[1], iter))
        {
        for (int idZ = zmin + 1; idZ <= zmax; idZ++)
          {
          yz[zj-1] = idZ;
          data->InsertNextExtent(r1, r2, yz[0], yz[1]);
          }
        yz[zj-1] = zmin;
        }
      }
    }
}


//----------------------------------------------------------------------------
// Rasterize a polygon into the stencil
static int vtkLassooStencilSourcePolygon(
  vtkPoints *points, vtkImageStencilData *data, int xj, int yj,
  const int extent[6], const double origin[3], const double spacing[3])
{
  // get the bounds of the polygon
  int subextent[6];
  vtkLassooStencilSourceSubExtent(points, origin, spacing, extent, subextent);

  // create a vector for each raster line in the Y extent
  int nraster = subextent[2*yj+1] - subextent[2*yj] + 1;
  vtkstd::deque< vtkstd::vector<double> > raster(nraster);

  // rasterize each line
  vtkIdType n = points->GetNumberOfPoints();
  double p[3];
  double p0[2], p1[2], p2[2], p3[2];

  points->GetPoint(n-1, p);
  p0[0] = (p[xj] - origin[xj])/spacing[xj];
  p0[1] = (p[yj] - origin[yj])/spacing[yj];

  points->GetPoint(0, p);
  p1[0] = (p[xj] - origin[xj])/spacing[xj];
  p1[1] = (p[yj] - origin[yj])/spacing[yj];

  double dx = p1[0] - p0[0];
  double dy = p1[1] - p0[1];
  if (dx*dx + dy*dy <= VTK_STENCIL_TOL*VTK_STENCIL_TOL)
    {
    n -= 1;
    points->GetPoint(n-1, p);
    p0[0] = (p[xj] - origin[xj])/spacing[xj];
    p0[1] = (p[yj] - origin[yj])/spacing[yj];
    }

  points->GetPoint(1, p);
  p2[0] = (p[xj] - origin[xj])/spacing[xj];
  p2[1] = (p[yj] - origin[yj])/spacing[yj];

  // inflection means the line changes vertical direction
  bool inflection1, inflection2;
  inflection1 = ( (p1[1] - p0[1])*(p2[1] - p1[1]) <= 0 );

  for (vtkIdType i = 0; i < n; i++)
    {
    points->GetPoint((i+2)%n, p);
    p3[0] = (p[xj] - origin[xj])/spacing[xj];
    p3[1] = (p[yj] - origin[yj])/spacing[yj];

    inflection2 = ( (p2[1] - p1[1])*(p3[1] - p2[1]) <= 0 );

    vtkLassooStencilSourceBresenham(p1, p2, inflection1, inflection2,
                                    &extent[2*yj], &subextent[2*yj],
                                    raster);

    p0[0] = p1[0]; p0[1] = p1[1];
    p1[0] = p2[0]; p1[1] = p2[1];
    p2[0] = p3[0]; p2[1] = p3[1];
    inflection1 = inflection2;
    }

  vtkLassooStencilSourceGenerateStencil(
    data, xj, yj, extent, subextent, raster);

  return 1;
}


//----------------------------------------------------------------------------
// Generate the splines for the given set of points.  The splines
// will be closed if the final point is equal to the first point.
// The parametric value for the resulting spline will be valid over
// the range [0, tmax] where the tmax value is returned by reference.
static void vtkLassooStencilSourceCreateSpline(vtkPoints *points,
  int xj, int yj, const double origin[3], const double spacing[3],
  vtkSpline *xspline, vtkSpline *yspline, double &tmax, double &dmax)
{
  // initialize the spline
  xspline->RemoveAllPoints();
  yspline->RemoveAllPoints();
  xspline->ClosedOff();
  yspline->ClosedOff();

  // get the number of points and the first/last point
  vtkIdType n = points->GetNumberOfPoints();
  double p[3];
  double p0[2], p1[2];

  points->GetPoint(n-1, p);
  p0[0] = (p[xj] - origin[xj])/spacing[xj];
  p0[1] = (p[yj] - origin[yj])/spacing[yj];

  points->GetPoint(0, p);
  p1[0] = (p[xj] - origin[xj])/spacing[xj];
  p1[1] = (p[yj] - origin[yj])/spacing[yj];

  // factor between real distance and parametric distance
  double f = 1.0;
  // the length of the implicit segment for closed loops
  double lastd = 0;

  // aspect ratio
  double xf = 1.0;
  double yf = 1.0;
  if (spacing[xj] > spacing[yj])
    {
    xf = spacing[xj]/spacing[yj];
    }
  else
    {
    yf = spacing[yj]/spacing[xj];
    }

  // if first and last point are same, spline is closed
  double dx = (p1[0] - p0[0])*xf;
  double dy = (p1[1] - p0[1])*yf;
  double d2 = dx*dx + dy*dy;
  while (d2 <= VTK_STENCIL_TOL*VTK_STENCIL_TOL && n > 1)
    {
    n -= 1;
    points->GetPoint(n-1, p);
    p0[0] = (p[xj] - origin[xj])/spacing[xj];
    p0[1] = (p[yj] - origin[yj])/spacing[yj];

    xspline->ClosedOn();
    yspline->ClosedOn();

    // vtkSpline considers the parametric length of the implicit
    // segment of closed loops to be unity, so set "f" so that
    // multiplying the real length of that segment by "f" gives unity.
    dx = (p1[0] - p0[0])*xf;
    dy = (p1[1] - p0[1])*yf;
    d2 = dx*dx + dy*dy;
    lastd = sqrt(d2);
    if (lastd > 0)
      {
      f = 1.0/lastd;
      }
    }

  // Add all the points to the spline.
  double d = 0.0;
  for (vtkIdType i = 0; i < n; i++)
    {
    p0[0] = p1[0]; p0[1] = p1[1];

    points->GetPoint(i, p);
    p1[0] = (p[xj] - origin[xj])/spacing[xj];
    p1[1] = (p[yj] - origin[yj])/spacing[yj];

    dx = (p1[0] - p0[0])*xf;
    dy = (p1[1] - p0[1])*yf;

    d += sqrt(dx*dx + dy*dy);

    double t = f*d;

    xspline->AddPoint(t, p1[0]);
    yspline->AddPoint(t, p1[1]);
    }

  // Do the spline precomputations
  xspline->Compute();
  yspline->Compute();

  // The spline is valid over t = [0, tmax]
  d += lastd;
  tmax = f*d;
  dmax = d;
}

//----------------------------------------------------------------------------
// Rasterize a spline contour into the stencil
static int vtkLassooStencilSourceSpline(
  vtkPoints *points, vtkImageStencilData *data, int xj, int yj,
  const int extent[6], const double origin[3], const double spacing[3],
  vtkSpline *xspline, vtkSpline *yspline)
{
  // create the splines
  double tmax, dmax;
  vtkLassooStencilSourceCreateSpline(
    points, xj, yj, origin, spacing, xspline, yspline, tmax, dmax);

  if (dmax <= VTK_STENCIL_TOL)
    {
    return 1;
    }

  // get the bounds of the polygon as a first guess of the spline bounds
  int subextent[6];
  vtkLassooStencilSourceSubExtent(points, origin, spacing, extent, subextent);

  // create a vector for each raster line in the Y extent
  int nraster = subextent[2*yj+1] - subextent[2*yj] + 1;
  vtkstd::deque< vtkstd::vector<double> > raster(nraster);

  // go around the spline
  vtkIdType n = vtkMath::Floor(dmax)+1;
  double delta = tmax/n;

  double p0[2], p1[2], p2[2], p3[2];

  double t = tmax;
  if (xspline->GetClosed())
    {
    t = (n-1)*tmax/n;
    }
  else
    {
    n = n + 1;
    }

  p0[0] = xspline->Evaluate(t);
  p0[1] = yspline->Evaluate(t);

  t = 0;
  p1[0] = xspline->Evaluate(t);
  p1[1] = yspline->Evaluate(t);

  t = delta;
  p2[0] = xspline->Evaluate(t);
  p2[1] = yspline->Evaluate(t);

  // inflection means the line changes vertical direction
  bool inflection1, inflection2;
  inflection1 = ( (p1[1] - p0[1])*(p2[1] - p1[1]) <= 0 );

  for (vtkIdType i = 0; i < n; i++)
    {
    t += delta;
    if (i == n-2)
      {
      t = 0;
      }

    p3[0] = xspline->Evaluate(t);
    p3[1] = yspline->Evaluate(t);

    inflection2 = ( (p2[1] - p1[1])*(p3[1] - p2[1]) <= 0 );

    vtkLassooStencilSourceBresenham(p1, p2, inflection1, inflection2,
                                    &extent[2*yj], &subextent[2*yj],
                                    raster);

    p0[0] = p1[0]; p0[1] = p1[1];
    p1[0] = p2[0]; p1[1] = p2[1];
    p2[0] = p3[0]; p2[1] = p3[1];
    inflection1 = inflection2;
    }

  vtkLassooStencilSourceGenerateStencil(
    data, xj, yj, extent, subextent, raster);

  return 1;
}

//----------------------------------------------------------------------------
static int vtkLassooStencilSourceExecute(
  vtkPoints *points, vtkImageStencilData *data, int xj, int yj,
  int extent[6], double origin[3], double spacing[3], int shape,
  vtkSpline *xspline, vtkSpline *yspline)
{
  int result = 1;

  if (points == 0 || points->GetNumberOfPoints() < 3)
    {
    return 1;
    }

  switch (shape)
    {
    case vtkLassooStencilSource::POLYGON:
      result = vtkLassooStencilSourcePolygon(
        points, data, xj, yj, extent, origin, spacing);
      break;
    case vtkLassooStencilSource::SPLINE:
      result = vtkLassooStencilSourceSpline(
        points, data, xj, yj, extent, origin, spacing,
        xspline, yspline);
      break;
    }

  return result;
}


//----------------------------------------------------------------------------
int vtkLassooStencilSource::RequestData(
  vtkInformation *request,
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  int extent[6];
  double origin[3];
  double spacing[3];
  int result = 1;

  this->Superclass::RequestData(request, inputVector, outputVector);

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageStencilData *data = vtkImageStencilData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent);
  outInfo->Get(vtkDataObject::ORIGIN(), origin);
  outInfo->Get(vtkDataObject::SPACING(), spacing);

  int slabExtent[6];
  slabExtent[0] = extent[0]; slabExtent[1] = extent[1];
  slabExtent[2] = extent[2]; slabExtent[3] = extent[3];
  slabExtent[4] = extent[4]; slabExtent[5] = extent[5];

  int xj = 0;
  int yj = 1;
  int zj = 2;

  if (this->SliceOrientation == 0)
    {
    xj = 1;
    yj = 2;
    zj = 0;
    }
  else if (this->SliceOrientation == 1)
    {
    xj = 0;
    yj = 2;
    zj = 1;
    }

  int zmin = extent[2*zj];
  int zmax = extent[2*zj+1];

  vtkLSSPointMap::iterator iter = this->PointMap->lower_bound(zmin);
  vtkLSSPointMap::iterator maxiter = this->PointMap->upper_bound(zmax);

  while (iter != maxiter && result != 0)
    {
    this->SetProgress((slabExtent[2*zj] - zmin)*1.0/(zmax - zmin + 1));

    int i = iter->first;
    vtkPoints *points = iter->second;

    // fill in the slices with no SlicePoints
    if (this->Points && i > slabExtent[2*zj])
      {
      slabExtent[2*zj+1] = i-1;

      result = vtkLassooStencilSourceExecute(
        this->Points, data, xj, yj, slabExtent, origin, spacing,
        this->Shape, this->SplineX, this->SplineY);
      }

    // do the slice with its SlicePoints
    if (result)
      {
      slabExtent[2*zj] = i;
      slabExtent[2*zj+1] = i;

      result = vtkLassooStencilSourceExecute(
        points, data, xj, yj, slabExtent, origin, spacing,
        this->Shape, this->SplineX, this->SplineY);

      slabExtent[2*zj] = slabExtent[2*zj+1] + 1;
      }

    ++iter;
    }

  this->SetProgress((slabExtent[2*zj] - zmin)*1.0/(zmax - zmin + 1));

  // fill in the rest
  if (result && slabExtent[2*zj] <= zmax)
    {
    slabExtent[2*zj+1] = zmax;

    result = vtkLassooStencilSourceExecute(
      this->Points, data, xj, yj, slabExtent, origin, spacing,
      this->Shape, this->SplineX, this->SplineY);

    this->SetProgress(1.0);
    }

  return result;
}

//----------------------------------------------------------------------------
int vtkLassooStencilSource::RequestInformation(
  vtkInformation *,
  vtkInformationVector **,
  vtkInformationVector *outputVector)
{
  int wholeExtent[6];
  double spacing[3];
  double origin[3];

  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  for (int i = 0; i < 3; i++)
    {
    wholeExtent[2*i] = this->OutputWholeExtent[2*i];
    wholeExtent[2*i+1] = this->OutputWholeExtent[2*i+1];
    spacing[i] = this->OutputSpacing[i];
    origin[i] = this->OutputOrigin[i];
    }

  // If InformationInput is set, then get the spacing,
  // origin, and whole extent from it.
  if (this->InformationInput)
    {
    this->InformationInput->UpdateInformation();
    this->InformationInput->GetWholeExtent(wholeExtent);
    this->InformationInput->GetSpacing(spacing);
    this->InformationInput->GetOrigin(origin);
    }

  outInfo->Set(vtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
               wholeExtent, 6);
  outInfo->Set(vtkDataObject::SPACING(), spacing, 3);
  outInfo->Set(vtkDataObject::ORIGIN(), origin, 3);

  return 1;
}
