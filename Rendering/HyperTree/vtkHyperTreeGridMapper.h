/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHyperTreeGridMapper
 * @brief   map vtkHyperTreeGrid to graphics primitives
 *
 * vtkHyperTreeGridMapper is a class that maps polygonal data (i.e., vtkHyperTreeGrid)
 * to graphics primitives. vtkHyperTreeGridMapper serves as a superclass for
 * device-specific poly data mappers, that actually do the mapping to the
 * rendering/graphics hardware/software.
 */

#ifndef vtkHyperTreeGridMapper_h
#define vtkHyperTreeGridMapper_h

#include "vtkOpenGLPolyDataMapper.h"
#include "vtkPolyDataMapper.h"           // For internal mapper
#include "vtkRenderingHyperTreeModule.h" // For export macro
#include "vtkSmartPointer.h"             // For vtkSmartPointer

class vtkAdaptiveDataSetSurfaceFilter;
class vtkHyperTreeGrid;
class vtkHyperTreeGridGeometry;
class vtkRenderer;
class vtkRenderWindow;

class VTKRENDERINGHYPERTREE_EXPORT vtkHyperTreeGridMapper : public vtkOpenGLPolyDataMapper
{
public:
  static vtkHyperTreeGridMapper* New();
  vtkTypeMacro(vtkHyperTreeGridMapper, vtkOpenGLPolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  using Superclass::SetInputConnection;
  void SetInputConnection(vtkAlgorithmOutput* input) override;

  /**
   * This calls RenderPiece (in a for loop if streaming is necessary).
   */
  void Render(vtkRenderer* ren, vtkActor* act) override;

  //@{
  /**
   * Specify the input data to map.
   */
  void SetInputData(vtkHyperTreeGrid* in);
  vtkPolyData* GetInput() override;
  //@}

  using Superclass::Update;
  void Update(int port) override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

protected:
  vtkHyperTreeGridMapper();
  ~vtkHyperTreeGridMapper() override = default;

private:
  vtkSmartPointer<vtkHyperTreeGridGeometry> GeometryFilter;
  vtkSmartPointer<vtkAdaptiveDataSetSurfaceFilter> Adaptive2DGeometryFilter;
  bool UseLOD;

  vtkHyperTreeGridMapper(const vtkHyperTreeGridMapper&) = delete;
  void operator=(const vtkHyperTreeGridMapper&) = delete;
};

#endif
