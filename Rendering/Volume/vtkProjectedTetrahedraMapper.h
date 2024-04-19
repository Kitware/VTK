// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2003 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov

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

VTK_ABI_NAMESPACE_BEGIN
class vtkFloatArray;
class vtkPoints;
class vtkUnsignedCharArray;
class vtkVisibilitySort;
class vtkVolumeProperty;
class vtkRenderWindow;

class VTKRENDERINGVOLUME_EXPORT vtkProjectedTetrahedraMapper
  : public vtkUnstructuredGridVolumeMapper
{
public:
  vtkTypeMacro(vtkProjectedTetrahedraMapper, vtkUnstructuredGridVolumeMapper);
  static vtkProjectedTetrahedraMapper* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  virtual void SetVisibilitySort(vtkVisibilitySort* sort);
  vtkGetObjectMacro(VisibilitySort, vtkVisibilitySort);

  static void MapScalarsToColors(
    vtkDataArray* colors, vtkVolumeProperty* property, vtkDataArray* scalars);
  static void TransformPoints(vtkPoints* inPoints, const float projection_mat[16],
    const float modelview_mat[16], vtkFloatArray* outPoints);

  /**
   * Return true if the rendering context provides
   * the nececessary functionality to use this class.
   */
  virtual bool IsSupported(vtkRenderWindow*) { return false; }

protected:
  vtkProjectedTetrahedraMapper();
  ~vtkProjectedTetrahedraMapper() override;

  vtkVisibilitySort* VisibilitySort;

  /**
   * The visibility sort will probably make a reference loop by holding a
   * reference to the input.
   */
  void ReportReferences(vtkGarbageCollector* collector) override;

private:
  vtkProjectedTetrahedraMapper(const vtkProjectedTetrahedraMapper&) = delete;
  void operator=(const vtkProjectedTetrahedraMapper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
