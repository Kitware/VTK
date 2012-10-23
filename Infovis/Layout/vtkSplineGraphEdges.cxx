/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSplineGraphEdges.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "vtkSplineGraphEdges.h"

#include "vtkCardinalSpline.h"
#include "vtkCommand.h"
#include "vtkGraph.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

vtkStandardNewMacro(vtkSplineGraphEdges);
vtkCxxSetObjectMacro(vtkSplineGraphEdges, Spline, vtkSpline);

namespace {
// N-function defined at:
// http://mathworld.wolfram.com/B-Spline.html
// optimized for j = 3.
double CubicSpline(vtkIdType i, double* k, double t)
{
  if (t >= k[i] && t < k[i+1])
    {
    double denom = (k[i+3]-k[i])*(k[i+2]-k[i])*(k[i+1]-k[i]);
    if (denom == 0.0) return 0.0;
    double temp = t - k[i];
    return temp*temp*temp/denom;
    }
  if (t >= k[i+1] && t < k[i+2])
    {
    double denom1 = (k[i+3]-k[i])*(k[i+2]-k[i])*(k[i+2]-k[i+1]);
    double term1;
    if (denom1 == 0.0)
      {
      term1 = 0.0;
      }
    else
      {
      term1 = (t-k[i])*(t-k[i])*(k[i+2]-t)/denom1;
      }

    double denom2 = (k[i+3]-k[i])*(k[i+3]-k[i+1])*(k[i+2]-k[i+1]);
    double term2;
    if (denom2 == 0.0)
      {
      term2 = 0.0;
      }
    else
      {
      term2 = (t-k[i])*(k[i+3]-t)*(t-k[i+1])/denom2;
      }

    double denom3 = (k[i+4]-k[i+1])*(k[i+3]-k[i+1])*(k[i+2]-k[i+1]);
    double term3;
    if (denom3 == 0.0)
      {
      term3 = 0.0;
      }
    else
      {
      term3 = (k[i+4]-t)*(t-k[i+1])*(t-k[i+1])/denom3;
      }

    return term1 + term2 + term3;
    }
  if (t >= k[i+2] && t < k[i+3])
    {
    double denom1 = (k[i+3]-k[i])*(k[i+3]-k[i+1])*(k[i+3]-k[i+2]);
    double term1;
    if (denom1 == 0.0)
      {
      term1 = 0.0;
      }
    else
      {
      term1 = (t-k[i])*(k[i+3]-t)*(k[i+3]-t)/denom1;
      }

    double denom2 = (k[i+4]-k[i+1])*(k[i+3]-k[i+1])*(k[i+3]-k[i+2]);
    double term2;
    if (denom2 == 0.0)
      {
      term2 = 0.0;
      }
    else
      {
      term2 = (k[i+4]-t)*(t-k[i+1])*(k[i+3]-t)/denom2;
      }

    double denom3 = (k[i+4]-k[i+1])*(k[i+4]-k[i+2])*(k[i+3]-k[i+2]);
    double term3;
    if (denom3 == 0.0)
      {
      term3 = 0.0;
      }
    else
      {
      term3 = (k[i+4]-t)*(k[i+4]-t)*(t-k[i+2])/denom3;
      }

    return term1 + term2 + term3;
    }
  if (t >= k[i+3] && t < k[i+4])
    {
    double denom = (k[i+4]-k[i+1])*(k[i+4]-k[i+2])*(k[i+4]-k[i+3]);
    if (denom == 0.0) return 0.0;
    double temp = k[i+4] - t;
    return temp*temp*temp/denom;
    }
  return 0.0;
}

// Slow, recursive version of N-function defined:
// http://mathworld.wolfram.com/B-Spline.html
#if 0
double N(vtkIdType i, vtkIdType j, double* k, double t)
{
  if (j < 0)
    {
    return 0.0;
    }
  if (j == 0)
    {
    if (t >= k[i] && t < k[i+1] && k[i] < k[i+1])
      {
      return 1.0;
      }
    return 0.0;
    }
  double term1 = 0.0;
  if (k[i] < k[i+j])
    {
    term1 = (t-k[i])/(k[i+j]-k[i])*N(i, j-1, k, t);
    }
  double term2 = 0.0;
  if (k[i+1] < k[i+j+1])
    {
    term2 = (k[i+j+1]-t)/(k[i+j+1]-k[i+1])*N(i+1, j-1, k, t);
    }
  return term1 + term2;
}
#endif

#if 0
double BCubic(vtkIdType i, double* k, double t)
{
  if (t < k[i-2] || t >= k[i+2])
    {
    return 0.0;
    }
  double temp;
  if (t >= k[i-2] && t < k[i-1])
    {
    temp = 2.0 + t;
    return temp*temp*temp/6.0;
    }
  if (t >= k[i-1] && t < k[i])
    {
    return (4 - 6*t*t - 3*t*t*t)/6.0;
    }
  if (t >= k[i] && t < k[i+1])
    {
    return (4 - 6*t*t + 3*t*t*t)/6.0;
    }
  temp = 2.0 - t;
  return temp*temp*temp;
}
#endif
}

