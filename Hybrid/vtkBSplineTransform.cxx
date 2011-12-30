/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkBSplineTransform.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBSplineTransform.h"

#include "vtkImageData.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"

#include <math.h>

vtkStandardNewMacro(vtkBSplineTransform);
vtkCxxSetObjectMacro(vtkBSplineTransform,Coefficients,vtkImageData);

namespace {

//-------------------------------------------------------------
// The b-spline provides continuity of the first and second
// derivatives with a piecewise cubic polynomial.  The polynomial
// does not pass through the knots.
inline void vtkBSplineTransformWeights(double F[4], double G[4], double f)
{
  const double sixth = 1.0/6.0;
  const double half = 0.5;

  double f2 = f*f;

  F[3] = f2*f*sixth;
  F[0] = (f2 - f)*half - F[3] + sixth;
  F[2] = f + F[0] - F[3]*2;
  F[1] = 1 - F[0] - F[2] - F[3];

  // weights for derivative
  G[3] = f2*half;
  G[0] = f - half - G[3];
  G[2] = 1 + G[0] - G[3]*2;
  G[1] = -G[0] - G[2] - G[3];
}

//-------------------------------------------------------------
// if the support region for the b-spline is not fully within
// the bounds, take action here according to the BorderMode
static int vtkBSplineTransformBorder(
  int gridId0[3], int gridId1[3], int gridId2[3], int gridId3[3],
  double *ff[3], double *gg[3], int ext[3], int borderMode)
{
  int pointIsInvalid = 0;

  switch (borderMode)
    {
    case VTK_BSPLINE_EDGE:
      // coefficient at edge continues infinitely past edge
      // (this is continuous and smooth)
      break;

    case VTK_BSPLINE_ZERO:
      // coefficients past edge are all zero
      // (this is continuous and smooth)
      for (int i = 0; i < 3; i++)
        {
        // note: "ext" is just the size subtract one
        if (ext[i] != 0)
          {
          if (gridId1[i] == 0)
            {
            ff[i][0] = 0.0;
            gg[i][0] = 0.0;
            }
          else if (gridId2[i] == 0)
            {
            ff[i][0] = 0.0; ff[i][1] = 0.0;
            gg[i][0] = 0.0; gg[i][1] = 0.0;
            }
          else if (gridId3[i] == 0)
            {
            ff[i][0] = 0.0; ff[i][1] = 0.0; ff[i][2] = 0.0;
            gg[i][0] = 0.0; gg[i][1] = 0.0; gg[i][2] = 0.0;
            }
          else if (gridId3[i] < 0)
            {
            pointIsInvalid = 1;
            }

          if (gridId2[i] == ext[i])
            {
            ff[i][3] = 0.0;
            gg[i][3] = 0.0;
            }
          else if (gridId1[i] == ext[i])
            {
            ff[i][2] = 0.0; ff[i][3] = 0.0;
            gg[i][2] = 0.0; gg[i][3] = 0.0;
            }
          else if (gridId0[i] == ext[i])
            {
            ff[i][1] = 0.0; ff[i][2] = 0.0; ff[i][3] = 0.0;
            gg[i][1] = 0.0; gg[i][2] = 0.0; gg[i][3] = 0.0;
            }
          else if (gridId0[i] > ext[i])
            {
            pointIsInvalid = 1;
            }
          }
        }
      break;

    case VTK_BSPLINE_ZERO_AT_BORDER:
      // adjust weights to achieve zero displacement at one
      // grid-spacing past the bounds of the grid
      // (this is continuous but not smooth)
      for (int j = 0; j < 3; j++)
        {
        // note: "ext" is just the size subtract one
        if (ext[j] != 0)
          {
          if (gridId1[j] == 0)
            {
            ff[j][0] = 0.0;
            if (gg[j])
              {
              gg[j][0] = 0.0;
              }
            }
          else if (gridId2[j] == 0)
            {
            ff[j][2] -= ff[j][0];
            ff[j][0] = 0.0;
            ff[j][1] = 0.0;
            gg[j][2] -= gg[j][0];
            gg[j][0] = 0.0;
            gg[j][1] = 0.0;
            }
          else if (gridId2[j] < 0)
            {
            pointIsInvalid = 1;
            }

          if (gridId2[j] == ext[j])
            {
            ff[j][3] = 0.0;
            gg[j][3] = 0.0;
            }
          else if (gridId1[j] == ext[j])
            {
            ff[j][1] -= ff[j][3];
            ff[j][2] = 0.0;
            ff[j][3] = 0.0;
            gg[j][1] -= gg[j][3];
            gg[j][2] = 0.0;
            gg[j][3] = 0.0;
            }
          else if (gridId1[j] > ext[j])
            {
            pointIsInvalid = 1;
            }
          }
        }
      break;
    }

  for (int k = 0; k < 3; k++)
    {
    // clamp to the boundary limits
    int emax = ext[k];
    if (gridId0[k] < 0) { gridId0[k] = 0; }
    if (gridId0[k] > emax) { gridId0[k] = emax; }
    if (gridId1[k] < 0) { gridId1[k] = 0; }
    if (gridId1[k] > emax) { gridId1[k] = emax; }
    if (gridId2[k] < 0) { gridId2[k] = 0; }
    if (gridId2[k] > emax) { gridId2[k] = emax; }
    if (gridId3[k] < 0) { gridId3[k] = 0; }
    if (gridId3[k] > emax) { gridId3[k] = emax; }
    }

  return pointIsInvalid;
}

//-------------------------------------------------------------
template <class T>
class vtkBSplineTransformFunction
{
  public:
    static void Cubic(
      const double point[3], double displacement[3], double derivatives[3][3],
      void *gridPtrVoid, int gridExt[6], vtkIdType gridInc[3], int borderMode);
};

template<class T>
void vtkBSplineTransformFunction<T>::Cubic(
  const double point[3], double displacement[3], double derivatives[3][3],
  void *gridPtrVoid, int gridExt[6], vtkIdType gridInc[3], int borderMode)
{
  // the interpolation weights
  double fX[4] = { 0, 1, 0, 0 };
  double fY[4] = { 0, 1, 0, 0 };
  double fZ[4] = { 0, 1, 0, 0 };
  double gX[4] = { 0, 0, 0, 0 };
  double gY[4] = { 0, 0, 0, 0 };
  double gZ[4] = { 0, 0, 0, 0 };

  double *ff[3];
  double *gg[3];

  ff[0] = fX;
  ff[1] = fY;
  ff[2] = fZ;

  gg[0] = gX;
  gg[1] = gY;
  gg[2] = gZ;

  // initialize the knot positions
  int gridId0[3] = { 0, 0, 0 };
  int gridId1[3] = { 0, 0, 0 };
  int gridId2[3] = { 0, 0, 0 };
  int gridId3[3] = { 0, 0, 0 };

  // "ext" is the size subtract one
  int ext[3];

  // compute the weights
  for (int i = 0; i < 3; i++)
    {
    ext[i] = gridExt[2*i + 1] - gridExt[2*i];

    if (ext[i] != 0)
      {
      // change point into integer plus fraction
      int idx = vtkMath::Floor(point[i]);
      double f = point[i] - idx;
      idx -= gridExt[2*i];
      idx--;
      gridId0[i] = idx++;
      gridId1[i] = idx++;
      gridId2[i] = idx++;
      gridId3[i] = idx++;

      vtkBSplineTransformWeights(ff[i], gg[i], f);
      }
    }

  // do bounds check, most points will be inside so optimize for that
  int pointIsInvalid = 0;

  if ((gridId0[0] | (ext[0] - gridId3[0]) |
       gridId0[1] | (ext[1] - gridId3[1]) |
       gridId0[2] | (ext[2] - gridId3[2])) < 0)
    {
    pointIsInvalid = vtkBSplineTransformBorder(
      gridId0, gridId1, gridId2, gridId3, ff, gg, ext, borderMode);
    }

  // Compute the indices into the data
  vtkIdType factX[4],factY[4],factZ[4];

  factX[0] = gridId0[0]*gridInc[0];
  factX[1] = gridId1[0]*gridInc[0];
  factX[2] = gridId2[0]*gridInc[0];
  factX[3] = gridId3[0]*gridInc[0];

  factY[0] = gridId0[1]*gridInc[1];
  factY[1] = gridId1[1]*gridInc[1];
  factY[2] = gridId2[1]*gridInc[1];
  factY[3] = gridId3[1]*gridInc[1];

  factZ[0] = gridId0[2]*gridInc[2];
  factZ[1] = gridId1[2]*gridInc[2];
  factZ[2] = gridId2[2]*gridInc[2];
  factZ[3] = gridId3[2]*gridInc[2];

  // initialize displacement and derivatives to zero
  displacement[0] = 0.0;
  displacement[1] = 0.0;
  displacement[2] = 0.0;

  if (derivatives)
    {
    for (int i = 0; i < 3; i++)
      {
      derivatives[i][0] = 0.0;
      derivatives[i][1] = 0.0;
      derivatives[i][2] = 0.0;
      }
    }

  // if point is valid, do the interpolation
  if (!pointIsInvalid)
    {
    T *gridPtr = static_cast<T *>(gridPtrVoid);

    // shortcut for 1D and 2D images (ext is size subtract 1)
    int singleZ = (ext[2] == 0);
    int jl = singleZ;
    int jm = 4 - 2*singleZ;
    int singleY = (ext[1] == 0);
    int kl = singleY;
    int km = 4 - 2*singleY;

    // here is the tricubic interpolation
    double vY[3],vZ[3];
    displacement[0] = 0;
    displacement[1] = 0;
    displacement[2] = 0;
    for (int j = jl; j < jm; j++)
      {
      T *gridPtr1 = gridPtr + factZ[j];
      vZ[0] = 0;
      vZ[1] = 0;
      vZ[2] = 0;
      for (int k = kl; k < km; k++)
        {
        T *gridPtr2 = gridPtr1 + factY[k];
        vY[0] = 0;
        vY[1] = 0;
        vY[2] = 0;
        if (!derivatives)
          {
          for (int l = 0; l < 4; l++)
            {
            T *gridPtr3 = gridPtr2 + factX[l];
            double f = fX[l];
            vY[0] += gridPtr3[0] * f;
            vY[1] += gridPtr3[1] * f;
            vY[2] += gridPtr3[2] * f;
            }
          }
        else
          {
          for (int l = 0; l < 4; l++)
            {
            T *gridPtr3 = gridPtr2 + factX[l];
            double f = fX[l];
            double gff = gX[l]*fY[k]*fZ[j];
            double fgf = fX[l]*gY[k]*fZ[j];
            double ffg = fX[l]*fY[k]*gZ[j];
            double inVal = gridPtr3[0];
            vY[0] += inVal * f;
            derivatives[0][0] += inVal * gff;
            derivatives[0][1] += inVal * fgf;
            derivatives[0][2] += inVal * ffg;
            inVal = gridPtr3[1];
            vY[1] += inVal * f;
            derivatives[1][0] += inVal * gff;
            derivatives[1][1] += inVal * fgf;
            derivatives[1][2] += inVal * ffg;
            inVal = gridPtr3[2];
            vY[2] += inVal * f;
            derivatives[2][0] += inVal * gff;
            derivatives[2][1] += inVal * fgf;
            derivatives[2][2] += inVal * ffg;
            }
          }
          vZ[0] += vY[0]*fY[k];
          vZ[1] += vY[1]*fY[k];
          vZ[2] += vY[2]*fY[k];
        }
      displacement[0] += vZ[0]*fZ[j];
      displacement[1] += vZ[1]*fZ[j];
      displacement[2] += vZ[2]*fZ[j];
      }
    }
}

} // end anonymous namespace

