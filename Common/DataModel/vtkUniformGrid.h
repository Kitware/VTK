// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkUniformGrid
 * @brief   image data with blanking
 *
 * vtkUniformGrid is a subclass of vtkImageData. In addition to all
 * the image data functionality, it supports blanking.
 */

#ifndef vtkUniformGrid_h
#define vtkUniformGrid_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkImageData.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkEmptyCell;
class vtkUnsignedCharArray;
class vtkAMRBox;

class VTKCOMMONDATAMODEL_EXPORT vtkUniformGrid : public vtkImageData
{
public:
  ///@{
  /**
   * Construct an empty uniform grid.
   */
  static vtkUniformGrid* New();
  vtkTypeMacro(vtkUniformGrid, vtkImageData);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Copy the geometric and topological structure of an input image data
   * object.
   */
  void CopyStructure(vtkDataSet* ds) override;

  /**
   * Return what type of dataset this is.
   */
  int GetDataObjectType() VTK_FUTURE_CONST override { return VTK_UNIFORM_GRID; }

  ///@{
  /**
   * Standard vtkDataSet API methods. See vtkDataSet for more information.
   */
  void Initialize() override;
  ///@}

  /**
   * Initialize with no ghost cell arrays, from the definition in
   * the given box. The box is expected to be 3D, if you have 2D
   * data the set the third dimensions 0. eg. (X,X,0)(X,X,0)
   * Returns 0 if the initialization failed.
   */
  int Initialize(const vtkAMRBox* def, double* origin, double* spacing);
  /**
   * Initialize from the definition in the given box, with ghost cell
   * arrays nGhosts cells thick in all directions. The box is expected
   * to be 3D, if you have 2D data the set the third dimensions 0.
   * eg. (X,X,0)(X,X,0)
   * Returns 0 if the initialization failed.
   */
  int Initialize(const vtkAMRBox* def, double* origin, double* spacing, int nGhosts);

  /**
   * Initialize from the definition in the given box, with ghost cell
   * arrays of the thickness given in each direction by "nGhosts" array.
   * The box and ghost array are expected to be 3D, if you have 2D data
   * the set the third dimensions 0. eg. (X,X,0)(X,X,0)
   * Returns 0 if the initialization failed.
   */
  int Initialize(const vtkAMRBox* def, double* origin, double* spacing, const int nGhosts[3]);
  /**
   * Construct a uniform grid, from the definition in the given box
   * "def", with ghost cell arrays of the thickness given in each
   * direction by "nGhosts*". The box and ghost array are expected
   * to be 3D, if you have 2D data the set the third dimensions 0. eg.
   * (X,X,0)(X,X,0)
   * Returns 0 if the initialization failed.
   */
  int Initialize(const vtkAMRBox* def, double* origin, double* spacing, int nGhostsI, int nGhostsJ,
    int nGhostsK);

  virtual VTK_NEWINSTANCE vtkImageData* NewImageDataCopy();

  ///@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkUniformGrid* GetData(vtkInformation* info);
  static vtkUniformGrid* GetData(vtkInformationVector* v, int i = 0);
  ///@}

protected:
  vtkUniformGrid();
  ~vtkUniformGrid() override;

  /**
   * Override this method because of blanking.
   */
  void ComputeScalarRange() override;

private:
  vtkUniformGrid(const vtkUniformGrid&) = delete;
  void operator=(const vtkUniformGrid&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
