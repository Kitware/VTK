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

/**
 * @class   vtkProjectedTetrahedraMapper
 * @brief   Unstructured grid volume renderer.
 *
 *
 * vtkProjectedTetrahedraMapper is an implementation of the classic
 * Projected Tetrahedra algorithm presented by Shirley and Tuchman in "A
 * Polygonal Approximation to Direct Scalar Volume Rendering" in Computer
 * Graphics, December 1990.
 *
 * @bug
 * This mapper relies highly on the implementation of the OpenGL pipeline.
 * A typical hardware driver has lots of options and some settings can
 * cause this mapper to produce artifacts.
 *
*/

#ifndef vtkProjectedTetrahedraMapper_h
#define vtkProjectedTetrahedraMapper_h

#include "vtkRenderingVolumeModule.h" // For export macro
#include "vtkUnstructuredGridVolumeMapper.h"

class vtkFloatArray;
class vtkPoints;
class vtkUnsignedCharArray;
class vtkVisibilitySort;
class vtkVolumeProperty;
class vtkRenderWindow;

class VTKRENDERINGVOLUME_EXPORT vtkProjectedTetrahedraMapper : public vtkUnstructuredGridVolumeMapper
{
public:
  vtkTypeMacro(vtkProjectedTetrahedraMapper,
                       vtkUnstructuredGridVolumeMapper);
  static vtkProjectedTetrahedraMapper *New();
  virtual void PrintSelf(ostream &os, vtkIndent indent);

  virtual void SetVisibilitySort(vtkVisibilitySort *sort);
  vtkGetObjectMacro(VisibilitySort, vtkVisibilitySort);

  static void MapScalarsToColors(vtkDataArray *colors,
                                 vtkVolumeProperty *property,
                                 vtkDataArray *scalars);
  static void TransformPoints(vtkPoints *inPoints,
                              const float projection_mat[16],
                              const float modelview_mat[16],
                              vtkFloatArray *outPoints);

  /**
   * Return true if the rendering context provides
   * the nececessary functionality to use this class.
   */
  virtual bool IsSupported(vtkRenderWindow *)
    { return false; }

protected:
  vtkProjectedTetrahedraMapper();
  ~vtkProjectedTetrahedraMapper();

  vtkVisibilitySort *VisibilitySort;

  /**
   * The visibility sort will probably make a reference loop by holding a
   * reference to the input.
   */
  void ReportReferences(vtkGarbageCollector *collector) VTK_OVERRIDE;

private:
  vtkProjectedTetrahedraMapper(const vtkProjectedTetrahedraMapper &) VTK_DELETE_FUNCTION;
  void operator=(const vtkProjectedTetrahedraMapper &) VTK_DELETE_FUNCTION;
};

#endif
