/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkUnstructuredGridHomogeneousRayIntegrator.h

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

// .NAME vtkUnstructuredGridHomogeneousRayIntegrator - performs peicewise constant ray integration.
//
// .SECTION Description
//
// vtkUnstructuredGridHomogeneousRayIntegrator performs homogeneous ray
// integration.  This is a good method to use when volume rendering scalars
// that are defined on cells.
//

#ifndef __vtkUnstructuredGridHomogeneousRayIntegrator_h
#define __vtkUnstructuredGridHomogeneousRayIntegrator_h

#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkUnstructuredGridVolumeRayIntegrator.h"

class vtkVolumeProperty;

class VTKRENDERINGVOLUME_EXPORT vtkUnstructuredGridHomogeneousRayIntegrator : public vtkUnstructuredGridVolumeRayIntegrator
{
public:
  vtkTypeMacro(vtkUnstructuredGridHomogeneousRayIntegrator,
                       vtkUnstructuredGridVolumeRayIntegrator);
  static vtkUnstructuredGridHomogeneousRayIntegrator *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  virtual void Initialize(vtkVolume *volume, vtkDataArray *scalars);

  virtual void Integrate(vtkDoubleArray *intersectionLengths,
                         vtkDataArray *nearIntersections,
                         vtkDataArray *farIntersections,
                         float color[4]);

  // Description:
  // For quick lookup, the transfer function is sampled into a table.
  // This parameter sets how big of a table to use.  By default, 1024
  // entries are used.
  vtkSetMacro(TransferFunctionTableSize, int);
  vtkGetMacro(TransferFunctionTableSize, int);

protected:
  vtkUnstructuredGridHomogeneousRayIntegrator();
  ~vtkUnstructuredGridHomogeneousRayIntegrator();

  vtkVolume *Volume;
  vtkVolumeProperty *Property;

  int      NumComponents;
  float  **ColorTable;
  float  **AttenuationTable;
  double  *TableShift;
  double  *TableScale;
  vtkTimeStamp TablesBuilt;

  int UseAverageColor;
  int TransferFunctionTableSize;

  virtual void GetTransferFunctionTables(vtkDataArray *scalars);

private:
  vtkUnstructuredGridHomogeneousRayIntegrator(const vtkUnstructuredGridHomogeneousRayIntegrator&);  // Not implemented.
  void operator=(const vtkUnstructuredGridHomogeneousRayIntegrator&);  // Not implemented
};

#endif //__vtkUnstructuredGridHomogeneousRayIntegrator_h
