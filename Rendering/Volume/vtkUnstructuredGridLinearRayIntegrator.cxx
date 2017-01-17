/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridLinearRayIntegrator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2004 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

#include "vtkUnstructuredGridLinearRayIntegrator.h"

#include "vtkObjectFactory.h"
#include "vtkVolumeProperty.h"
#include "vtkVolume.h"
#include "vtkDoubleArray.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"
#include "vtkMath.h"

#include <vector>
#include <set>
#include <algorithm>

#include <cmath>

#ifndef M_SQRTPI
#define M_SQRTPI        1.77245385090551602792981
#endif
#ifndef M_2_SQRTPI
#define M_2_SQRTPI      1.12837916709551257390
#endif
#ifndef M_1_SQRTPI
#define M_1_SQRTPI      (0.5*M_2_SQRTPI)
#endif

//-----------------------------------------------------------------------------

// VTK's native classes for defining transfer functions is actually slow to
// access, so we have to cache it somehow.  This class is straightforward
// copy of the transfer function.
class vtkLinearRayIntegratorTransferFunction
{
public:
  vtkLinearRayIntegratorTransferFunction();
  ~vtkLinearRayIntegratorTransferFunction();

  void GetTransferFunction(vtkColorTransferFunction *color,
                           vtkPiecewiseFunction *opacity,
                           double unit_distance,
                           double scalar_range[2]);
  void GetTransferFunction(vtkPiecewiseFunction *intensity,
                           vtkPiecewiseFunction *opacity,
                           double unit_distance,
                           double scalar_range[2]);

  inline void GetColor(double x, double c[4]);

  struct acolor {
    double c[4];
  };
  double *ControlPoints;
  int NumControlPoints;
  acolor *Colors;

private:
  vtkLinearRayIntegratorTransferFunction(const vtkLinearRayIntegratorTransferFunction&) VTK_DELETE_FUNCTION;
  void operator=(const vtkLinearRayIntegratorTransferFunction &) VTK_DELETE_FUNCTION;
};

vtkLinearRayIntegratorTransferFunction::vtkLinearRayIntegratorTransferFunction()
{
  this->ControlPoints = NULL;
  this->Colors = NULL;

  this->NumControlPoints = 0;
}

vtkLinearRayIntegratorTransferFunction::~vtkLinearRayIntegratorTransferFunction()
{
  delete[] this->ControlPoints;
  delete[] this->Colors;
}

static const double huebends[6] = {
  1.0/6.0, 1.0/3.0, 0.5, 2.0/3.0, 5.0/6.0, 1.0
};

