/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRemoveGhosts.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkRemoveGhosts
 * @brief   Remove ghost points, cells and arrays
 *
 *
 * Removes ghost points, cells and associated data arrays. Works on
 * vtkPolyDatas and vtkUnstructuredGrids.
*/

#ifndef vtkRemoveGhosts_h
#define vtkRemoveGhosts_h

#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

class vtkPolyData;
class vtkUnsignedCharArray;

class VTKFILTERSPARALLEL_EXPORT vtkRemoveGhosts : public vtkPassInputTypeAlgorithm
{
public:
  vtkTypeMacro(vtkRemoveGhosts, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream &os, vtkIndent indent) override;

  static vtkRemoveGhosts *New();

protected:
  vtkRemoveGhosts();
  ~vtkRemoveGhosts() override;

  int RequestUpdateExtent(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *) override;
  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) override;

  // see algorithm for more info
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkRemoveGhosts(const vtkRemoveGhosts &) = delete;
  void operator=(const vtkRemoveGhosts &) = delete;
};

#endif //_vtkRemoveGhosts_h