vtkSplineGraphEdges::vtkSplineGraphEdges()
{
  this->Spline = vtkCardinalSpline::New();
  this->XSpline = 0;
  this->YSpline = 0;
  this->ZSpline = 0;
  this->NumberOfSubdivisions = 20;
  this->SplineType = CUSTOM;
}

vtkSplineGraphEdges::~vtkSplineGraphEdges()
{
  if (this->Spline)
    {
    this->Spline->Delete();
    this->Spline = 0;
    }
}

unsigned long vtkSplineGraphEdges::GetMTime()
{
  unsigned long mtime = this->Superclass::GetMTime();
  if (this->Spline && this->Spline->GetMTime() > mtime)
    {
    mtime = this->Spline->GetMTime();
    }
  return mtime;
}

int vtkSplineGraphEdges::RequestData(
  vtkInformation *vtkNotUsed(request),
  vtkInformationVector **inputVector,
  vtkInformationVector *outputVector)
{
  if (!this->Spline)
    {
    vtkErrorMacro("Must have a valid spline.");
    return 0;
    }

  // get the info objects
  vtkInformation *inInfo = inputVector[0]->GetInformationObject(0);
  vtkInformation *outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  vtkGraph *input = vtkGraph::SafeDownCast(
    inInfo->Get(vtkDataObject::DATA_OBJECT()));
  vtkGraph *output = vtkGraph::SafeDownCast(
    outInfo->Get(vtkDataObject::DATA_OBJECT()));

  output->ShallowCopy(input);
  output->DeepCopyEdgePoints(input);

  if (this->SplineType == CUSTOM)
    {
    this->XSpline.TakeReference(this->Spline->NewInstance());
    this->XSpline->DeepCopy(this->Spline);
    this->YSpline.TakeReference(this->Spline->NewInstance());
    this->YSpline->DeepCopy(this->Spline);
    this->ZSpline.TakeReference(this->Spline->NewInstance());
    this->ZSpline->DeepCopy(this->Spline);
    }

  for (vtkIdType i = 0; i < output->GetNumberOfEdges(); ++i)
    {
    if (this->SplineType == BSPLINE)
      {
      this->GenerateBSpline(output, i);
      }
    else
      {
      this->GeneratePoints(output, i);
      }
    if (i % 1000 == 0)
      {
      double progress = static_cast<double>(i)/
        static_cast<double>(output->GetNumberOfEdges());
      this->InvokeEvent(vtkCommand::ProgressEvent, &progress);
      }
    }

  return 1;
}

void vtkSplineGraphEdges::GeneratePoints(vtkGraph* g, vtkIdType e)
{
  // Initialize the splines
  this->XSpline->RemoveAllPoints();
  this->YSpline->RemoveAllPoints();
  this->ZSpline->RemoveAllPoints();

  vtkIdType numInternalPoints;
  double* internalPoints;
  g->GetEdgePoints(e, numInternalPoints, internalPoints);

  vtkIdType numPoints = numInternalPoints + 2;
  double* points = new double[3*static_cast<size_t>(numPoints)];
  memcpy(points + 3, internalPoints, sizeof(double)*3
         *static_cast<size_t>(numInternalPoints));
  g->GetPoint(g->GetSourceVertex(e), points);
  g->GetPoint(g->GetTargetVertex(e), points + 3*(numInternalPoints+1));

  double* xPrev;
  double* x;
  double length = 0.0;
  double* xEnd = points + 3*numPoints;
  for (xPrev = points, x = points+3; x != xEnd; xPrev += 3, x += 3)
    {
    double len = sqrt(vtkMath::Distance2BetweenPoints(x,xPrev));
    length += len;
    }

  if (length <= 0.0)
    {
    return;
    }

  // Now we insert points into the splines with the parametric coordinate
  // based on length. We keep track of the parametric coordinates
  // of the points for later point interpolation.
  this->XSpline->AddPoint(0.0, points[0]);
  this->YSpline->AddPoint(0.0, points[1]);
  this->ZSpline->AddPoint(0.0, points[2]);
  double len = 0.0;
  for (xPrev = points, x = points+3; x != xEnd; xPrev += 3, x += 3)
    {
    double dist = sqrt(vtkMath::Distance2BetweenPoints(x, xPrev));
    if (dist == 0)
      {
      continue;
      }
    len += dist;
    double t = len/length;

    this->XSpline->AddPoint(t, x[0]);
    this->YSpline->AddPoint(t, x[1]);
    this->ZSpline->AddPoint(t, x[2]);
    }

  // Now compute the new points
  vtkIdType numNewPoints = this->NumberOfSubdivisions - 1;
  double* newPoints = new double[3*numNewPoints];
  vtkIdType i;
  for (i = 0, x = newPoints; i < numNewPoints; i++, x += 3)
    {
    double t = static_cast<double>(i+1) /
      static_cast<double>(this->NumberOfSubdivisions);
    x[0] = this->XSpline->Evaluate(t);
    x[1] = this->YSpline->Evaluate(t);
    x[2] = this->ZSpline->Evaluate(t);
    }
  g->SetEdgePoints(e, numNewPoints, newPoints);

  delete [] points;
  delete [] newPoints;
}

