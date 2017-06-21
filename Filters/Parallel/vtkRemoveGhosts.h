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
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  static vtkRemoveGhosts *New();

protected:
  vtkRemoveGhosts();
  ~vtkRemoveGhosts() VTK_OVERRIDE;

  int RequestUpdateExtent(vtkInformation *,
                          vtkInformationVector **,
                          vtkInformationVector *) VTK_OVERRIDE;
  int RequestData(vtkInformation *, vtkInformationVector **,
                  vtkInformationVector *) VTK_OVERRIDE;

  // see algorithm for more info
  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

private:
  vtkRemoveGhosts(const vtkRemoveGhosts &) VTK_DELETE_FUNCTION;
  void operator=(const vtkRemoveGhosts &) VTK_DELETE_FUNCTION;
};

#endif //_vtkRemoveGhosts_h