//----------------------------------------------------------------------------
vtkBSplineTransform::vtkBSplineTransform()
{
  this->Coefficients = 0;
  this->BorderMode = VTK_BSPLINE_EDGE;
  this->InverseTolerance = 1e-6;
  this->DisplacementScale = 1.0;
  this->CalculateSpline = 0;
}

//----------------------------------------------------------------------------
vtkBSplineTransform::~vtkBSplineTransform()
{
  this->SetCoefficients(0);
}

//----------------------------------------------------------------------------
void vtkBSplineTransform::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "BorderMode: " << this->GetBorderModeAsString() << "\n";
  os << indent << "DisplacementScale: " << this->DisplacementScale << "\n";
  os << indent << "Coefficients: " << this->Coefficients << "\n";
  if (this->Coefficients)
    {
    this->Coefficients->PrintSelf(os,indent.GetNextIndent());
    }
}

//----------------------------------------------------------------------------
const char *vtkBSplineTransform::GetBorderModeAsString()
{
  switch (this->BorderMode)
    {
    case VTK_BSPLINE_EDGE:
      return "Edge";
    case VTK_BSPLINE_ZERO:
      return "Zero";
    case VTK_BSPLINE_ZERO_AT_BORDER:
      return "ZeroAtBorder";
    default:
      break;
    }

  return "Unknown";
}

