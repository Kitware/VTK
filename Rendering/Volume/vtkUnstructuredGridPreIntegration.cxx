/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridPreIntegration.cxx

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

#include "vtkUnstructuredGridPreIntegration.h"

#include "vtkObjectFactory.h"
#include "vtkVolumeProperty.h"
#include "vtkVolume.h"
#include "vtkAbstractVolumeMapper.h"
#include "vtkUnstructuredGrid.h"
#include "vtkDoubleArray.h"
#include "vtkUnstructuredGridPartialPreIntegration.h"

#include <algorithm>
#include <math.h>

//-----------------------------------------------------------------------------

vtkStandardNewMacro(vtkUnstructuredGridPreIntegration);

vtkCxxSetObjectMacro(vtkUnstructuredGridPreIntegration, Integrator,
                     vtkUnstructuredGridVolumeRayIntegrator);

//-----------------------------------------------------------------------------

vtkUnstructuredGridPreIntegration::vtkUnstructuredGridPreIntegration()
{
  this->Integrator = vtkUnstructuredGridPartialPreIntegration::New();
  this->Property = NULL;

  this->NumComponents = 0;
  this->IntegrationTable = NULL;
  this->IntegrationTableScalarShift = NULL;
  this->IntegrationTableScalarScale = NULL;

  this->IntegrationTableScalarResolution = 128;
  this->IntegrationTableLengthResolution = 256;

  this->IncrementalPreIntegration = 1;
  this->IntegrationTableLengthScale = 0;
}

vtkUnstructuredGridPreIntegration::~vtkUnstructuredGridPreIntegration()
{
  this->SetIntegrator(NULL);

  if (this->IntegrationTable)
    {
    for (int i = 0; i < this->NumComponents; i++)
      {
      delete[] this->IntegrationTable[i];
      }
    delete[] this->IntegrationTable;
    }
  if (this->IntegrationTableScalarShift)
    {
    delete[] this->IntegrationTableScalarShift;
    }
  if (this->IntegrationTableScalarScale)
    {
    delete[] this->IntegrationTableScalarScale;
    }
}

void vtkUnstructuredGridPreIntegration::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Integrator: " << this->Integrator << endl;

  os << indent << "IntegrationTableScalarResolution: "
     << this->IntegrationTableScalarResolution << endl;
  os << indent << "IntegrationTableLengthResolution: "
     << this->IntegrationTableLengthResolution << endl;

  os << indent << "IncrementalPreIntegration: "
     << this->IncrementalPreIntegration << endl;
}

//-----------------------------------------------------------------------------

double vtkUnstructuredGridPreIntegration::GetIntegrationTableScalarShift(int component)
{
  return this->IntegrationTableScalarShift[component];
}

double vtkUnstructuredGridPreIntegration::GetIntegrationTableScalarScale(int component)
{
  return this->IntegrationTableScalarScale[component];
}

double vtkUnstructuredGridPreIntegration::GetIntegrationTableLengthScale()
{
  return this->IntegrationTableLengthScale;
}

float *vtkUnstructuredGridPreIntegration::GetPreIntegrationTable(int component)
{
  return this->IntegrationTable[component];
}

//-----------------------------------------------------------------------------