void vtkLinearRayIntegratorTransferFunction::GetTransferFunction(
                                              vtkColorTransferFunction *color,
                                              vtkPiecewiseFunction *opacity,
                                              double unit_distance,
                                              double scalar_range[2])
{
  std::set<double> cpset;

  double *function_range = color->GetRange();
  double *function = color->GetDataPointer();
  while (1)
  {
    cpset.insert(function[0]);
    if (function[0] == function_range[1]) break;
    function += 4;
  }

  if (color->GetColorSpace() != VTK_CTF_RGB)
  {
    // If we are in an HSV color space, we must insert control points
    // in places where the RGB bends.
    double rgb[3], hsv[3];
    double hue1, hue2;
    double x1, x2;
    std::set<double>::iterator i = cpset.begin();
    x1 = *i;
    color->GetColor(x1, rgb);
    vtkMath::RGBToHSV(rgb, hsv);
    hue1 = hsv[0];
    for (++i; i != cpset.end(); ++i)
    {
      x2 = *i;
      color->GetColor(x2, rgb);
      vtkMath::RGBToHSV(rgb, hsv);
      hue2 = hsv[0];

      // Are we crossing the 0/1 boundary?
      if (   (color->GetColorSpace() == VTK_CTF_HSV && color->GetHSVWrap() )
          && ((hue1 - hue2 > 0.5) || (hue2 - hue1 > 0.5)) )
      {
        // Yes, we are crossing the boundary.
        if (hue1 > hue2)
        {
          int j;
          for (j = 0; huebends[j] <= hue2; j++)
          {
            double interp = (1-hue1+huebends[j])/(1-hue1+hue2);
            cpset.insert((x2-x1)*interp + x1);
          }
          while (huebends[j] < hue1) j++;
          for ( ; j < 6; j++)
          {
            double interp = (huebends[j]-hue1)/(1-hue1+hue2);
            cpset.insert((x2-x1)*interp + x1);
          }
        }
        else
        {
          int j;
          for (j = 0; huebends[j] <= hue1; j++)
          {
            double interp = (hue1-huebends[j])/(1-hue2+hue1);
            cpset.insert((x2-x1)*interp + x1);
          }
          while (huebends[j] < hue2) j++;
          for ( ; j < 6; j++)
          {
            double interp = (1-huebends[j]+hue1)/(1-hue2+hue1);
            cpset.insert((x2-x1)*interp + x1);
          }
        }
      }
      else
      {
        // No, we are not crossing the boundary.
        int j = 0;
        double minh, maxh;
        if (hue1 < hue2)
        {
          minh = hue1;  maxh = hue2;
        }
        else
        {
          minh = hue2;  maxh = hue1;
        }
        while (huebends[j] < minh) j++;
        for (j = 0; huebends[j] < maxh; j++)
        {
          double interp = (huebends[j]-hue1)/(hue2-hue1);
          cpset.insert((x2-x1)*interp + x1);
        }
      }

      x1 = x2;
      hue1 = hue2;
    }
  }

  function_range = opacity->GetRange();
  function = opacity->GetDataPointer();
  while (1)
  {
    cpset.insert(function[0]);
    if (function[0] == function_range[0]) break;
    function += 2;
  }

  // Add the scalar at the beginning and end of the range so the interpolation
  // is correct there.
  cpset.insert(scalar_range[0]);
  cpset.insert(scalar_range[1]);
  // Make extra sure there are at least two entries in cpset.
  if (cpset.size() < 2)
  {
    cpset.insert(0.0);
    cpset.insert(1.0);
  }

  // Now record control points and colors.
  delete[] this->ControlPoints;
  delete[] this->Colors;
  this->NumControlPoints = static_cast<int>(cpset.size());
  this->ControlPoints = new double[this->NumControlPoints];
  this->Colors = new acolor[this->NumControlPoints];

  std::copy(cpset.begin(), cpset.end(), this->ControlPoints);
  for (int i = 0; i < this->NumControlPoints; i++)
  {
    color->GetColor(this->ControlPoints[i], this->Colors[i].c);
    this->Colors[i].c[3] = (  opacity->GetValue(this->ControlPoints[i])
                            / unit_distance);
  }
}

void vtkLinearRayIntegratorTransferFunction::GetTransferFunction(
                                              vtkPiecewiseFunction *intensity,
                                              vtkPiecewiseFunction *opacity,
                                              double unit_distance,
                                              double scalar_range[2])
{
  std::set<double> cpset;

  double *function_range = intensity->GetRange();
  double *function = intensity->GetDataPointer();
  while (1)
  {
    cpset.insert(function[0]);
    if (function[0] == function_range[1]) break;
    function += 2;
  }

  function_range = opacity->GetRange();
  function = opacity->GetDataPointer();
  while (1)
  {
    cpset.insert(function[0]);
    if (function[0] == function_range[0]) break;
    function += 2;
  }

  // Add the scalar at the beginning and end of the range so the interpolation
  // is correct there.
  cpset.insert(scalar_range[0]);
  cpset.insert(scalar_range[1]);
  // Make extra sure there are at least two entries in cpset.
  if (cpset.size() < 2)
  {
    cpset.insert(0.0);
    cpset.insert(1.0);
  }

  // Now record control points and colors.
  delete[] this->ControlPoints;
  delete[] this->Colors;
  this->NumControlPoints = static_cast<int>(cpset.size());
  this->ControlPoints = new double[this->NumControlPoints];
  this->Colors = new acolor[this->NumControlPoints];

  std::copy(cpset.begin(), cpset.end(), this->ControlPoints);
  for (int i = 0; i < this->NumControlPoints; i++)
  {
    // Is setting all the colors to the same value the right thing to do?
    this->Colors[i].c[0] = this->Colors[i].c[1] = this->Colors[i].c[2]
      = intensity->GetValue(this->ControlPoints[i]);
    this->Colors[i].c[3] = (  opacity->GetValue(this->ControlPoints[i])
                            / unit_distance);
  }
}