//----------------------------------------------------------------------------
// need to check the input image data to determine MTime
unsigned long vtkBSplineTransform::GetMTime()
{
  unsigned long mtime,result;
  result = vtkWarpTransform::GetMTime();
  if (this->Coefficients)
    {
    this->Coefficients->UpdateInformation();

    mtime = this->Coefficients->GetPipelineMTime();
    result = ( mtime > result ? mtime : result );

    mtime = this->Coefficients->GetMTime();
    result = ( mtime > result ? mtime : result );
    }

  return result;
}

//----------------------------------------------------------------------------
void vtkBSplineTransform::ForwardTransformPoint(const double inPoint[3],
                                                double outPoint[3])
{
  if (!this->Coefficients || !this->CalculateSpline)
    {
    outPoint[0] = inPoint[0];
    outPoint[1] = inPoint[1];
    outPoint[2] = inPoint[2];
    return;
    }

  void *gridPtr = this->GridPointer;
  double *spacing = this->GridSpacing;
  double *origin = this->GridOrigin;
  int *extent = this->GridExtent;
  vtkIdType *increments = this->GridIncrements;

  double scale = this->DisplacementScale;

  double point[3];
  double displacement[3];

  displacement[0] = 0.0;
  displacement[1] = 0.0;
  displacement[2] = 0.0;

  // Convert the inPoint to i,j,k indices into the deformation grid
  // plus fractions
  point[0] = (inPoint[0] - origin[0])/spacing[0];
  point[1] = (inPoint[1] - origin[1])/spacing[1];
  point[2] = (inPoint[2] - origin[2])/spacing[2];

  this->CalculateSpline(point, displacement, 0,
                        gridPtr, extent, increments, this->BorderMode);

  outPoint[0] = inPoint[0] + displacement[0]*scale;
  outPoint[1] = inPoint[1] + displacement[1]*scale;
  outPoint[2] = inPoint[2] + displacement[2]*scale;
}

