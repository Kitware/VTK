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

#include <math.h>
#include <vtkstd/vector>
#include <vtkstd/deque>
#include <vtkstd/algorithm>

vtkStandardNewMacro(vtkLassooStencilSource);
vtkCxxSetObjectMacro(vtkLassooStencilSource, InformationInput, vtkImageData);
vtkCxxSetObjectMacro(vtkLassooStencilSource, Points, vtkPoints);

//----------------------------------------------------------------------------
vtkLassooStencilSource::vtkLassooStencilSource()
{
  this->SetNumberOfInputPorts(0);

  this->Shape = vtkLassooStencilSource::POLYGON;
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

  return mTime;
}


//----------------------------------------------------------------------------
// tolerance for stencil operations

#define VTK_STENCIL_TOL 7.62939453125e-06

//----------------------------------------------------------------------------
// Compute a reduced extent based on the bounds of the shape.
static void vtkLassooStencilSourceSubExtent(
  vtkLassooStencilSource *self,
  const double origin[3], const double spacing[3],
  const int extent[6], int subextent[6])
{
  double bounds[6];
  self->GetPoints()->GetBounds(bounds);

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
  const int extent[6], int subextent[6],
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

  // Guard against extent
  if (iy1 < extent[2])
    {
    iy1 = extent[2];
    }
  if (iy2 > extent[3])
    {
    iy2 = extent[3];
    }

  // Expand subextent if necessary
  while (iy1 < subextent[2])
    {
    raster.push_front(vtkstd::vector<double>());
    --(subextent[2]);
    }
  while (iy2 > subextent[3])
    {
    raster.push_back(vtkstd::vector<double>());
    ++(subextent[3]);
    }

  // Precompute values for a Bresenham-like line algorithm
  double grad = (x2 - x1)/(y2 - y1);
  double delta = (iy1 - y1)*grad;

  // Go along y and place each x in the proper raster line
  for (int y = iy1; y <= iy2; y++)
    {
    delta += grad;
    double x = x1 + delta;
    // clamp x (because of tolerance, it might not be in range)
    if (x < xmin)
      {
      x = xmin;
      }
    else if (x > xmax)
      {
      x = xmax;
      }

    raster[y - subextent[2]].push_back(x);
    }
}

//----------------------------------------------------------------------------
// Write the generated raster into the stencil
static void vtkLassooStencilSourceGenerateStencil(
  vtkImageStencilData *data, const int extent[6], const int subextent[6],
  vtkstd::deque< vtkstd::vector<double> > &raster)
{
  // convert each raster line into extents for the stencil
  for (int idY = subextent[2]; idY <= subextent[3]; idY++)
    {
    vtkstd::vector<double> &rline = raster[idY - subextent[2]];

    // sort the positions where line segments intersected raster lines
    vtkstd::sort(rline.begin(), rline.end());

    int lastr = VTK_INT_MIN;

    // go through each raster line and fill the stencil
    size_t l = rline.size();
    l = l - (l & 1); // force l to be an even number
    for (size_t k = 0; k < l; k += 2)
      {
      double x1 = rline[k] - VTK_STENCIL_TOL;
      double x2 = rline[k+1] + VTK_STENCIL_TOL;

      int r1 = vtkMath::Floor(x1);
      int r2 = vtkMath::Floor(x2) + 1;

      // ensure no overlap occurs between extents
      if (r1 <= lastr)
        {
        r1 = lastr + 1;
        }
      lastr = r2;

      if (r2 >= r1)
        {
        data->InsertNextExtent(r1, r2, idY, extent[4]);
        }
      }
    }

  // copy the result to all other slices
  for (int idZ = extent[4] + 1; idZ <= extent[5]; idZ++)
    {
    for (int idY = subextent[2]; idY <= subextent[3]; idY++)
      {
      int iter = 0;
      int r1, r2;

      data->GetNextExtent(r1, r2, extent[0], extent[1], idY, extent[4], iter);
      data->InsertNextExtent(r1, r2, idY, idZ);
      }
    }
}