void vtkUnstructuredGridPreIntegration::BuildPreIntegrationTables(vtkDataArray *scalars)
{
  // Delete old tables.
  if (this->IntegrationTable)
    {
    for (int i = 0; i < this->NumComponents; i++)
      {
      delete[] this->IntegrationTable[i];
      }
    delete[] this->IntegrationTable;
    }
  if (this->IntegrationTableScalarShift)
    {
    delete[] this->IntegrationTableScalarShift;
    }
  if (this->IntegrationTableScalarScale)
    {
    delete[] this->IntegrationTableScalarScale;
    }

  this->NumComponents = scalars->GetNumberOfComponents();

  // Establish temporary inputs to integrator.
  vtkVolume         *tmpVolume = vtkVolume::New();
  vtkVolumeProperty *tmpProperty = vtkVolumeProperty::New();
  vtkDataArray      *tmpScalars
    = vtkDataArray::CreateDataArray(scalars->GetDataType());

  tmpVolume->SetMapper(this->Volume->GetMapper());
  tmpVolume->SetProperty(tmpProperty);

  tmpProperty->IndependentComponentsOn();
  tmpProperty->SetInterpolationType(this->Property->GetInterpolationType());

  tmpScalars->SetNumberOfComponents(1);
  tmpScalars->SetNumberOfTuples(2);

  vtkDoubleArray *tmpIntersectionLengths = vtkDoubleArray::New();
  vtkDataArray   *tmpNearIntersections
    = vtkDataArray::CreateDataArray(scalars->GetDataType());
  vtkDataArray   *tmpFarIntersections
    = vtkDataArray::CreateDataArray(scalars->GetDataType());

  tmpIntersectionLengths->SetNumberOfComponents(1);
  tmpIntersectionLengths->SetNumberOfTuples(1);
  tmpNearIntersections->SetNumberOfComponents(1);
  tmpNearIntersections->SetNumberOfTuples(1);
  tmpFarIntersections->SetNumberOfComponents(1);
  tmpFarIntersections->SetNumberOfTuples(1);

  this->IntegrationTable = new float*[this->NumComponents];
  this->IntegrationTableScalarShift = new double[this->NumComponents];
  this->IntegrationTableScalarScale = new double[this->NumComponents];
  // Note that the scale set up such that a length of (this->MaxLength +
  // epsilon + 0.5) will scale to this->IntegrationTableLengthResolution-1.
  // Similar scaling is performed for the other dimensions of the
  // pre-integration table.
  this->IntegrationTableLengthScale
    = (this->IntegrationTableLengthResolution-2)/this->MaxLength;

  // We only do computations at one length.
  float d_length = (float)(1/this->IntegrationTableLengthScale);

  for (int component = 0; component < this->NumComponents; component++)
    {
    int d_idx, sb_idx, sf_idx;
    float *c;

    // Allocate table.
    try
      {
      this->IntegrationTable[component]
        = new float[4*this->IntegrationTableScalarResolution
                   *this->IntegrationTableScalarResolution
                   *this->IntegrationTableLengthResolution];
      }
    catch (...)
      {
      this->IntegrationTable[component] = NULL;
      }

    if (this->IntegrationTable[component] == NULL)
      {
      // Could not allocate memory for table.
      if (   (this->IntegrationTableScalarResolution > 32)
          || (this->IntegrationTableLengthResolution > 64) )
        {
        vtkWarningMacro("Could not allocate integration table.\n"
                        "Reducing the table size and trying again.");
        for (int i = 0; i < component; i++)
          {
          delete[] this->IntegrationTable[i];
          }
        delete[] this->IntegrationTable;
        this->IntegrationTable = NULL;

        this->IntegrationTableScalarResolution = 32;
        this->IntegrationTableLengthResolution = 64;
        this->BuildPreIntegrationTables(scalars);
        }
      else
        {
        vtkErrorMacro("Could not allocate integration table.");
        }
      break;
      }

    // Determine scale and shift.
    double *range = scalars->GetRange(component);
    if (range[0] == range[1])
      {
      // Unusual case where the scalars are all the same.
      this->IntegrationTableScalarScale[component] = 1.0;
      }
    else
      {
      this->IntegrationTableScalarScale[component]
        = (this->IntegrationTableScalarResolution-2)/(range[1]-range[0]);
      }
    this->IntegrationTableScalarShift[component]
      = -range[0]*this->IntegrationTableScalarScale[component];

    // Set values for d=0 (they are all zero).
    c = this->IntegrationTable[component];
    for (sb_idx = 0; sb_idx < this->IntegrationTableScalarResolution; sb_idx++)
      {
      for (sf_idx = 0; sf_idx < this->IntegrationTableScalarResolution;
           sf_idx++)
        {
        c[0] = c[1] = c[2] = c[3] = 0.0f;
        c += 4;
        }
      }

    // Initialize integrator.
    if (this->Property->GetColorChannels(component) == 3)
      {
      tmpProperty->SetColor(this->Property->GetRGBTransferFunction(component));
      }
    else
      {
      tmpProperty->SetColor(this->Property->GetGrayTransferFunction(component));
      }
    tmpProperty->SetScalarOpacity(this->Property->GetScalarOpacity(component));
    tmpProperty->SetScalarOpacityUnitDistance
      (this->Property->GetScalarOpacityUnitDistance(component));
    tmpProperty->SetShade(this->Property->GetShade(component));
    tmpProperty->SetAmbient(this->Property->GetAmbient(component));
    tmpProperty->SetDiffuse(this->Property->GetDiffuse(component));
    tmpProperty->SetSpecular(this->Property->GetSpecular(component));
    tmpProperty->SetSpecularPower(this->Property->GetSpecularPower(component));
    tmpScalars->SetTuple1(0, range[0]);
    tmpScalars->SetTuple1(1, range[1]);
    this->Integrator->Initialize(tmpVolume, tmpScalars);

    // Set values for next smallest d (the base values).
    tmpIntersectionLengths->SetTuple1(0, d_length);
    for (sb_idx = 0; sb_idx < this->IntegrationTableScalarResolution; sb_idx++)
      {
      tmpFarIntersections
        ->SetTuple1(0,
                    (  (sb_idx - this->IntegrationTableScalarShift[component])
                     / (this->IntegrationTableScalarScale[component]) ));
      for (sf_idx = 0; sf_idx < this->IntegrationTableScalarResolution;
           sf_idx++)
        {
        tmpNearIntersections
          ->SetTuple1(0,
                      (  (sf_idx - this->IntegrationTableScalarShift[component])
                       / (this->IntegrationTableScalarScale[component]) ));
        c[0] = c[1] = c[2] = c[3] = 0;
        this->Integrator->Integrate(tmpIntersectionLengths,
                                    tmpNearIntersections, tmpFarIntersections,
                                    c);
        c += 4;
        }
      }

    // Set rest of values using other values in table.
    if (this->IncrementalPreIntegration)
      {
      for (d_idx = 2; d_idx < this->IntegrationTableLengthResolution; d_idx++)
        {
        for (sb_idx = 0; sb_idx < this->IntegrationTableScalarResolution;
             sb_idx++)
          {
          for (sf_idx = 0; sf_idx < this->IntegrationTableScalarResolution;
               sf_idx++)
            {
            // We are going to perform incremental pre-integration.  To do
            // this, we compute the integration of a ray from sf to sb of
            // length d by combining two entries in the table.  The first
            // entry will be from sf to sm of length delta d (the smallest
            // non-zero length stored in the table).  The second entry will
            // be from sm to sb of length d - delta d.  See Weiler, et
            // al. "Hardware-Based Ray Casting for Tetrahedral Meshes" for
            // more details.
            int sm_idx = ((d_idx-1)*sf_idx + sb_idx + d_idx/2)/d_idx;

            float *colorf = this->GetIndexedTableEntry(sf_idx, sm_idx, 1,
                                                       component);
            float *colorb = this->GetIndexedTableEntry(sm_idx, sb_idx, d_idx-1,
                                                       component);

            c[0] = colorf[0] + colorb[0]*(1-colorf[3]);
            c[1] = colorf[1] + colorb[1]*(1-colorf[3]);
            c[2] = colorf[2] + colorb[2]*(1-colorf[3]);
            c[3] = colorf[3] + colorb[3]*(1-colorf[3]);
            c += 4;
            }
          }
        }
      }
    else
      {
      for (d_idx = 2; d_idx < this->IntegrationTableLengthResolution; d_idx++)
        {
        for (sb_idx = 0; sb_idx < this->IntegrationTableScalarResolution;
             sb_idx++)
          {
          for (sf_idx = 0; sf_idx < this->IntegrationTableScalarResolution;
               sf_idx++)
            {
            // Compute the integration table the old-fashioned slow way.
            float length = d_idx*d_length;
            float sb = (float)
              (  (sb_idx - this->IntegrationTableScalarShift[component])
               / (this->IntegrationTableScalarScale[component]) );
            float sf = (float)
              (  (sf_idx - this->IntegrationTableScalarShift[component])
               / (this->IntegrationTableScalarScale[component]) );
            tmpIntersectionLengths->SetTuple1(0, length);
            tmpFarIntersections->SetTuple1(0, sb);
            tmpNearIntersections->SetTuple1(0, sf);
            c[0] = c[1] = c[2] = c[3] = 0;
            this->Integrator->Integrate(tmpIntersectionLengths,
                                        tmpNearIntersections,
                                        tmpFarIntersections,
                                        c);
            c += 4;
            }
          }
        }
      }
    }

  // Get rid of temporary data.
  tmpVolume->Delete();
  tmpProperty->Delete();
  tmpScalars->Delete();

  tmpIntersectionLengths->Delete();
  tmpNearIntersections->Delete();
  tmpFarIntersections->Delete();
}