//----------------------------------------------------------------------------
// convert float to double, transform, and back again
void vtkBSplineTransform::ForwardTransformPoint(const float point[3],
                                                float output[3])
{
  double fpoint[3];
  fpoint[0] = point[0];
  fpoint[1] = point[1];
  fpoint[2] = point[2];

  this->ForwardTransformPoint(fpoint,fpoint);

  output[0] = static_cast<float>(fpoint[0]);
  output[1] = static_cast<float>(fpoint[1]);
  output[2] = static_cast<float>(fpoint[2]);
}

//----------------------------------------------------------------------------
// calculate the derivative of the transform
void vtkBSplineTransform::ForwardTransformDerivative(const double inPoint[3],
                                                     double outPoint[3],
                                                     double derivative[3][3])
{
  if (!this->Coefficients || !this->CalculateSpline)
    {
    outPoint[0] = inPoint[0];
    outPoint[1] = inPoint[1];
    outPoint[2] = inPoint[2];
    vtkMath::Identity3x3(derivative);
    return;
    }

  void *gridPtr = this->GridPointer;
  double *spacing = this->GridSpacing;
  double *origin = this->GridOrigin;
  int *extent = this->GridExtent;
  vtkIdType *increments = this->GridIncrements;

  double scale = this->DisplacementScale;

  double point[3];
  double displacement[3];

  // convert the inPoint to i,j,k indices plus fractions
  point[0] = (inPoint[0] - origin[0])/spacing[0];
  point[1] = (inPoint[1] - origin[1])/spacing[1];
  point[2] = (inPoint[2] - origin[2])/spacing[2];

  this->CalculateSpline(point,displacement,derivative,
                        gridPtr,extent,increments, this->BorderMode);

  for (int i = 0; i < 3; i++)
    {
    derivative[i][0] = derivative[i][0]*scale/spacing[0];
    derivative[i][1] = derivative[i][1]*scale/spacing[1];
    derivative[i][2] = derivative[i][2]*scale/spacing[2];
    derivative[i][i] += 1.0;
    }

  outPoint[0] = inPoint[0] + displacement[0]*scale;
  outPoint[1] = inPoint[1] + displacement[1]*scale;
  outPoint[2] = inPoint[2] + displacement[2]*scale;
}

