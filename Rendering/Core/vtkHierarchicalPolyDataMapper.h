/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHierarchicalPolyDataMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHierarchicalPolyDataMapper
 * @brief   a class that renders hierarchical polygonal data
 *
 * Legacy class. Use vtkCompositePolyDataMapper instead.
 *
 * @sa
 * vtkPolyDataMapper
*/

#ifndef vtkHierarchicalPolyDataMapper_h
#define vtkHierarchicalPolyDataMapper_h

#include "vtkRenderingCoreModule.h" // For export macro
#include "vtkCompositePolyDataMapper.h"

class VTKRENDERINGCORE_EXPORT vtkHierarchicalPolyDataMapper : public vtkCompositePolyDataMapper
{

public:
  static vtkHierarchicalPolyDataMapper *New();
  vtkTypeMacro(vtkHierarchicalPolyDataMapper, vtkCompositePolyDataMapper);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkHierarchicalPolyDataMapper();
  ~vtkHierarchicalPolyDataMapper();

private:
  vtkHierarchicalPolyDataMapper(const vtkHierarchicalPolyDataMapper&) VTK_DELETE_FUNCTION;
  void operator=(const vtkHierarchicalPolyDataMapper&) VTK_DELETE_FUNCTION;
};

#endif