//----------------------------------------------------------------------------
// Rasterize a polygon into the stencil
static int vtkLassooStencilSourcePolygon(
  vtkLassooStencilSource *self, vtkImageStencilData *data,
  const int extent[6], const double origin[3], const double spacing[3])
{
  self->UpdateProgress(0.0);

  // get the points
  vtkPoints *points = self->GetPoints();

  // get the bounds of the polygon
  int subextent[6];
  vtkLassooStencilSourceSubExtent(self, origin, spacing, extent, subextent);

  // create a vector for each raster line in the Y extent
  int nraster = subextent[3] - subextent[2] + 1;
  vtkstd::deque< vtkstd::vector<double> > raster(nraster);

  // rasterize each line
  vtkIdType n = points->GetNumberOfPoints();
  double p0[3], p1[3], p2[3], p3[3];

  points->GetPoint(n-1, p0);
  p0[0] = (p0[0] - origin[0])/spacing[0];
  p0[1] = (p0[1] - origin[1])/spacing[1];
  p0[2] = (p0[2] - origin[2])/spacing[2];

  points->GetPoint(0, p1);
  p1[0] = (p1[0] - origin[0])/spacing[0];
  p1[1] = (p1[1] - origin[1])/spacing[1];
  p1[2] = (p1[2] - origin[2])/spacing[2];

  if (vtkMath::Distance2BetweenPoints(p0, p1) <
      VTK_STENCIL_TOL*VTK_STENCIL_TOL)
    {
    n -= 1;
    points->GetPoint(n-1, p0);
    p0[0] = (p0[0] - origin[0])/spacing[0];
    p0[1] = (p0[1] - origin[1])/spacing[1];
    p0[2] = (p0[2] - origin[2])/spacing[2];
    }

  points->GetPoint(1, p2);
  p2[0] = (p2[0] - origin[0])/spacing[0];
  p2[1] = (p2[1] - origin[1])/spacing[1];
  p2[2] = (p2[2] - origin[2])/spacing[2];

  // inflection means the line changes vertical direction
  bool inflection1, inflection2;
  inflection1 = ( (p1[1] - p0[1])*(p2[1] - p1[1]) <= 0 );

  for (vtkIdType i = 0; i < n; i++)
    {
    points->GetPoint((i+2)%n, p3);
    p3[0] = (p3[0] - origin[0])/spacing[0];
    p3[1] = (p3[1] - origin[1])/spacing[1];
    p3[2] = (p3[2] - origin[2])/spacing[2];

    inflection2 = ( (p2[1] - p1[1])*(p3[1] - p2[1]) <= 0 );

    vtkLassooStencilSourceBresenham(p1, p2, inflection1, inflection2,
                                    extent, subextent, raster);

    p0[0] = p1[0]; p0[1] = p1[1]; p0[2] = p1[2];
    p1[0] = p2[0]; p1[1] = p2[1]; p1[2] = p2[2];
    p2[0] = p3[0]; p2[1] = p3[1]; p2[2] = p3[2];
    inflection1 = inflection2;
    }

  self->UpdateProgress(0.5);

  vtkLassooStencilSourceGenerateStencil(data, extent, subextent, raster);

  self->UpdateProgress(1.0);

  return 1;
}