//----------------------------------------------------------------------------
// convert double to double
void vtkBSplineTransform::ForwardTransformDerivative(const float point[3],
                                                     float output[3],
                                                     float derivative[3][3])
{
  double fpoint[3];
  double fderivative[3][3];
  fpoint[0] = point[0];
  fpoint[1] = point[1];
  fpoint[2] = point[2];

  this->ForwardTransformDerivative(fpoint,fpoint,fderivative);

  for (int i = 0; i < 3; i++)
    {
    derivative[i][0] = static_cast<float>(fderivative[i][0]);
    derivative[i][1] = static_cast<float>(fderivative[i][1]);
    derivative[i][2] = static_cast<float>(fderivative[i][2]);
    output[i] = static_cast<float>(fpoint[i]);
    }
}

//----------------------------------------------------------------------------
// We use Newton's method to iteratively invert the transformation.
// This is actally quite robust as long as the Jacobian matrix is never
// singular.
// Note that this is similar to vtkWarpTransform::InverseTransformPoint()
// but has been optimized specifically for uniform grid transforms.
void vtkBSplineTransform::InverseTransformDerivative(const double inPoint[3],
                                                     double outPoint[3],
                                                     double derivative[3][3])
{
  if (!this->Coefficients || !this->CalculateSpline)
    {
    outPoint[0] = inPoint[0];
    outPoint[1] = inPoint[1];
    outPoint[2] = inPoint[2];
    return;
    }

  void *gridPtr = this->GridPointer;
  double *spacing = this->GridSpacing;
  double *origin = this->GridOrigin;
  int *extent = this->GridExtent;
  vtkIdType *increments = this->GridIncrements;

  double invSpacing[3];
  invSpacing[0] = 1.0/spacing[0];
  invSpacing[1] = 1.0/spacing[1];
  invSpacing[2] = 1.0/spacing[2];

  double scale = this->DisplacementScale;

  double point[3], inverse[3], lastInverse[3];
  double deltaP[3], deltaI[3];

  double functionValue = 0;
  double functionDerivative = 0;
  double lastFunctionValue = VTK_DOUBLE_MAX;

  double errorSquared = 0.0;
  double toleranceSquared = this->InverseTolerance;
  toleranceSquared *= toleranceSquared;

  double f = 1.0;
  double a;

  // convert the inPoint to i,j,k indices plus fractions
  point[0] = (inPoint[0] - origin[0])*invSpacing[0];
  point[1] = (inPoint[1] - origin[1])*invSpacing[1];
  point[2] = (inPoint[2] - origin[2])*invSpacing[2];

  // first guess at inverse point, just subtract displacement
  // (the inverse point is given in i,j,k indices plus fractions)
  this->CalculateSpline(point, deltaP, 0,
                        gridPtr, extent, increments, this->BorderMode);

  inverse[0] = point[0] - deltaP[0]*scale*invSpacing[0];
  inverse[1] = point[1] - deltaP[1]*scale*invSpacing[1];
  inverse[2] = point[2] - deltaP[2]*scale*invSpacing[2];
  lastInverse[0] = inverse[0];
  lastInverse[1] = inverse[1];
  lastInverse[2] = inverse[2];

  // do a maximum 500 iterations, usually less than 10 are required
  int n = this->InverseIterations;
  int i, j;

  for (i = 0; i < n; i++)
    {
    this->CalculateSpline(inverse, deltaP, derivative,
                          gridPtr, extent, increments, this->BorderMode);

    // convert displacement
    deltaP[0] = (inverse[0] - point[0])*spacing[0] + deltaP[0]*scale;
    deltaP[1] = (inverse[1] - point[1])*spacing[1] + deltaP[1]*scale;
    deltaP[2] = (inverse[2] - point[2])*spacing[2] + deltaP[2]*scale;

    // convert derivative
    for (j = 0; j < 3; j++)
      {
      derivative[j][0] = derivative[j][0]*scale*invSpacing[0];
      derivative[j][1] = derivative[j][1]*scale*invSpacing[1];
      derivative[j][2] = derivative[j][2]*scale*invSpacing[2];
      derivative[j][j] += 1.0;
      }

    // get the current function value
    functionValue = (deltaP[0]*deltaP[0] +
                     deltaP[1]*deltaP[1] +
                     deltaP[2]*deltaP[2]);

    // if the function value is decreasing, do next Newton step
    if (functionValue < lastFunctionValue)
      {
      // here is the critical step in Newton's method
      vtkMath::LinearSolve3x3(derivative,deltaP,deltaI);

      // get the error value in the output coord space
      errorSquared = (deltaI[0]*deltaI[0] +
                      deltaI[1]*deltaI[1] +
                      deltaI[2]*deltaI[2]);

      // break if less than tolerance in both coordinate systems
      if (errorSquared < toleranceSquared &&
          functionValue < toleranceSquared)
        {
        break;
        }

      // save the last inverse point
      lastInverse[0] = inverse[0];
      lastInverse[1] = inverse[1];
      lastInverse[2] = inverse[2];

      // save error at last inverse point
      lastFunctionValue = functionValue;

      // derivative of functionValue at last inverse point
      functionDerivative = (deltaP[0]*derivative[0][0]*deltaI[0] +
                            deltaP[1]*derivative[1][1]*deltaI[1] +
                            deltaP[2]*derivative[2][2]*deltaI[2])*2;

      // calculate new inverse point
      inverse[0] -= deltaI[0]*invSpacing[0];
      inverse[1] -= deltaI[1]*invSpacing[1];
      inverse[2] -= deltaI[2]*invSpacing[2];

      // reset f to 1.0
      f = 1.0;

      continue;
      }

    // the error is increasing, so take a partial step
    // (see Numerical Recipes 9.7 for rationale, this code
    //  is a simplification of the algorithm provided there)

    // quadratic approximation to find best fractional distance
    a = -functionDerivative/(2*(functionValue -
                                lastFunctionValue -
                                functionDerivative));

    // clamp to range [0.1,0.5]
    f *= (a < 0.1 ? 0.1 : (a > 0.5 ? 0.5 : a));

    // re-calculate inverse using fractional distance
    inverse[0] = lastInverse[0] - f*deltaI[0]*invSpacing[0];
    inverse[1] = lastInverse[1] - f*deltaI[1]*invSpacing[1];
    inverse[2] = lastInverse[2] - f*deltaI[2]*invSpacing[2];
    }

  if (i >= n)
    {
    // didn't converge: back up to last good result
    inverse[0] = lastInverse[0];
    inverse[1] = lastInverse[1];
    inverse[2] = lastInverse[2];

    vtkWarningMacro("InverseTransformPoint: no convergence (" <<
                    inPoint[0] << ", " << inPoint[1] << ", " << inPoint[2] <<
                    ") error = " << sqrt(errorSquared) << " after " <<
                    i << " iterations.");
    }

  // convert point
  outPoint[0] = inverse[0]*spacing[0] + origin[0];
  outPoint[1] = inverse[1]*spacing[1] + origin[1];
  outPoint[2] = inverse[2]*spacing[2] + origin[2];
}

