// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkRemoveDuplicatePolys
 * @brief   remove duplicate/degenerate polygons
 *
 * vtkRemoveDuplicatePolys is a filter that removes duplicate or degenerate
 * polygons. Assumes the input grid does not contain duplicate points. You
 * may want to run vtkCleanPolyData first to assert it. If duplicated
 * polygons are found they are removed in the output.
 *
 * @sa
 * vtkCleanPolyData
 */

#ifndef vtkRemoveDuplicatePolys_h
#define vtkRemoveDuplicatePolys_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSCORE_EXPORT vtkRemoveDuplicatePolys : public vtkPolyDataAlgorithm
{
public:
  static vtkRemoveDuplicatePolys* New();
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkTypeMacro(vtkRemoveDuplicatePolys, vtkPolyDataAlgorithm);

protected:
  vtkRemoveDuplicatePolys();
  ~vtkRemoveDuplicatePolys() override;

  // Usual data generation method.
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkRemoveDuplicatePolys(const vtkRemoveDuplicatePolys&) = delete;
  void operator=(const vtkRemoveDuplicatePolys&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
