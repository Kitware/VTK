/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridHomogeneousRayIntegrator.cxx

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

#include "vtkUnstructuredGridHomogeneousRayIntegrator.h"

#include "vtkObjectFactory.h"
#include "vtkVolumeProperty.h"
#include "vtkVolume.h"
#include "vtkAbstractVolumeMapper.h"
#include "vtkUnstructuredGrid.h"
#include "vtkDoubleArray.h"
#include "vtkColorTransferFunction.h"
#include "vtkPiecewiseFunction.h"

#include <cmath>

//-----------------------------------------------------------------------------

vtkStandardNewMacro(vtkUnstructuredGridHomogeneousRayIntegrator);

//-----------------------------------------------------------------------------

vtkUnstructuredGridHomogeneousRayIntegrator::vtkUnstructuredGridHomogeneousRayIntegrator()
{
  this->Property = NULL;

  this->NumComponents = 0;
  this->ColorTable = NULL;
  this->AttenuationTable = NULL;
  this->TableShift = NULL;
  this->TableScale = NULL;

  this->UseAverageColor = 0;
  this->TransferFunctionTableSize = 1024;
}

vtkUnstructuredGridHomogeneousRayIntegrator::~vtkUnstructuredGridHomogeneousRayIntegrator()
{
  for (int i = 0; i < this->NumComponents; i++)
  {
    delete[] this->ColorTable[i];
    delete[] this->AttenuationTable[i];
  }
  delete[] this->ColorTable;
  delete[] this->AttenuationTable;
  delete[] this->TableShift;
  delete[] this->TableScale;
}

void vtkUnstructuredGridHomogeneousRayIntegrator::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "UseAverageColor: " << this->UseAverageColor << endl;
  os << indent << "TransferFunctionTableSize: "
     << this->TransferFunctionTableSize << endl;
}

//-----------------------------------------------------------------------------

void vtkUnstructuredGridHomogeneousRayIntegrator::GetTransferFunctionTables(vtkDataArray *scalars)
{
  for (int i = 0; i < this->NumComponents; i++)
  {
    delete[] this->ColorTable[i];
    delete[] this->AttenuationTable[i];
  }
  delete[] this->ColorTable;
  delete[] this->AttenuationTable;
  delete[] this->TableShift;
  delete[] this->TableScale;

  this->NumComponents = scalars->GetNumberOfComponents();

  this->ColorTable = new float*[this->NumComponents];
  this->AttenuationTable = new float*[this->NumComponents];
  this->TableShift = new double[this->NumComponents];
  this->TableScale = new double[this->NumComponents];

  for (int c = 0; c < this->NumComponents; c++)
  {
    double range[2];
    scalars->GetRange(range, c);
    if (range[0] >= range[1])
    {
      range[1] = range[0] + 1;
    }
    this->TableScale[c] = this->TransferFunctionTableSize/(range[1]-range[0]);
    this->TableShift[c]
      = -range[0]*this->TransferFunctionTableSize/(range[1]-range[0]);

    this->ColorTable[c] = new float[3*this->TransferFunctionTableSize];
    if (this->Property->GetColorChannels(c) == 1)
    {
      // Get gray values.  Store temporarily in allocated RGB array.
      this->Property->GetGrayTransferFunction(c)
        ->GetTable(range[0], range[1], this->TransferFunctionTableSize,
                   this->ColorTable[c]);
      // Convert gray into RGB.  Copy backward so that we can use the same
      // array.
      for (int i = this->TransferFunctionTableSize-1; i >= 0; i--)
      {
        this->ColorTable[c][3*i + 0]
          = this->ColorTable[c][3*i + 1]
          = this->ColorTable[c][3*i + 2] = this->ColorTable[c][i];
      }
    }
    else
    {
      this->Property->GetRGBTransferFunction(c)
        ->GetTable(range[0], range[1], this->TransferFunctionTableSize,
                   this->ColorTable[c]);
    }

    this->AttenuationTable[c] = new float[this->TransferFunctionTableSize];
    this->Property->GetScalarOpacity(c)
      ->GetTable(range[0], range[1], this->TransferFunctionTableSize,
                 this->AttenuationTable[c]);

    // Adjust attenuation by scalar unit length.  This will make the unit
    // lenth the same as the model.
    float unitlength = this->Property->GetScalarOpacityUnitDistance(c);
    for (int i = 0; i < this->TransferFunctionTableSize; i++)
    {
      this->AttenuationTable[c][i] /= unitlength;
    }
  }

  this->TablesBuilt.Modified();
}

//-----------------------------------------------------------------------------

void vtkUnstructuredGridHomogeneousRayIntegrator::Initialize(vtkVolume *volume,
                                                   vtkDataArray *scalars)
{
  vtkVolumeProperty *property = volume->GetProperty();

  if (   (property == this->Property)
      && (this->TablesBuilt > property->GetMTime())
      && (this->TablesBuilt > this->MTime) )
  {
    // Nothing changed from the last time Initialize was run.
    return;
  }

  this->Property = property;
  this->Volume = volume;

  if (property->GetIndependentComponents())
  {
    this->GetTransferFunctionTables(scalars);
  }
}

