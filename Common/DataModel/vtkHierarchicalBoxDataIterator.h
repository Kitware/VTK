/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkHierarchicalBoxDataIterator.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkHierarchicalBoxDataIterator.h--Empty class for backwards compatibility
//
// .SECTION Description
//  Empty class for backwards compatibility.
#ifndef VTKHIERARCHICALBOXDATAITERATOR_H_
#define VTKHIERARCHICALBOXDATAITERATOR_H_

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkUniformGridAMRDataIterator.h"

class VTKCOMMONDATAMODEL_EXPORT vtkHierarchicalBoxDataIterator :
  public vtkUniformGridAMRDataIterator
{
  public:
    static vtkHierarchicalBoxDataIterator* New();
    vtkTypeMacro(vtkHierarchicalBoxDataIterator,vtkUniformGridAMRDataIterator);
    void PrintSelf(ostream &os, vtkIndent indent);

  protected:
    vtkHierarchicalBoxDataIterator();
    virtual ~vtkHierarchicalBoxDataIterator();

  private:
    vtkHierarchicalBoxDataIterator(const vtkHierarchicalBoxDataIterator&); // Not implemented
    void operator=(const vtkHierarchicalBoxDataIterator&); // Not implemented
};

#endif /* VTKHIERARCHICALBOXDATAITERATOR_H_ */