//----------------------------------------------------------------------------
// convert double to double and back again
void vtkBSplineTransform::InverseTransformDerivative(const float point[3],
                                                     float output[3],
                                                     float derivative[3][3])
{
  double fpoint[3];
  double fderivative[3][3];
  fpoint[0] = point[0];
  fpoint[1] = point[1];
  fpoint[2] = point[2];

  this->InverseTransformDerivative(fpoint,fpoint,fderivative);

  for (int i = 0; i < 3; i++)
    {
    output[i] = static_cast<float>(fpoint[i]);
    derivative[i][0] = static_cast<float>(fderivative[i][0]);
    derivative[i][1] = static_cast<float>(fderivative[i][1]);
    derivative[i][2] = static_cast<float>(fderivative[i][2]);
    }
}

//----------------------------------------------------------------------------
void vtkBSplineTransform::InverseTransformPoint(const double point[3],
                                                double output[3])
{
  // the derivative won't be used, but it is required for Newton's method
  double derivative[3][3];
  this->InverseTransformDerivative(point,output,derivative);
}

//----------------------------------------------------------------------------
// convert double to double and back again
void vtkBSplineTransform::InverseTransformPoint(const float point[3],
                                                float output[3])
{
  double fpoint[3];
  double fderivative[3][3];
  fpoint[0] = point[0];
  fpoint[1] = point[1];
  fpoint[2] = point[2];

  this->InverseTransformDerivative(fpoint,fpoint,fderivative);

  output[0] = static_cast<float>(fpoint[0]);
  output[1] = static_cast<float>(fpoint[1]);
  output[2] = static_cast<float>(fpoint[2]);
}

