/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataSetMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkMapper.h"

class vtkPolyDataMapper;
class vtkDataSetSurfaceFilter;

class VTKRENDERINGCORE_EXPORT vtkDataSetMapper : public vtkMapper
{
public:
  static vtkDataSetMapper *New();
  vtkTypeMacro(vtkDataSetMapper, vtkMapper);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  void Render(vtkRenderer *ren, vtkActor *act) VTK_OVERRIDE;

  //@{
  /**
   * Get the internal poly data mapper used to map data set to graphics system.
   */
  vtkGetObjectMacro(PolyDataMapper, vtkPolyDataMapper);
  //@}

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(vtkWindow *) VTK_OVERRIDE;

  /**
   * Get the mtime also considering the lookup table.
   */
  vtkMTimeType GetMTime() VTK_OVERRIDE;

  //@{
  /**
   * Set the Input of this mapper.
   */
  void SetInputData(vtkDataSet *input);
  vtkDataSet *GetInput();
  //@}

protected:
  vtkDataSetMapper();
  ~vtkDataSetMapper() VTK_OVERRIDE;

  vtkDataSetSurfaceFilter *GeometryExtractor;
  vtkPolyDataMapper *PolyDataMapper;

  void ReportReferences(vtkGarbageCollector*) VTK_OVERRIDE;

  // see algorithm for more info
  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

private:
  vtkDataSetMapper(const vtkDataSetMapper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDataSetMapper&) VTK_DELETE_FUNCTION;
};

#endif
