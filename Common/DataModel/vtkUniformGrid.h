// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkUniformGrid
 * @brief   Deprecated vtkImageData
 *
 * vtkUniformGrid is an empty subclass of vtkImageData that will be deprecated
 */

#ifndef vtkUniformGrid_h
#define vtkUniformGrid_h

#include "vtkAMRBox.h"                // Fox vtkAMRBox
#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkDeprecation.h"           // For VTK_DEPRECATED_IN_9_6_0
#include "vtkImageData.h"
#include "vtkWrappingHints.h" // For VTK_MARSHALAUTO

VTK_ABI_NAMESPACE_BEGIN
class vtkEmptyCell;
class vtkUnsignedCharArray;

class VTKCOMMONDATAMODEL_EXPORT VTK_MARSHALAUTO vtkUniformGrid : public vtkImageData
{
public:
  ///@{
  /**
   * Construct an empty uniform grid.
   */
  static vtkUniformGrid* New();
  vtkTypeMacro(vtkUniformGrid, vtkImageData);
  ///@}

  /**
   * Return what type of dataset this is.
   */
  int GetDataObjectType() VTK_FUTURE_CONST override { return VTK_UNIFORM_GRID; }

  using Superclass::Initialize;

  /**
   * Initialize with no ghost cell arrays, from the definition in
   * the given box. The box is expected to be 3D, if you have 2D
   * data the set the third dimensions 0. eg. (X,X,0)(X,X,0)
   * Returns 0 if the initialization failed
   * Deprecated, Use vtkAMRBox::InitializeGrid instead.
   */
  VTK_DEPRECATED_IN_9_6_0("Use vtkAMRBox::InitializeGrid instead")
  int Initialize(const vtkAMRBox* def, double* origin, double* spacing)
  {
    return def->InitializeGrid(this, origin, spacing);
  };

  /**
   * Initialize from the definition in the given box, with ghost cell
   * arrays nGhosts cells thick in all directions. The box is expected
   * to be 3D, if you have 2D data the set the third dimensions 0.
   * eg. (X,X,0)(X,X,0)
   * Returns 0 if the initialization failed.
   * Deprecated, Use vtkAMRBox::InitializeGrid instead.
   */
  VTK_DEPRECATED_IN_9_6_0("Use vtkAMRBox::InitializeGrid instead")
  int Initialize(const vtkAMRBox* def, double* origin, double* spacing, int nGhosts)
  {
    return def->InitializeGrid(this, origin, spacing, nGhosts);
  };

  /**
   * Initialize from the definition in the given box, with ghost cell
   * arrays of the thickness given in each direction by "nGhosts" array.
   * The box and ghost array are expected to be 3D, if you have 2D data
   * the set the third dimensions 0. eg. (X,X,0)(X,X,0)
   * Returns 0 if the initialization failed.
   * Deprecated, Use vtkAMRBox::InitializeGrid instead.
   */
  VTK_DEPRECATED_IN_9_6_0("Use vtkAMRBox::InitializeGrid instead")
  int Initialize(const vtkAMRBox* def, double* origin, double* spacing, const int nGhosts[3])
  {
    return def->InitializeGrid(this, origin, spacing, nGhosts);
  };

  /**
   * Construct a uniform grid, from the definition in the given box
   * "def", with ghost cell arrays of the thickness given in each
   * direction by "nGhosts*". The box and ghost array are expected
   * to be 3D, if you have 2D data the set the third dimensions 0. eg.
   * (X,X,0)(X,X,0)
   * Returns 0 if the initialization failed.
   * Deprecated, Use vtkAMRBox::InitializeGrid instead.
   */
  VTK_DEPRECATED_IN_9_6_0("Use vtkAMRBox::InitializeGrid instead")
  int Initialize(
    const vtkAMRBox* def, double* origin, double* spacing, int nGhostsI, int nGhostsJ, int nGhostsK)
  {
    return def->InitializeGrid(this, origin, spacing, nGhostsI, nGhostsJ, nGhostsK);
  };

  VTK_DEPRECATED_IN_9_6_0("Will be removed, create a vtkImageData manually if needed")
  virtual VTK_NEWINSTANCE vtkImageData* NewImageDataCopy();

  ///@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  VTK_DEPRECATED_IN_9_6_0("Use vtkImageData::GetData instead")
  static vtkUniformGrid* GetData(vtkInformation* info);
  VTK_DEPRECATED_IN_9_6_0("Use vtkImageData::GetData instead")
  static vtkUniformGrid* GetData(vtkInformationVector* v, int i = 0);
  ///@}

protected:
  vtkUniformGrid();
  ~vtkUniformGrid() override;

private:
  vtkUniformGrid(const vtkUniformGrid&) = delete;
  void operator=(const vtkUniformGrid&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