//-----------------------------------------------------------------------------

void vtkUnstructuredGridPreIntegration::Initialize(vtkVolume *volume,
                                                   vtkDataArray *scalars)
{
  vtkIdType i;
  vtkVolumeProperty *property = volume->GetProperty();

  if (   (property == this->Property)
      && (this->IntegrationTableBuilt > property->GetMTime())
      && (this->IntegrationTableBuilt > this->MTime) )
    {
    // Nothing changed from the last time Initialize was run.
    return;
    }

  this->Property = property;
  this->Volume = volume;
  this->IntegrationTableBuilt.Modified();

  if (!property->GetIndependentComponents())
    {
    vtkErrorMacro("Cannot store dependent components in pre-integration table.");
    return;
    }

  // Determine the maximum possible length of a ray segment.
  vtkDataSet *input = volume->GetMapper()->GetDataSetInput();
  vtkIdType numcells = input->GetNumberOfCells();
  this->MaxLength = 0;
  for (i = 0; i < numcells; i++)
    {
    double cellbounds[6];
    input->GetCellBounds(i, cellbounds);
#define SQR(x)  ((x)*(x))
    double diagonal_length = sqrt(  SQR(cellbounds[1]-cellbounds[0])
                                  + SQR(cellbounds[3]-cellbounds[2])
                                  + SQR(cellbounds[5]-cellbounds[4]) );
#undef SQR
    if (diagonal_length > this->MaxLength)
      {
      this->MaxLength = diagonal_length;
      }
    }

  this->BuildPreIntegrationTables(scalars);
}

