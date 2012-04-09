/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridPreIntegration.h

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

// .NAME vtkUnstructuredGridPreIntegration - performs ray integration with pre-integration tables.
//
// .SECTION Description
//
// vtkUnstructuredGridPreIntegration performs ray integration by looking
// into a precomputed table.  The result should be equivalent to that
// computed by vtkUnstructuredGridLinearRayIntegrator and
// vtkUnstructuredGridPartialPreIntegration, but faster than either one.
// The pre-integration algorithm was first introduced by Roettger, Kraus,
// and Ertl in "Hardware-Accelerated Volume And Isosurface Rendering Based
// On Cell-Projection."
//
// Due to table size limitations, a table can only be indexed by
// independent scalars.  Thus, dependent scalars are not supported.
//

#ifndef __vtkUnstructuredGridPreIntegration_h
#define __vtkUnstructuredGridPreIntegration_h

#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkUnstructuredGridVolumeRayIntegrator.h"

class vtkVolumeProperty;

class VTKRENDERINGVOLUME_EXPORT vtkUnstructuredGridPreIntegration : public vtkUnstructuredGridVolumeRayIntegrator
{
public:
  vtkTypeMacro(vtkUnstructuredGridPreIntegration,
                       vtkUnstructuredGridVolumeRayIntegrator);
  static vtkUnstructuredGridPreIntegration *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  virtual void Initialize(vtkVolume *volume, vtkDataArray *scalars);

  virtual void Integrate(vtkDoubleArray *intersectionLengths,
                         vtkDataArray *nearIntersections,
                         vtkDataArray *farIntersections,
                         float color[4]);

  // Description:
  // The class used to fill the pre integration table.  By default, a
  // vtkUnstructuredGridPartialPreIntegration is built.
  vtkGetObjectMacro(Integrator, vtkUnstructuredGridVolumeRayIntegrator);
  virtual void SetIntegrator(vtkUnstructuredGridVolumeRayIntegrator *);

  // Description:
  // Set/Get the size of the integration table built.
  vtkSetMacro(IntegrationTableScalarResolution, int);
  vtkGetMacro(IntegrationTableScalarResolution, int);
  vtkSetMacro(IntegrationTableLengthResolution, int);
  vtkGetMacro(IntegrationTableLengthResolution, int);

  // Description::
  // Get how an integration table is indexed.
  virtual double GetIntegrationTableScalarShift(int component = 0);
  virtual double GetIntegrationTableScalarScale(int component = 0);
  virtual double GetIntegrationTableLengthScale();

  // Description:
  // Get/set whether to use incremental pre-integration (by default it's
  // on).  Incremental pre-integration is much faster but can introduce
  // error due to numerical imprecision.  Under most circumstances, the
  // error is not noticeable.
  vtkGetMacro(IncrementalPreIntegration, int);
  vtkSetMacro(IncrementalPreIntegration, int);
  vtkBooleanMacro(IncrementalPreIntegration, int);

  // Description:
  // Get the partial pre-integration table for the given scalar component.
  // The tables are built when Initialize is called.  A segment of length d
  // with a front scalar of sf and a back scalar of sb is referenced in the
  // resulting table as 4 * ((l * \c IntegrationTableLengthScale) * \c
  // IntegrationTableScalarResolution * \c IntegrationTableScalarResolution
  // + (sb * \c IntegrationTableScalarScale + \c
  // IntegrationTableScalarShift) * \c IntegrationTableScalarResolution
  // + (sf * \c IntegrationTableScalarScale + \c
  // IntegrationTableScalarShift)).
  virtual float *GetPreIntegrationTable(int component = 0);

  // Description:
  // Get an entry (RGBA) in one of the pre-integration tables.  The tables
  // are built when Intialize is called.
  float *GetTableEntry(double scalar_front, double scalar_back, double lenth,
                       int component = 0);

  // Description:
  // Like GetTableEntry, except the inputs are scaled indices into the table
  // rather than than the actual scalar and length values.  Use GetTableEntry
  // unless you are really sure you know what you are doing.
  float *GetIndexedTableEntry(int scalar_front_index, int scalar_back_index,
                              int length_index, int component = 0);

protected:
  vtkUnstructuredGridPreIntegration();
  ~vtkUnstructuredGridPreIntegration();

  vtkUnstructuredGridVolumeRayIntegrator *Integrator;

  vtkVolume *Volume;
  vtkVolumeProperty *Property;
  double MaxLength;

  int      NumComponents;
  float  **IntegrationTable;
  double  *IntegrationTableScalarShift;
  double  *IntegrationTableScalarScale;
  double   IntegrationTableLengthScale;
  vtkTimeStamp IntegrationTableBuilt;

  int IntegrationTableScalarResolution;
  int IntegrationTableLengthResolution;

  int IncrementalPreIntegration;

  virtual void BuildPreIntegrationTables(vtkDataArray *scalars);

private:
  vtkUnstructuredGridPreIntegration(const vtkUnstructuredGridPreIntegration&);  // Not implemented.
  void operator=(const vtkUnstructuredGridPreIntegration&);  // Not implemented
};

inline float *vtkUnstructuredGridPreIntegration::GetIndexedTableEntry(
                                                         int scalar_front_index,
                                                         int scalar_back_index,
                                                         int length_index,
                                                         int component)
{
  // Snap entries to bounds.  I don't really want to spend cycles doing
  // this, but I've had the ray caster give me values that are noticeably
  // out of bounds.
  if (scalar_front_index < 0) scalar_front_index = 0;
  if (scalar_front_index >= this->IntegrationTableScalarResolution)
    scalar_front_index = this->IntegrationTableScalarResolution - 1;
  if (scalar_back_index < 0) scalar_back_index = 0;
  if (scalar_back_index >= this->IntegrationTableScalarResolution)
    scalar_back_index = this->IntegrationTableScalarResolution - 1;
  if (length_index < 0) length_index = 0;
  if (length_index >= this->IntegrationTableLengthResolution)
    length_index = this->IntegrationTableLengthResolution - 1;

  return (  this->IntegrationTable[component]
          + 4*(  (  (  length_index*this->IntegrationTableScalarResolution
                     + scalar_back_index)
                  * this->IntegrationTableScalarResolution)
               + scalar_front_index));
}

inline float *vtkUnstructuredGridPreIntegration::GetTableEntry(
  double scalar_front, double scalar_back, double length, int component)
{
  int sfi = static_cast<int>(  scalar_front
                    *this->IntegrationTableScalarScale[component]
                  + this->IntegrationTableScalarShift[component] + 0.5);
  int sbi =  static_cast<int>(  scalar_back*this->IntegrationTableScalarScale[component]
                  + this->IntegrationTableScalarShift[component] + 0.5);
  int li =  static_cast<int>(length*this->IntegrationTableLengthScale + 0.5);
  return this->GetIndexedTableEntry(sfi, sbi, li, component);
}

#endif //__vtkUnstructuredGridPreIntegration_h