//----------------------------------------------------------------------------
// Generate the splines for the given set of points.  The splines
// will be closed if the final point is equal to the first point.
// The parametric value for the resulting spline will be valid over
// the range [0, tmax] where the tmax value is returned by reference.
static void vtkLassooStencilSourceCreateSpline(vtkPoints *points,
  const double origin[3], const double spacing[3],
  vtkSpline *xspline, vtkSpline *yspline, double &tmax, double &dmax)
{
  // initialize the spline
  xspline->RemoveAllPoints();
  yspline->RemoveAllPoints();
  xspline->ClosedOff();
  yspline->ClosedOff();

  // get the number of points and the first/last point
  vtkIdType n = points->GetNumberOfPoints();
  double p0[3], p1[3];

  points->GetPoint(n-1, p0);
  p0[0] = (p0[0] - origin[0])/spacing[0];
  p0[1] = (p0[1] - origin[1])/spacing[1];
  p0[2] = (p0[2] - origin[2])/spacing[2];

  points->GetPoint(0, p1);
  p1[0] = (p1[0] - origin[0])/spacing[0];
  p1[1] = (p1[1] - origin[1])/spacing[1];
  p1[2] = (p1[2] - origin[2])/spacing[2];

  // factor between real distance and parametric distance
  double f = 1.0;
  // the length of the implicit segment for closed loops
  double lastd = 0;

  // if first and last point are same, spline is closed
  while ((vtkMath::Distance2BetweenPoints(p0, p1) <
          VTK_STENCIL_TOL*VTK_STENCIL_TOL) && n > 1)
    {
    n -= 1;
    points->GetPoint(n-1, p0);
    p0[0] = (p0[0] - origin[0])/spacing[0];
    p0[1] = (p0[1] - origin[1])/spacing[1];
    p0[2] = (p0[2] - origin[2])/spacing[2];

    xspline->ClosedOn();
    yspline->ClosedOn();

    // vtkSpline considers the parametric length of the implicit
    // segment of closed loops to be unity, so set "f" so that
    // multiplying the real length of that segment by "f" gives unity.
    lastd = sqrt(vtkMath::Distance2BetweenPoints(p0, p1));
    if (lastd > 0)
      {
      f = 1.0/lastd;
      }
    }

  // Add all the points to the spline.
  double d = 0.0;
  for (vtkIdType i = 0; i < n; i++)
    {
    p0[0] = p1[0]; p0[1] = p1[1]; p0[2] = p1[2];

    points->GetPoint(i, p1);
    p1[0] = (p1[0] - origin[0])/spacing[0];
    p1[1] = (p1[1] - origin[1])/spacing[1];
    p1[2] = (p1[2] - origin[2])/spacing[2];

    d += sqrt(vtkMath::Distance2BetweenPoints(p0, p1));

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
  vtkLassooStencilSource *self, vtkImageStencilData *data,
  const int extent[6], const double origin[3], const double spacing[3],
  vtkSpline *xspline, vtkSpline *yspline)
{
  self->UpdateProgress(0.0);

  // get the points, create the splines
  vtkPoints *points = self->GetPoints();
  double tmax, dmax;
  vtkLassooStencilSourceCreateSpline(
    points, origin, spacing, xspline, yspline, tmax, dmax);

  if (dmax <= VTK_STENCIL_TOL)
    {
    return 1;
    }

  // get the bounds of the polygon as a first guess of the spline bounds
  int subextent[6];
  vtkLassooStencilSourceSubExtent(self, origin, spacing, extent, subextent);

  // create a vector for each raster line in the Y extent
  int nraster = subextent[3] - subextent[2] + 1;
  vtkstd::deque< vtkstd::vector<double> > raster(nraster);

  // go around the spline
  vtkIdType n = vtkMath::Floor(dmax)+1;
  double delta = tmax/n;

  double p0[3], p1[3], p2[3], p3[3];

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
  p0[2] = 0;

  t = 0;
  p1[0] = xspline->Evaluate(t);
  p1[1] = yspline->Evaluate(t);
  p1[2] = 0;

  t = delta;
  p2[0] = xspline->Evaluate(t);
  p2[1] = yspline->Evaluate(t);
  p2[2] = 0;

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
    p3[2] = 0;

    inflection2 = ( (p2[1] - p1[1])*(p3[1] - p2[1]) <= 0 );

    vtkLassooStencilSourceBresenham(p1, p2, inflection1, inflection2,
                                    extent, subextent, raster);

    p0[0] = p1[0]; p0[1] = p1[1];
    p1[0] = p2[0]; p1[1] = p2[1];
    p2[0] = p3[0]; p2[1] = p3[1];
    inflection1 = inflection2;
    }

  self->UpdateProgress(0.5);

  vtkLassooStencilSourceGenerateStencil(data, extent, subextent, raster);

  self->UpdateProgress(1.0);

  return 1;
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

  if (this->Points == 0 || this->Points->GetNumberOfPoints() < 3)
    {
    return 1;
    }

  vtkInformation *outInfo = outputVector->GetInformationObject(0);
  vtkImageStencilData *data = vtkImageStencilData::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  outInfo->Get(vtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(), extent);
  outInfo->Get(vtkDataObject::ORIGIN(), origin);
  outInfo->Get(vtkDataObject::SPACING(), spacing);

  switch (this->Shape)
    {
    case vtkLassooStencilSource::POLYGON:
      result = vtkLassooStencilSourcePolygon(
        this, data, extent, origin, spacing);
      break;
    case vtkLassooStencilSource::SPLINE:
      result = vtkLassooStencilSourceSpline(
        this, data, extent, origin, spacing,
        this->SplineX, this->SplineY);
      break;
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
