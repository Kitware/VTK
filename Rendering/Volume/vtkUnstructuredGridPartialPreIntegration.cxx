/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridPartialPreIntegration.cxx

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

#include "vtkUnstructuredGridPartialPreIntegration.h"

#include "vtkObjectFactory.h"
#include "vtkVolumeProperty.h"
#include "vtkVolume.h"
#include "vtkDoubleArray.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"
#include "vtkUnstructuredGridLinearRayIntegrator.h"
#include "vtkMath.h"

#include <vector>
#include <set>
#include <algorithm>

//-----------------------------------------------------------------------------

// VTK's native classes for defining transfer functions is actually slow to
// access, so we have to cache it somehow.  This class is straightforward
// copy of the transfer function.
class vtkPartialPreIntegrationTransferFunction
{
public:
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
  std::vector<double> ControlPoints;
  std::vector<acolor> Colors;
};

static const double huebends[6] = {
  1.0/6.0, 1.0/3.0, 0.5, 2.0/3.0, 5.0/6.0, 1.0
};

void vtkPartialPreIntegrationTransferFunction::GetTransferFunction(
                                              vtkColorTransferFunction *color,
                                              vtkPiecewiseFunction *opacity,
                                              double unit_distance,
                                              double scalar_range[2])
{
  std::set<double> cpset;

  double *function_range = color->GetRange();
  double *function = color->GetDataPointer();
  if( !function )
  {
    return;
  }
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
    for (i++; i != cpset.end(); i++)
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
        for ( ; huebends[j] < maxh; j++)
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
  this->ControlPoints.erase(this->ControlPoints.begin(),
                            this->ControlPoints.end());
  this->ControlPoints.resize(cpset.size());
  this->Colors.erase(this->Colors.begin(), this->Colors.end());
  this->Colors.resize(cpset.size());

  std::copy(cpset.begin(), cpset.end(), this->ControlPoints.begin());
  for (unsigned int i = 0; i < this->ControlPoints.size(); i++)
  {
    color->GetColor(this->ControlPoints[i], this->Colors[i].c);
    this->Colors[i].c[3] = (  opacity->GetValue(this->ControlPoints[i])
                            / unit_distance);
  }
}

void vtkPartialPreIntegrationTransferFunction::GetTransferFunction(
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
  this->ControlPoints.erase(this->ControlPoints.begin(),
                            this->ControlPoints.end());
  this->ControlPoints.resize(cpset.size());
  this->Colors.erase(this->Colors.begin(), this->Colors.end());
  this->Colors.resize(cpset.size());

  std::copy(cpset.begin(), cpset.end(), this->ControlPoints.begin());
  for (unsigned int i = 0; i < this->ControlPoints.size(); i++)
  {
    // Is setting all the colors to the same value the right thing to do?
    this->Colors[i].c[0] = this->Colors[i].c[1] = this->Colors[i].c[2]
      = intensity->GetValue(this->ControlPoints[i]);
    this->Colors[i].c[3] = (  opacity->GetValue(this->ControlPoints[i])
                            / unit_distance);
  }
}

inline void vtkPartialPreIntegrationTransferFunction::GetColor(double x,
                                                               double c[4])
{
  unsigned int i = 1;
  unsigned int size = static_cast<unsigned int>(this->ControlPoints.size());
  if( !size )
  {
      c[0] = 0.0;
      c[1] = 0.0;
      c[2] = 0.0;
      c[3] = 0.0;
    return;
  }
  while ((i < size-1) && (this->ControlPoints[i] < x))
  {
    i++;
  }

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

vtkStandardNewMacro(vtkUnstructuredGridPartialPreIntegration);

float vtkUnstructuredGridPartialPreIntegration::PsiTable[PSI_TABLE_SIZE*PSI_TABLE_SIZE];
int vtkUnstructuredGridPartialPreIntegration::PsiTableBuilt = 0;

//-----------------------------------------------------------------------------

vtkUnstructuredGridPartialPreIntegration::vtkUnstructuredGridPartialPreIntegration()
{
  this->Property = NULL;
  this->TransferFunctions = NULL;
  this->NumIndependentComponents = 0;
}

//-----------------------------------------------------------------------------
vtkUnstructuredGridPartialPreIntegration::~vtkUnstructuredGridPartialPreIntegration()
{
  delete[] this->TransferFunctions;
}

//-----------------------------------------------------------------------------
void vtkUnstructuredGridPartialPreIntegration::PrintSelf(ostream &os,
                                                       vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------

void vtkUnstructuredGridPartialPreIntegration::Initialize(
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

  this->BuildPsiTable();

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
    if (numcomponents == 2)
    {
      this->TransferFunctions
        = new vtkPartialPreIntegrationTransferFunction[1];
      this->TransferFunctions[0]
        .GetTransferFunction(property->GetRGBTransferFunction(0),
                             property->GetScalarOpacity(0),
                             property->GetScalarOpacityUnitDistance(0),
                             scalars->GetRange(0));
    }
    return;
  }

  delete[] this->TransferFunctions;

  this->NumIndependentComponents = numcomponents;
  this->TransferFunctions
      = new vtkPartialPreIntegrationTransferFunction[numcomponents];

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

void vtkUnstructuredGridPartialPreIntegration::Integrate(
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
        std::vector<double> &cp = this->TransferFunctions[j].ControlPoints;
        vtkIdType numcp = cp.size();
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
      for (segi++; segi != segments.end(); segi++)
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
      double nearColor[4], farColor[4], tmpColor[4];
      for (vtkIdType i = 0; i < numintersections; i++)
      {
        double length = intersectionLengths->GetValue(i);
        double *nearScalars = nearIntersections->GetTuple(i);
        double *farScalars = farIntersections->GetTuple(i);
        this->TransferFunctions[0].GetColor(nearScalars[0], nearColor);
        this->TransferFunctions[0].GetColor(nearScalars[1], tmpColor);
        nearColor[3] = tmpColor[3];
        this->TransferFunctions[0].GetColor(farScalars[0], farColor);
        this->TransferFunctions[0].GetColor(farScalars[1], tmpColor);
        farColor[3] = tmpColor[3];
        this->IntegrateRay(length, nearColor, nearColor[3]/unitdistance,
                           farColor, farColor[3]/unitdistance, color);
      }
    }
  }
}

//-----------------------------------------------------------------------------

void vtkUnstructuredGridPartialPreIntegration::BuildPsiTable()
{
  if (vtkUnstructuredGridPartialPreIntegration::PsiTableBuilt)
  {
    return;
  }

  for (int gammafi = 0; gammafi < PSI_TABLE_SIZE; gammafi++)
  {
    float gammaf = ((float)gammafi+0.0f)/PSI_TABLE_SIZE;
    float taufD = gammaf/(1-gammaf);
    for (int gammabi = 0; gammabi < PSI_TABLE_SIZE; gammabi++)
    {
      float gammab = ((float)gammabi+0.0f)/PSI_TABLE_SIZE;
      float taubD = gammab/(1-gammab);

      PsiTable[gammafi*PSI_TABLE_SIZE + gammabi]
        = vtkUnstructuredGridLinearRayIntegrator::Psi(1, taufD, taubD);
    }
  }

  vtkUnstructuredGridPartialPreIntegration::PsiTableBuilt = 1;
}
