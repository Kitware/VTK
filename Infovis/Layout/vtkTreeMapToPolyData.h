// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkTreeMapToPolyData
 * @brief   converts a tree to a polygonal data representing a tree map
 *
 *
 * This algorithm requires that the vtkTreeMapLayout filter has already applied to the
 * data in order to create the quadruple array (min x, max x, min y, max y) of
 * bounds for each vertex of the tree.
 */

#ifndef vtkTreeMapToPolyData_h
#define vtkTreeMapToPolyData_h

#include "vtkInfovisLayoutModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKINFOVISLAYOUT_EXPORT vtkTreeMapToPolyData : public vtkPolyDataAlgorithm
{
public:
  static vtkTreeMapToPolyData* New();
  vtkTypeMacro(vtkTreeMapToPolyData, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * The field containing quadruples of the form (min x, max x, min y, max y)
   * representing the bounds of the rectangles for each vertex.
   * This array may be added to the tree using vtkTreeMapLayout.
   */
  virtual void SetRectanglesArrayName(const char* name)
  {
    this->SetInputArrayToProcess(0, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, name);
  }

  /**
   * The field containing the level of each tree node.
   * This can be added using vtkTreeLevelsFilter before this filter.
   * If this is not present, the filter simply calls tree->GetLevel(v) for
   * each vertex, which will produce the same result, but
   * may not be as efficient.
   */
  virtual void SetLevelArrayName(const char* name)
  {
    this->SetInputArrayToProcess(1, 0, 0, vtkDataObject::FIELD_ASSOCIATION_VERTICES, name);
  }

  ///@{
  /**
   * The spacing along the z-axis between tree map levels.
   */
  vtkGetMacro(LevelDeltaZ, double);
  vtkSetMacro(LevelDeltaZ, double);
  ///@}

  ///@{
  /**
   * The spacing along the z-axis between tree map levels.
   */
  vtkGetMacro(AddNormals, bool);
  vtkSetMacro(AddNormals, bool);
  ///@}

  int FillInputPortInformation(int port, vtkInformation* info) override;

protected:
  vtkTreeMapToPolyData();
  ~vtkTreeMapToPolyData() override;

  double LevelDeltaZ;
  bool AddNormals;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkTreeMapToPolyData(const vtkTreeMapToPolyData&) = delete;
  void operator=(const vtkTreeMapToPolyData&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
