/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRemoveUnusedPoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkRemoveUnusedPoints
 * @brief remove points not used by any cell.
 *
 * vtkRemoveUnusedPoints is a filter that removes any points that are not used by the
 * cells. Currently, this filter only supports vtkUnstructuredGrid.
 */

#ifndef vtkRemoveUnusedPoints_h
#define vtkRemoveUnusedPoints_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class VTKFILTERSCORE_EXPORT vtkRemoveUnusedPoints : public vtkUnstructuredGridAlgorithm
{
public:
  static vtkRemoveUnusedPoints* New();
  vtkTypeMacro(vtkRemoveUnusedPoints, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Enable adding a `vtkOriginalPointIds` array to the point data
   * which identifies the original point index. Default is true.
   */
  vtkSetMacro(GenerateOriginalPointIds, bool);
  vtkGetMacro(GenerateOriginalPointIds, bool);
  vtkBooleanMacro(GenerateOriginalPointIds, bool);
  ///@}

  ///@{
  /**
   * Choose the name to use for the original point ids array. Default is `vtkOriginalPointIds`.
   * This is used only when `GenerateOriginalPointIds` is true.
   */
  vtkSetStringMacro(OriginalPointIdsArrayName);
  vtkGetStringMacro(OriginalPointIdsArrayName);
  ///@}

protected:
  vtkRemoveUnusedPoints();
  ~vtkRemoveUnusedPoints() override;

  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkRemoveUnusedPoints(const vtkRemoveUnusedPoints&) = delete;
  void operator=(const vtkRemoveUnusedPoints&) = delete;

  bool GenerateOriginalPointIds;
  char* OriginalPointIdsArrayName;
};

#endif
