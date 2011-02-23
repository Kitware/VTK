/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLassoStencilSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkLassoStencilSource.h"

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

vtkStandardNewMacro(vtkLassoStencilSource);
vtkCxxSetObjectMacro(vtkLassoStencilSource, Points, vtkPoints);

//----------------------------------------------------------------------------
class vtkLSSPointMap : public vtkstd::map<int, vtkSmartPointer<vtkPoints> >
{
};

//----------------------------------------------------------------------------
vtkLassoStencilSource::vtkLassoStencilSource()
{
  this->SetNumberOfInputPorts(0);

  this->Shape = vtkLassoStencilSource::POLYGON;
  this->SliceOrientation = 2;
  this->Points = NULL;
  this->SplineX = vtkCardinalSpline::New();
  this->SplineY = vtkCardinalSpline::New();

  this->PointMap = new vtkLSSPointMap();
}

//----------------------------------------------------------------------------
vtkLassoStencilSource::~vtkLassoStencilSource()
{
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
void vtkLassoStencilSource::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Shape: " << this->GetShapeAsString() << "\n";
  os << indent << "Points: " << this->Points << "\n";
  os << indent << "SliceOrientation: " << this->GetSliceOrientation() << "\n";
  os << indent << "SlicePoints: " << this->PointMap->size() << "\n";
}

//----------------------------------------------------------------------------
const char *vtkLassoStencilSource::GetShapeAsString()
{
  switch (this->Shape)
    {
    case vtkLassoStencilSource::POLYGON:
      return "Polygon";
    case vtkLassoStencilSource::SPLINE:
      return "Spline";
    }
  return "";
}

//----------------------------------------------------------------------------
unsigned long int vtkLassoStencilSource::GetMTime()
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
void vtkLassoStencilSource::SetSlicePoints(int i, vtkPoints *points)
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
void vtkLassoStencilSource::RemoveAllSlicePoints()
{
  this->PointMap->clear();
}

//----------------------------------------------------------------------------
vtkPoints *vtkLassoStencilSource::GetSlicePoints(int i)
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
static void vtkLassoStencilSourceSubExtent(
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
// Rasterize a polygon into the stencil
static int vtkLassoStencilSourcePolygon(
  vtkPoints *points, vtkImageStencilData *data, vtkImageStencilRaster *raster,
  const int extent[6], const double origin[3], const double spacing[3],
  int xj, int yj)
{
  // get the bounds of the polygon
  int subextent[6];
  vtkLassoStencilSourceSubExtent(points, origin, spacing, extent, subextent);

  // allocate the raster
  raster->PrepareForNewData(&subextent[2*yj]);

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

    raster->InsertLine(p1, p2, inflection1, inflection2);

    p0[0] = p1[0]; p0[1] = p1[1];
    p1[0] = p2[0]; p1[1] = p2[1];
    p2[0] = p3[0]; p2[1] = p3[1];
    inflection1 = inflection2;
    }

  raster->FillStencilData(data, extent, xj, yj);

  return 1;
}


//----------------------------------------------------------------------------
// Generate the splines for the given set of points.  The splines
// will be closed if the final point is equal to the first point.
// The parametric value for the resulting spline will be valid over
// the range [0, tmax] where the tmax value is returned by reference.
static void vtkLassoStencilSourceCreateSpline(vtkPoints *points,
  const double origin[3], const double spacing[3],
  int xj, int yj, vtkSpline *xspline, vtkSpline *yspline,
  double &tmax, double &dmax)
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
static int vtkLassoStencilSourceSpline(
  vtkPoints *points, vtkImageStencilData *data, vtkImageStencilRaster *raster,
  const int extent[6], const double origin[3], const double spacing[3],
  int xj, int yj, vtkSpline *xspline, vtkSpline *yspline)
{
  // create the splines
  double tmax, dmax;
  vtkLassoStencilSourceCreateSpline(
    points, origin, spacing, xj, yj, xspline, yspline, tmax, dmax);

  if (dmax <= VTK_STENCIL_TOL)
    {
    return 1;
    }

  // get the bounds of the polygon as a first guess of the spline bounds
  int subextent[6];
  vtkLassoStencilSourceSubExtent(points, origin, spacing, extent, subextent);

  // allocate the raster
  raster->PrepareForNewData(&subextent[2*yj]);

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

    raster->InsertLine(p1, p2, inflection1, inflection2);

    p0[0] = p1[0]; p0[1] = p1[1];
    p1[0] = p2[0]; p1[1] = p2[1];
    p2[0] = p3[0]; p2[1] = p3[1];
    inflection1 = inflection2;
    }

  raster->FillStencilData(data, extent, xj, yj);

  return 1;
}

//----------------------------------------------------------------------------
static int vtkLassoStencilSourceExecute(
  vtkPoints *points, vtkImageStencilData *data, vtkImageStencilRaster *raster,
  int extent[6], double origin[3], double spacing[3], int shape,
  int xj, int yj, vtkSpline *xspline, vtkSpline *yspline)
{
  int result = 1;

  if (points == 0 || points->GetNumberOfPoints() < 3)
    {
    return 1;
    }

  switch (shape)
    {
    case vtkLassoStencilSource::POLYGON:
      result = vtkLassoStencilSourcePolygon(
        points, data, raster, extent, origin, spacing, xj, yj);
      break;
    case vtkLassoStencilSource::SPLINE:
      result = vtkLassoStencilSourceSpline(
        points, data, raster, extent, origin, spacing, xj, yj,
        xspline, yspline);
      break;
    }

  return result;
}


//----------------------------------------------------------------------------
int vtkLassoStencilSource::RequestData(
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

  vtkImageStencilRaster raster(&extent[2*yj]);
  raster.SetTolerance(VTK_STENCIL_TOL);

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

      result = vtkLassoStencilSourceExecute(
        this->Points, data, &raster, slabExtent, origin, spacing,
        this->Shape, xj, yj, this->SplineX, this->SplineY);
      }

    // do the slice with its SlicePoints
    if (result)
      {
      slabExtent[2*zj] = i;
      slabExtent[2*zj+1] = i;

      result = vtkLassoStencilSourceExecute(
        points, data, &raster, slabExtent, origin, spacing,
        this->Shape, xj, yj, this->SplineX, this->SplineY);

      slabExtent[2*zj] = slabExtent[2*zj+1] + 1;
      }

    ++iter;
    }

  this->SetProgress((slabExtent[2*zj] - zmin)*1.0/(zmax - zmin + 1));

  // fill in the rest
  if (result && slabExtent[2*zj] <= zmax)
    {
    slabExtent[2*zj+1] = zmax;

    result = vtkLassoStencilSourceExecute(
      this->Points, data, &raster, slabExtent, origin, spacing,
      this->Shape, xj, yj, this->SplineX, this->SplineY);

    this->SetProgress(1.0);
    }

  return result;
}
