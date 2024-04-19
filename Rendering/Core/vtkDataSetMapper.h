// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDataSetMapper
 * @brief   map vtkDataSet and derived classes to graphics primitives
 *
 * vtkDataSetMapper is a mapper to map data sets (i.e., vtkDataSet and
 * all derived classes) to graphics primitives. The mapping procedure
 * is as follows: all 0D, 1D, and 2D cells are converted into points,
 * lines, and polygons/triangle strips and then mapped to the graphics
 * system. The 2D faces of 3D cells are mapped only if they are used by
 * only one cell, i.e., on the boundary of the data set.
 */

#ifndef vtkDataSetMapper_h
#define vtkDataSetMapper_h

#include "vtkMapper.h"
#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkWrappingHints.h"       // For VTK_MARSHALMANUAL

VTK_ABI_NAMESPACE_BEGIN
class vtkPolyDataMapper;
class vtkDataSetSurfaceFilter;

class VTKRENDERINGCORE_EXPORT VTK_MARSHALMANUAL vtkDataSetMapper : public vtkMapper
{
public:
  static vtkDataSetMapper* New();
  vtkTypeMacro(vtkDataSetMapper, vtkMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  void Render(vtkRenderer* ren, vtkActor* act) override;

  ///@{
  /**
   * Get the internal poly data mapper used to map data set to graphics system.
   */
  vtkGetObjectMacro(PolyDataMapper, vtkPolyDataMapper);
  ///@}

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow*) override;

  /**
   * Get the mtime also considering the lookup table.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Set the Input of this mapper.
   */
  void SetInputData(vtkDataSet* input);
  vtkDataSet* GetInput();
  ///@}

protected:
  vtkDataSetMapper();
  ~vtkDataSetMapper() override;

  vtkDataSetSurfaceFilter* GeometryExtractor;
  vtkPolyDataMapper* PolyDataMapper;

  void ReportReferences(vtkGarbageCollector*) override;

  // see algorithm for more info
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkDataSetMapper(const vtkDataSetMapper&) = delete;
  void operator=(const vtkDataSetMapper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
