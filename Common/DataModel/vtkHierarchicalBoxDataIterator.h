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
/**
 * @class   vtkHierarchicalBoxDataIterator
 *
 *
 *  Empty class for backwards compatibility.
*/

#ifndef vtkHierarchicalBoxDataIterator_h
#define vtkHierarchicalBoxDataIterator_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkUniformGridAMRDataIterator.h"

class VTKCOMMONDATAMODEL_EXPORT vtkHierarchicalBoxDataIterator :
  public vtkUniformGridAMRDataIterator
{
  public:
    static vtkHierarchicalBoxDataIterator* New();
    vtkTypeMacro(vtkHierarchicalBoxDataIterator,vtkUniformGridAMRDataIterator);
    void PrintSelf(ostream &os, vtkIndent indent) override;

  protected:
    vtkHierarchicalBoxDataIterator();
    ~vtkHierarchicalBoxDataIterator() override;

  private:
    vtkHierarchicalBoxDataIterator(const vtkHierarchicalBoxDataIterator&) = delete;
    void operator=(const vtkHierarchicalBoxDataIterator&) = delete;
};

#endif /* VTKHIERARCHICALBOXDATAITERATOR_H_ */