//-----------------------------------------------------------------------------

void vtkUnstructuredGridPreIntegration::Integrate(
                                            vtkDoubleArray *intersectionLengths,
                                            vtkDataArray *nearIntersections,
                                            vtkDataArray *farIntersections,
                                            float color[4])
{
  vtkIdType numIntersections = intersectionLengths->GetNumberOfTuples();

  for (vtkIdType i = 0; i < numIntersections; i++)
    {
    float newcolor[4];
    float *c;
    c = this->GetTableEntry(nearIntersections->GetComponent(i, 0),
                            farIntersections->GetComponent(i, 0),
                            intersectionLengths->GetComponent(i, 0), 0);
    newcolor[0] = c[0];  newcolor[1] = c[1];
    newcolor[2] = c[2];  newcolor[3] = c[3];
    for (int component = 1; component < this->NumComponents; component++)
      {
      c = this->GetTableEntry(nearIntersections->GetComponent(i, component),
                              farIntersections->GetComponent(i, component),
                              intersectionLengths->GetComponent(i, 0),
                              component);
      // The blending I'm using is a combination of porter and duff xors
      // and ins.
      float coef1=1-0.5f*c[3];
      float coef2=1-0.5f*newcolor[3];
      newcolor[0] = newcolor[0]*coef1 + c[0]*coef2;
      newcolor[1] = newcolor[1]*coef1 + c[1]*coef2;
      newcolor[2] = newcolor[2]*coef1 + c[2]*coef2;
      newcolor[3] = newcolor[3]*coef1 + c[3]*coef2;
      }

    float coef=1-color[3];
    color[0] += newcolor[0]*coef;
    color[1] += newcolor[1]*coef;
    color[2] += newcolor[2]*coef;
    color[3] += newcolor[3]*coef;
    }
}