inline void vtkLinearRayIntegratorTransferFunction::GetColor(double x,
                                                             double c[4])
{
  int i = 1;
  while ((i < this->NumControlPoints-1) && (this->ControlPoints[i] < x))
    i++;

  double before = this->ControlPoints[i-1];
  double after = this->ControlPoints[i];

  double interp = (x-before)/(after-before);

  double *beforec = this->Colors[i-1].c;
  double *afterc = this->Colors[i].c;
  c[0] = (1-interp)*beforec[0] + interp*afterc[0];
  c[1] = (1-interp)*beforec[1] + interp*afterc[1];
  c[2] = (1-interp)*beforec[2] + interp*afterc[2];
  c[3] = (1-interp)*beforec[3] + interp*afterc[3];
}

//-----------------------------------------------------------------------------

vtkStandardNewMacro(vtkUnstructuredGridLinearRayIntegrator);

vtkUnstructuredGridLinearRayIntegrator::vtkUnstructuredGridLinearRayIntegrator()
{
  this->Property = NULL;
  this->TransferFunctions = NULL;
  this->NumIndependentComponents = 0;
}

//-----------------------------------------------------------------------------

vtkUnstructuredGridLinearRayIntegrator::~vtkUnstructuredGridLinearRayIntegrator()
{
  delete[] this->TransferFunctions;
}

//-----------------------------------------------------------------------------

