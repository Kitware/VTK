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
#include "vtkDoubleArray.h"
#include "vtkPiecewiseFunction.h"
#include "vtkColorTransferFunction.h"
#include "vtkUnstructuredGridLinearRayIntegrator.h"

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable:4100)
#endif

#include <set>
#include <algorithm>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

#include <math.h>

//-----------------------------------------------------------------------------

vtkCxxRevisionMacro(vtkUnstructuredGridPartialPreIntegration, "1.1");
vtkStandardNewMacro(vtkUnstructuredGridPartialPreIntegration);

float *vtkUnstructuredGridPartialPreIntegration::PsiTable = NULL;

//-----------------------------------------------------------------------------

vtkUnstructuredGridPartialPreIntegration::vtkUnstructuredGridPartialPreIntegration()
{
  this->Property = NULL;
  this->ControlPoints = NULL;
  this->NumIndependentComponents = 0;

  vtkUnstructuredGridPartialPreIntegration::BuildPsiTable();
}

vtkUnstructuredGridPartialPreIntegration::~vtkUnstructuredGridPartialPreIntegration()
{
  for (int c = 0; c < this->NumIndependentComponents; c++)
    {
    this->ControlPoints[c]->Delete();
    }
  delete[] this->ControlPoints;
}

void vtkUnstructuredGridPartialPreIntegration::PrintSelf(ostream &os,
                                                       vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------

void vtkUnstructuredGridPartialPreIntegration::Initialize(
                                                    vtkVolumeProperty *property,
                                                    vtkDataArray *scalars)
{
  if (   (property == this->Property)
      && (this->ControlPointsModified > property->GetMTime()) )
    {
    // Nothing has changed from the last time Initialize was run.
    return;
    }

  int numcomponents = scalars->GetNumberOfComponents();

  this->Property = property;
  this->ControlPointsModified.Modified();

  if (!property->GetIndependentComponents())
    {
    // The scalars actually hold material properties.
    if ((numcomponents != 4) && (numcomponents != 2) )
      {
      vtkErrorMacro("Only 2-tuples and 4-tuples allowed for dependent components.");
      }
    this->Property = property;
    return;
    }

  int component;
  for (component = 0; component < this->NumIndependentComponents; component++)
    {
    this->ControlPoints[component]->Delete();
    }
  delete[] this->ControlPoints;

  this->NumIndependentComponents = numcomponents;
  this->ControlPoints = new vtkDoubleArray*[numcomponents];

  vtkstd::set<double> cpset;
  for (component = 0; component < numcomponents; component++)
    {
    cpset.erase(cpset.begin(), cpset.end());
    vtkPiecewiseFunction *opacity = property->GetScalarOpacity(component);
    double *function_range = opacity->GetRange();
    double *function = opacity->GetDataPointer();
    while (1)
      {
      cpset.insert(function[0]);
      if (function[0] == function_range[1]) break;
      function += 2;
      }
    if (property->GetColorChannels(component) == 1)
      {
      vtkPiecewiseFunction *intensity
        = property->GetGrayTransferFunction(component);
      function_range = intensity->GetRange();
      function = intensity->GetDataPointer();
      while (1)
        {
        cpset.insert(function[0]);
        if (function[0] == function_range[1]) break;
        function += 2;
        }
      }
    else
      {
      vtkColorTransferFunction *color
        = property->GetRGBTransferFunction(component);
      function_range = color->GetRange();
      function = color->GetDataPointer();
      while (1)
        {
        cpset.insert(function[0]);
        if (function[0] == function_range[1]) break;
        function += 4;
        }
      }
    this->ControlPoints[component] = vtkDoubleArray::New();
    this->ControlPoints[component]->SetNumberOfComponents(1);
    this->ControlPoints[component]->SetNumberOfTuples(cpset.size());
    vtkstd::copy(cpset.begin(), cpset.end(),
                 this->ControlPoints[component]->GetPointer(0));
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
    vtkstd::set<double> segments;
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
        double *cp = this->ControlPoints[j]->GetPointer(0);
        vtkIdType numcp = this->ControlPoints[j]->GetNumberOfTuples();
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
      vtkstd::set<double>::iterator segi = segments.begin();
      double nearInterpolant = *segi;
      for (segi++; segi != segments.end(); segi++)
        {
        double farInterpolant = *segi;
        double nearcolor[4] = {0.0, 0.0, 0.0, 0.0};
        double farcolor[4] = {0.0, 0.0, 0.0, 0.0};
        double length = total_length*(farInterpolant-nearInterpolant);
        // Here we handle the mixing of material properties.  This never
        // seems to be defined very clearly.  I handle this by assuming
        // that each scalar represents a cloud of particles of a certian
        // color and a certain density.  We mix the scalars in the same way
        // as mixing these particles together.  By necessity, the density
        // becomes greater.  The "opacity" parameter is really interpreted
        // as the attenuation coefficient (which is proportional to
        // density) and can therefore easily be greater than one.  The
        // opacity of the resulting color will, however, always be scaled
        // between 0 and 1.
        for (int j = 0; j < numscalars; j++)
          {
          double c[4];
          double scalar
            = (farScalars[j]-nearScalars[j])*nearInterpolant + nearScalars[j];
          if (this->Property->GetColorChannels(j) == 3)
            {
            this->Property->GetRGBTransferFunction(j)->GetColor(scalar,c);
            }
          else
            {
            // Is this the right thing to do?
            c[0] = c[1] = c[2]
              = this->Property->GetGrayTransferFunction(j)->GetValue(scalar);
            }
          c[3] = this->Property->GetScalarOpacity(j)->GetValue(scalar);
          // Normalize by unit distance.
          c[3] /= this->Property->GetScalarOpacityUnitDistance(j);
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

          scalar
            = (farScalars[j]-nearScalars[j])*farInterpolant + nearScalars[j];
          if (this->Property->GetColorChannels(j) == 3)
            {
            this->Property->GetRGBTransferFunction(j)->GetColor(scalar,c);
            }
          else
            {
            // Is setting the RGB values the same the right thing to do?
            c[0] = c[1] = c[2]
              = this->Property->GetGrayTransferFunction(j)->GetValue(scalar);
            }
          c[3] = this->Property->GetScalarOpacity(j)->GetValue(scalar);
          // Normalize by unit distance.
          c[3] /= this->Property->GetScalarOpacityUnitDistance(j);
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
        this->IntegrateRay(length, nearcolor, nearcolor[3],
                           farcolor, farcolor[3], color);

        nearInterpolant = farInterpolant;
        }
      }
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

void vtkUnstructuredGridPartialPreIntegration::BuildPsiTable()
{
  if (PsiTable != NULL) return;

  PsiTable = new float[PSI_TABLE_SIZE*PSI_TABLE_SIZE];

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
}