//-----------------------------------------------------------------------------

void vtkUnstructuredGridHomogeneousRayIntegrator::Integrate(
                                     vtkDoubleArray *intersectionLengths,
                                     vtkDataArray *nearIntersections,
                                     vtkDataArray *vtkNotUsed(farIntersections),
                                     float color[4])
{
  vtkIdType numIntersections = intersectionLengths->GetNumberOfTuples();

  if (this->Property->GetIndependentComponents())
  {
    if (this->NumComponents == 1)
    {
      // Optimize for what I think is one of the most common uses.
      for (vtkIdType i = 0; i < numIntersections; i++)
      {
        int table_index
          = (int)(  this->TableScale[0]*nearIntersections->GetComponent(i, 0)
                  + this->TableShift[0] );
        if (table_index < 0)
        {
          table_index = 0;
        }
        if (table_index >= this->TransferFunctionTableSize)
        {
          table_index = this->TransferFunctionTableSize-1;
        }
        float *c = this->ColorTable[0] + 3*table_index;
        float tau = this->AttenuationTable[0][table_index];
        float alpha = 1-(float)exp(-intersectionLengths->GetComponent(i,0)*tau);
        color[0] += c[0]*alpha*(1-color[3]);
        color[1] += c[1]*alpha*(1-color[3]);
        color[2] += c[2]*alpha*(1-color[3]);
        color[3] += alpha*(1-color[3]);
      }
    }
    else
    {
      // Generic case.
      for (vtkIdType i = 0; i < numIntersections; i++)
      {
        float newcolor[4];
        int table_index
          = (int)(  this->TableScale[0]*nearIntersections->GetComponent(i, 0)
                  + this->TableShift[0] );
        if (table_index < 0)
        {
          table_index = 0;
        }
        if (table_index >= this->TransferFunctionTableSize)
        {
          table_index = this->TransferFunctionTableSize-1;
        }
        float *c = this->ColorTable[0] + 3*table_index;
        float tau = this->AttenuationTable[0][table_index];
        newcolor[0] = c[0];  newcolor[1] = c[1];
        newcolor[2] = c[2];  newcolor[3] = tau;
        for (int component = 1; component < this->NumComponents; component++)
        {
          table_index
            = (int)(  this->TableScale[component]
                      *nearIntersections->GetComponent(i, component)
                    + this->TableShift[component] );
          if (table_index < 0)
          {
            table_index = 0;
          }
          if (table_index >= this->TransferFunctionTableSize)
          {
            table_index = this->TransferFunctionTableSize-1;
          }
          c = this->ColorTable[component] + 3*table_index;
          tau = this->AttenuationTable[component][table_index];
          // Here we handle the mixing of material properties.  This never
          // seems to be defined very clearly.  I handle this by assuming
          // that each scalar represents a cloud of particles of a certain
          // color and a certain density.  We mix the scalars in the same
          // way as mixing these particles together.  By necessity, the
          // density becomes greater.  The "opacity" parameter is really
          // interpreted as the attenuation coefficient (which is
          // proportional to density) and can therefore easily be greater
          // than one.  The opacity of the resulting color will, however,
          // always be scaled between 0 and 1.
          if (tau + newcolor[3] > 1.0e-8f)
          {
            newcolor[0] *= newcolor[3]/(tau + newcolor[3]);
            newcolor[1] *= newcolor[3]/(tau + newcolor[3]);
            newcolor[2] *= newcolor[3]/(tau + newcolor[3]);
            newcolor[0] += c[0]*tau/(tau + newcolor[3]);
            newcolor[1] += c[1]*tau/(tau + newcolor[3]);
            newcolor[2] += c[2]*tau/(tau + newcolor[3]);
            newcolor[3] += tau;
          }
        }
        float alpha = 1 - (float)exp(-intersectionLengths->GetComponent(i,0)
                                     *newcolor[3]);
        color[0] += newcolor[0]*alpha*(1-color[3]);
        color[1] += newcolor[1]*alpha*(1-color[3]);
        color[2] += newcolor[2]*alpha*(1-color[3]);
        color[3] += alpha*(1-color[3]);
      }
    }
  }
  else
  {
    int numComponents = nearIntersections->GetNumberOfComponents();
    for (vtkIdType i = 0; i < numIntersections; i++)
    {
      double c[4];
      if (numComponents == 4)
      {
        nearIntersections->GetTuple(i, c);
      }
      else
      {
        double *lt = nearIntersections->GetTuple(i);
        c[0] = c[1] = c[2] = lt[0];
        c[3] = lt[1];
      }
      float alpha = 1-(float)exp(-intersectionLengths->GetComponent(i,0)*c[3]);
      color[0] += (float)c[0]*alpha*(1-color[3]);
      color[1] += (float)c[1]*alpha*(1-color[3]);
      color[2] += (float)c[2]*alpha*(1-color[3]);
      color[3] += alpha*(1-color[3]);
    }
  }
}