void vtkSplineGraphEdges::GenerateBSpline(vtkGraph* g, vtkIdType e)
{
  vtkIdType numInternalPoints;
  double* internalPoints;
  g->GetEdgePoints(e, numInternalPoints, internalPoints);

  // Duplicate internal point if there is just one, so there are at least
  // four points, required for B-spline.
  bool repeat = false;
  if (numInternalPoints == 1)
    {
    repeat = true;
    numInternalPoints = 2;
    }
  vtkIdType numPoints = numInternalPoints + 2;
  double* points = new double[3*numPoints];
  if (repeat)
    {
    memcpy(points + 3, internalPoints, sizeof(double)*3);
    memcpy(points + 6, internalPoints, sizeof(double)*3);
    }
  else
    {
    memcpy(points + 3, internalPoints, sizeof(double)*3
           *static_cast<size_t>(numInternalPoints));
    }
  g->GetPoint(g->GetSourceVertex(e), points);
  g->GetPoint(g->GetTargetVertex(e), points + 3*(numInternalPoints+1));

  if (numPoints <= 3)
    {
    return;
    }

  // Compute the knot vector
  vtkIdType numKnots = numPoints + 4;
  double* knots = new double[numKnots];
  knots[0] = 0.0;
  knots[1] = 0.0;
  knots[2] = 0.0;
  knots[3] = 0.0;
  knots[numKnots-4] = 1.0;
  knots[numKnots-3] = 1.0;
  knots[numKnots-2] = 1.0;
  knots[numKnots-1] = 1.0;

  vtkIdType i;
  for (i = 4; i < numKnots-4; ++i)
    {
    knots[i] = static_cast<double>(i-3)/static_cast<double>(numKnots-7);
    }

  // Special case of 3 points, make symmetric
  if (numPoints == 3)
    {
    knots[3] = 0.5;
    }

  // Now compute the new points
  vtkIdType numNewPoints = this->NumberOfSubdivisions - 1;
  double* newPoints = new double[3*numNewPoints];
  double* xNew;
  for (i = 0, xNew = newPoints; i < numNewPoints; i++, xNew += 3)
    {
    xNew[0] = 0.0;
    xNew[1] = 0.0;
    xNew[2] = 0.0;
    double t = static_cast<double>(i+1) /
      static_cast<double>(this->NumberOfSubdivisions);
    double* x = points;
    //double bsum = 0.0;
    for (vtkIdType j = 0; j < numPoints; ++j, x += 3)
      {
      //double b = BCubic(j+2, knots, t);
      //double b = N(j, 3, knots, t);
      double b = CubicSpline(j, knots, t);
      //bsum += b;
      xNew[0] += x[0]*b;
      xNew[1] += x[1]*b;
      xNew[2] += x[2]*b;
      }
    //cerr << "bsum: " << bsum << endl;
    }
  g->SetEdgePoints(e, numNewPoints, newPoints);

  delete [] points;
  delete [] knots;
  delete [] newPoints;
}

void vtkSplineGraphEdges::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
  os << indent << "SplineType: " << this->SplineType << endl;
  os << indent << "NumberOfSubdivisions: " << this->NumberOfSubdivisions << endl;
  os << indent << "Spline: " << (this->Spline ? "" : "(none)") << endl;
  if (this->Spline)
    {
    this->Spline->PrintSelf(os, indent.GetNextIndent());
    }
}