void vtkUnstructuredGridLinearRayIntegrator::PrintSelf(ostream &os,
                                                       vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------

void vtkUnstructuredGridLinearRayIntegrator::Initialize(
                                                    vtkVolume *volume,
                                                    vtkDataArray *scalars)
{
  vtkVolumeProperty *property = volume->GetProperty();

  if (   (property == this->Property)
      && (this->TransferFunctionsModified > property->GetMTime()) )
  {
    // Nothing has changed from the last time Initialize was run.
    return;
  }

  int numcomponents = scalars->GetNumberOfComponents();

  this->Property = property;
  this->TransferFunctionsModified.Modified();

  if (!property->GetIndependentComponents())
  {
    // The scalars actually hold material properties.
    if ((numcomponents != 4) && (numcomponents != 2) )
    {
      vtkErrorMacro("Only 2-tuples and 4-tuples allowed for dependent components.");
    }
    return;
  }

  delete[] this->TransferFunctions;

  this->NumIndependentComponents = numcomponents;
  this->TransferFunctions
    = new vtkLinearRayIntegratorTransferFunction[numcomponents];

  for (int component = 0; component < numcomponents; component++)
  {
    if (property->GetColorChannels(component) == 1)
    {
      this->TransferFunctions[component]
        .GetTransferFunction(property->GetGrayTransferFunction(component),
                             property->GetScalarOpacity(component),
                             property->GetScalarOpacityUnitDistance(component),
                             scalars->GetRange(component));
    }
    else
    {
      this->TransferFunctions[component]
        .GetTransferFunction(property->GetRGBTransferFunction(component),
                             property->GetScalarOpacity(component),
                             property->GetScalarOpacityUnitDistance(component),
                             scalars->GetRange(component));
    }
  }
}

//-----------------------------------------------------------------------------

void vtkUnstructuredGridLinearRayIntegrator::Integrate(
                                            vtkDoubleArray *intersectionLengths,
                                            vtkDataArray *nearIntersections,
                                            vtkDataArray *farIntersections,
                                            float color[4])
{
  int numintersections = intersectionLengths->GetNumberOfTuples();
  if (this->Property->GetIndependentComponents())
  {
    int numscalars = nearIntersections->GetNumberOfComponents();
    double *nearScalars = new double[numscalars];
    double *farScalars = new double[numscalars];
    std::set<double> segments;
    for (vtkIdType i = 0; i < numintersections; i++)
    {
      double total_length = intersectionLengths->GetValue(i);
      nearIntersections->GetTuple(i, nearScalars);
      farIntersections->GetTuple(i, farScalars);

      // Split up segment on control points, because it is nonlinear in
      // these regions.
      segments.erase(segments.begin(), segments.end());
      segments.insert(0.0);
      segments.insert(1.0);
      for (int j = 0; j < numscalars; j++)
      {
        double *cp = this->TransferFunctions[j].ControlPoints;
        vtkIdType numcp = this->TransferFunctions[j].NumControlPoints;
        double minscalar, maxscalar;
        if (nearScalars[j] < farScalars[j])
        {
          minscalar = nearScalars[j];  maxscalar = farScalars[j];
        }
        else
        {
          minscalar = farScalars[j];  maxscalar = nearScalars[j];
        }
        for (int k = 0; k < numcp; k++)
        {
          if (cp[k] <= minscalar) continue;
          if (cp[k] >= maxscalar) break;
          // If we are here, we need to break the segment at the given scalar.
          // Find the fraction between the near and far segment points.
          segments.insert(  (cp[k]-nearScalars[j])
                          / (farScalars[j]-nearScalars[j]));
        }
      }

      // Iterate over all the segment pieces (from front to back) and
      // integrate each piece.
      std::set<double>::iterator segi = segments.begin();
      double nearInterpolant = *segi;
      for (++segi; segi != segments.end(); ++segi)
      {
        double farInterpolant = *segi;
        double nearcolor[4] = {0.0, 0.0, 0.0, 0.0};
        double farcolor[4] = {0.0, 0.0, 0.0, 0.0};
        double length = total_length*(farInterpolant-nearInterpolant);
        // Here we handle the mixing of material properties.  This never
        // seems to be defined very clearly.  I handle this by assuming
        // that each scalar represents a cloud of particles of a certain
        // color and a certain density.  We mix the scalars in the same way
        // as mixing these particles together.  By necessity, the density
        // becomes greater.  The "opacity" parameter is really interpreted
        // as the attenuation coefficient (which is proportional to
        // density) and can therefore easily be greater than one.  The
        // opacity of the resulting color will, however, always be scaled
        // between 0 and 1.
        for (int j = 0; j < numscalars; j++)
        {
          double scalar
            = (farScalars[j]-nearScalars[j])*nearInterpolant + nearScalars[j];
          if (j == 0)
          {
            this->TransferFunctions[j].GetColor(scalar, nearcolor);
          }
          else
          {
            double c[4];
            this->TransferFunctions[j].GetColor(scalar, c);
            if (c[3] + nearcolor[3] > 1.0e-8f)
            {
              nearcolor[0] *= nearcolor[3]/(c[3] + nearcolor[3]);
              nearcolor[1] *= nearcolor[3]/(c[3] + nearcolor[3]);
              nearcolor[2] *= nearcolor[3]/(c[3] + nearcolor[3]);
              nearcolor[0] += c[0]*c[3]/(c[3] + nearcolor[3]);
              nearcolor[1] += c[1]*c[3]/(c[3] + nearcolor[3]);
              nearcolor[2] += c[2]*c[3]/(c[3] + nearcolor[3]);
              nearcolor[3] += c[3];
            }
          }

          scalar
            = (farScalars[j]-nearScalars[j])*farInterpolant + nearScalars[j];
          if (j == 0)
          {
            this->TransferFunctions[j].GetColor(scalar, farcolor);
          }
          else
          {
            double c[4];
            this->TransferFunctions[j].GetColor(scalar, c);
            if (c[3] + farcolor[3] > 1.0e-8f)
            {
              farcolor[0] *= farcolor[3]/(c[3] + farcolor[3]);
              farcolor[1] *= farcolor[3]/(c[3] + farcolor[3]);
              farcolor[2] *= farcolor[3]/(c[3] + farcolor[3]);
              farcolor[0] += c[0]*c[3]/(c[3] + farcolor[3]);
              farcolor[1] += c[1]*c[3]/(c[3] + farcolor[3]);
              farcolor[2] += c[2]*c[3]/(c[3] + farcolor[3]);
              farcolor[3] += c[3];
            }
          }
        }
        this->IntegrateRay(length, nearcolor, nearcolor[3],
                           farcolor, farcolor[3], color);

        nearInterpolant = farInterpolant;
      }
    }
      delete[] nearScalars;
      delete[] farScalars;
  }
  else
  {
    double unitdistance = this->Property->GetScalarOpacityUnitDistance();
    if (nearIntersections->GetNumberOfComponents() == 4)
    {
      for (vtkIdType i = 0; i < numintersections; i++)
      {
        double length = intersectionLengths->GetValue(i);
        double *nearcolor = nearIntersections->GetTuple(i);
        double *farcolor = farIntersections->GetTuple(i);
        this->IntegrateRay(length, nearcolor, nearcolor[3]/unitdistance,
                           farcolor, farcolor[3]/unitdistance, color);
      }
    }
    else  // Two components.
    {
      for (vtkIdType i = 0; i < numintersections; i++)
      {
        double length = intersectionLengths->GetValue(i);
        double *nearcolor = nearIntersections->GetTuple(i);
        double *farcolor = farIntersections->GetTuple(i);
        this->IntegrateRay(length, nearcolor[0], nearcolor[1]/unitdistance,
                           farcolor[0], farcolor[1]/unitdistance, color);
      }
    }
  }
}

//-----------------------------------------------------------------------------

void vtkUnstructuredGridLinearRayIntegrator::IntegrateRay(
                                                       double length,
                                                       double intensity_front,
                                                       double attenuation_front,
                                                       double intensity_back,
                                                       double attenuation_back,
                                                       float color[4])
{
  float Psi = vtkUnstructuredGridLinearRayIntegrator::Psi(length,
                                                          attenuation_front,
                                                          attenuation_back);
  float zeta = (float)exp(-0.5*length*(attenuation_front+attenuation_back));
  float alpha = 1-zeta;

  float newintensity = (1-color[3])*(  intensity_front*(1-Psi)
                                     + intensity_back*(Psi-zeta) );
  // Is setting the RGB values the same the right thing to do?
  color[0] += newintensity;
  color[1] += newintensity;
  color[2] += newintensity;
  color[3] += (1-color[3])*alpha;
}

void vtkUnstructuredGridLinearRayIntegrator::IntegrateRay(
                                                    double length,
                                                    const double color_front[3],
                                                    double attenuation_front,
                                                    const double color_back[3],
                                                    double attenuation_back,
                                                    float color[4])
{
  float Psi = vtkUnstructuredGridLinearRayIntegrator::Psi(length,
                                                          attenuation_front,
                                                          attenuation_back);
  float zeta = (float)exp(-0.5*length*(attenuation_front+attenuation_back));
  float alpha = 1-zeta;

  color[0] += (1-color[3])*(color_front[0]*(1-Psi) + color_back[0]*(Psi-zeta));
  color[1] += (1-color[3])*(color_front[1]*(1-Psi) + color_back[1]*(Psi-zeta));
  color[2] += (1-color[3])*(color_front[2]*(1-Psi) + color_back[2]*(Psi-zeta));
  color[3] += (1-color[3])*alpha;
}

//-----------------------------------------------------------------------------

static inline float erf_fitting_function(float u)
{
  return
    - 1.26551223 + u*(1.00002368 + u*(0.37409196 + u*(0.09678418 +
        u*(-0.18628806 + u*(0.27886807 + u*(-1.13520398 + u*(1.48851587 +
        u*(-0.82215223 + u*0.17087277))))))));
}

#if 0
// This function is not used directly.  It is here for reference.
static inline float erf(float x)
{
  /* Compute as described in Numerical Recipes in C++ by Press, et al. */
/*   x = abs(x);        In this application, x should always be >= 0. */
  float u = 1/(1 + 0.5*x);
  float ans = u*exp(-x*x + erf_fitting_function(u));
/*   return (x >= 0 ? 1 - ans : ans - 1);    x should always be >= 0. */
  return 1 - ans;
}
#endif

/* Compute Dawson's integral as described in Numerical Recipes in C++ by
   Press, et al. */
#define H 0.4
static const float dawson_constant0 = 0.852144;
static const float dawson_constant1 = 0.236928;
static const float dawson_constant2 = 0.0183156;
static const float dawson_constant3 = 0.000393669;
static const float dawson_constant4 = 2.35258e-6;
static const float dawson_constant5 = 3.90894e-9;
static inline float dawson(float x)
{
  if (x > 0.2)
  {
/*  x = abs(x);       In this application, x should always be >= 0. */
    int n0 = 2*(int)((0.5/H)*x + 0.5);
    float xp = x - (float)n0*H;
    float e1 = exp((2*H)*xp);
    float e2 = e1*e1;
    float d1 = n0 + 1;
    float d2 = d1 - 2;
    float sum = 0;
    sum = dawson_constant0*(e1/d1 + 1/(d2*e1));
    d1 += 2;  d2 -= 2;  e1 *= e2;
    sum += dawson_constant1*(e1/d1 + 1/(d2*e1));
    d1 += 2;  d2 -= 2;  e1 *= e2;
    sum += dawson_constant2*(e1/d1 + 1/(d2*e1));
    d1 += 2;  d2 -= 2;  e1 *= e2;
    sum += dawson_constant3*(e1/d1 + 1/(d2*e1));
    d1 += 2;  d2 -= 2;  e1 *= e2;
    sum += dawson_constant4*(e1/d1 + 1/(d2*e1));
    d1 += 2;  d2 -= 2;  e1 *= e2;
    sum += dawson_constant5*(e1/d1 + 1/(d2*e1));
    return M_1_SQRTPI*exp(-xp*xp)*sum;
  }
  else
  {
    float x2 = x*x;
    return x*(1 - (2.0/3.0)*x2*(1 - .4*x2*(1 - (2.0/7.0)*x2)));
  }
}

#if 0
// This function is not used directly.  It is here for reference.
inline float erfi(float x)
{
  return M_2_SQRTPI*exp(x*x)*dawson(x);
}
#endif

float vtkUnstructuredGridLinearRayIntegrator::Psi(float length,
                                                  float attenuation_front,
                                                  float attenuation_back)
{
  float difftauD = length*fabs(attenuation_back - attenuation_front);

  if (difftauD < 1.0e-8f)
  {
    // Volume is homogeneous (with respect to attenuation).
    float tauD = length * attenuation_front;
    if (tauD < 1.0e-8f)
    {
      return 1;
    }
    else
    {
      return (1 - (float)exp(-tauD))/tauD;
    }
  }
  else
  {
    float invsqrt2difftauD = 1/(float)sqrt(2*difftauD);
    float frontterm = length*invsqrt2difftauD*attenuation_front;
    float backterm  = length*invsqrt2difftauD*attenuation_back;
    if (attenuation_back > attenuation_front)
    {
      float u, Y;
      u = 1/(1+0.5f*frontterm);
      Y = u*(float)exp(erf_fitting_function(u));
      u = 1/(1+0.5f*backterm);
      Y += -u*exp(  frontterm*frontterm-backterm*backterm
                          + erf_fitting_function(u));
      Y *= M_SQRTPI*invsqrt2difftauD;
      return Y;
    }
    else
    {
      float expterm = (float)exp(backterm*backterm-frontterm*frontterm);
      return 2*invsqrt2difftauD*(dawson(frontterm) - expterm*dawson(backterm));
    }
  }
}
