/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPolyDataMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCellGridMapper
 * @brief   map a vtkCellGrid to graphics primitives.
 *
 * This mapper is a prototype to help enhance VTK visualization capabilities for
 * Discontinuous Galerkin fields.
 * For now, it requires a `vtkDataObject` input.
 *
 * A vtkCellGrid does not couple the fields with geometric degrees of freedom, unlike
 * vtkDataSet. This is useful to render Discontinuous Galerkin fields where two points
 * sharing a common face might not have the same field value (discontinuous).
 */

#ifndef vtkCellGridMapper_h
#define vtkCellGridMapper_h

#include "vtkMapper.h"
#include "vtkRenderingCoreModule.h" // For export macro.
#include "vtkUnsignedCharArray.h"   // For API.

VTK_ABI_NAMESPACE_BEGIN
class vtkCellGrid;
class vtkCellAttribute;
class vtkDataObject;
class vtkRenderer;

class VTKRENDERINGCORE_EXPORT vtkCellGridMapper : public vtkMapper
{
public:
  static vtkCellGridMapper* New();
  vtkTypeMacro(vtkCellGridMapper, vtkMapper);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkSetMacro(VisualizePCoords, int);
  vtkGetMacro(VisualizePCoords, int);

  vtkSetMacro(VisualizeBasisFunction, int);
  vtkGetMacro(VisualizeBasisFunction, int);

  /**
   * Implemented by sub classes. Actual rendering is done here.
   */
  void Render(vtkRenderer*, vtkActor*) override {}

  ///@{
  /**
   * Specify the input data to map.
   */
  void SetInputData(vtkCellGrid* in);
  vtkCellGrid* GetInput();
  ///@}

  ///@{
  /**
   * Bring this algorithm's outputs up-to-date.
   */
  void Update(int port) override;
  void Update() override;
  vtkTypeBool Update(int port, vtkInformationVector* requests) override;
  vtkTypeBool Update(vtkInformation* requests) override;
  ///@}

  /**
   * Return bounding box (array of six doubles) of data expressed as
   * (xmin,xmax, ymin,ymax, zmin,zmax).
   */
  double* GetBounds() VTK_SIZEHINT(6) override;
  void GetBounds(double bounds[6]) override { this->Superclass::GetBounds(bounds); }

protected:
  vtkCellGridMapper();
  ~vtkCellGridMapper() override = default;

  void ComputeBounds();
  int FillInputPortInformation(int, vtkInformation*) override;

  /// Fill this->ColorTextureMap with an image using this->LookupTable.
  void CreateColormapTexture();
  /// Return the attribute to color the \a input with.
  virtual vtkCellAttribute* GetColorAttribute(vtkCellGrid* input) const;

  int VisualizePCoords = -1;
  int VisualizeBasisFunction = -1;

private:
  vtkCellGridMapper(const vtkCellGridMapper&) = delete;
  void operator=(const vtkCellGridMapper&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellGridMapper_h
