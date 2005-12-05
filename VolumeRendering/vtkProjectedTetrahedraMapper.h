/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkProjectedTetrahedraMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2003 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

// .NAME vtkProjectedTetrahedraMapper - Unstructured grid volume renderer.
//
// .SECTION Description
// vtkProjectedTetrahedraMapper is an implementation of the classic
// Projected Tetrahedra algorithm presented by Shirley and Tuchman in "A
// Polygonal Approximation to Direct Scalar Volume Rendering" in Computer
// Graphics, December 1990.
//
// .SECTION Bugs
// This mapper relies highly on the implementation of the OpenGL pipeline.
// A typically hardware driver has lots of options and some settings can
// cause this mapper to produce artifacts.
//

#ifndef __vtkProjectedTetrahedraMapper_h
#define __vtkProjectedTetrahedraMapper_h

#include "vtkUnstructuredGridVolumeMapper.h"

class vtkVisibilitySort;
class vtkUnsignedCharArray;
class vtkFloatArray;

class VTK_VOLUMERENDERING_EXPORT vtkProjectedTetrahedraMapper : public vtkUnstructuredGridVolumeMapper
{
public:
  vtkTypeRevisionMacro(vtkProjectedTetrahedraMapper,
                       vtkUnstructuredGridVolumeMapper);
  static vtkProjectedTetrahedraMapper *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  virtual void SetVisibilitySort(vtkVisibilitySort *sort);
  vtkGetObjectMacro(VisibilitySort, vtkVisibilitySort);

  virtual void Render(vtkRenderer *renderer, vtkVolume *volume);

  virtual void ReleaseGraphicsResources(vtkWindow *window);

  static void MapScalarsToColors(vtkDataArray *colors, vtkVolume *volume,
                                 vtkDataArray *scalars);

protected:
  vtkProjectedTetrahedraMapper();
  ~vtkProjectedTetrahedraMapper();

  vtkUnsignedCharArray *Colors;
  int UsingCellColors;

  vtkFloatArray *TransformedPoints;

  float MaxCellSize;
  vtkTimeStamp InputAnalyzedTime;
  vtkTimeStamp OpacityTextureTime;
  vtkTimeStamp ColorsMappedTime;

  unsigned int OpacityTexture;

  vtkVisibilitySort *VisibilitySort;

  int GaveError;

  vtkVolume *LastVolume;

  virtual void ProjectTetrahedra(vtkRenderer *renderer, vtkVolume *volume);

  // Description:
  // The visibility sort will probably make a reference loop by holding a
  // reference to the input.
  virtual void ReportReferences(vtkGarbageCollector *collector);

private:
  vtkProjectedTetrahedraMapper(const vtkProjectedTetrahedraMapper &);  // Not Implemented.
  void operator=(const vtkProjectedTetrahedraMapper &);  // Not Implemented.
};

#endif