//----------------------------------------------------------------------------
void vtkBSplineTransform::InternalDeepCopy(vtkAbstractTransform *transform)
{
  vtkBSplineTransform *gridTransform = (vtkBSplineTransform *)transform;

  this->SetInverseTolerance(gridTransform->InverseTolerance);
  this->SetInverseIterations(gridTransform->InverseIterations);
  this->CalculateSpline = gridTransform->CalculateSpline;
  this->SetCoefficients(gridTransform->Coefficients);
  this->SetDisplacementScale(gridTransform->DisplacementScale);
  this->SetBorderMode(gridTransform->BorderMode);

  if (this->InverseFlag != gridTransform->InverseFlag)
    {
    this->InverseFlag = gridTransform->InverseFlag;
    this->Modified();
    }
}

//----------------------------------------------------------------------------
void vtkBSplineTransform::InternalUpdate()
{
  vtkImageData *grid = this->Coefficients;

  if (grid == 0)
    {
    return;
    }

  grid->UpdateInformation();

  if (grid->GetNumberOfScalarComponents() != 3)
    {
    vtkErrorMacro(<< "TransformPoint: displacement grid must have 3 components");
    return;
    }

  // get the correct spline calculation function according to the grid type
  switch (grid->GetScalarType())
    {
    case VTK_FLOAT:
      this->CalculateSpline = &(vtkBSplineTransformFunction<float>::Cubic);
      break;
    case VTK_DOUBLE:
      this->CalculateSpline = &(vtkBSplineTransformFunction<double>::Cubic);
      break;
    default:
      this->CalculateSpline = 0;
      vtkErrorMacro("InternalUpdate: grid type must be float or double");
      break;
    }

  grid->SetUpdateExtent(grid->GetWholeExtent());
  grid->Update();

  this->GridPointer = grid->GetScalarPointer();
  grid->GetSpacing(this->GridSpacing);
  grid->GetOrigin(this->GridOrigin);
  grid->GetExtent(this->GridExtent);
  grid->GetIncrements(this->GridIncrements);
}

//----------------------------------------------------------------------------
vtkAbstractTransform *vtkBSplineTransform::MakeTransform()
{
  return vtkBSplineTransform::New();
}
