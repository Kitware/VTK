/*=========================================================================
  Program:   Visualization Toolkit
  Module:    vtkHierarchicalBoxDataSet.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHierarchicalBoxDataSet
 * @brief   Backwards compatibility class
 *
 *
 * An empty class for backwards compatiblity
 *
 * @sa
 * vtkUniformGridAM vtkOverlappingAMR vtkNonOverlappingAMR
*/

#ifndef vtkHierarchicalBoxDataSet_h
#define vtkHierarchicalBoxDataSet_h

#include "vtkCommonDataModelModule.h" // For export macro
#include "vtkOverlappingAMR.h"

class vtkInformation;
class vtkInformationVector;

class VTKCOMMONDATAMODEL_EXPORT vtkHierarchicalBoxDataSet:
  public vtkOverlappingAMR
{
public:
  static vtkHierarchicalBoxDataSet *New();
  vtkTypeMacro(vtkHierarchicalBoxDataSet,vtkOverlappingAMR);
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Return a new iterator (the iterator has to be deleted by user).
   */
  VTK_NEWINSTANCE vtkCompositeDataIterator* NewIterator() VTK_OVERRIDE;

  /**
   * Return class name of data type (see vtkType.h for definitions).
   */
  int GetDataObjectType() VTK_OVERRIDE {return VTK_HIERARCHICAL_BOX_DATA_SET;}

  //@{
  /**
   * Retrieve an instance of this class from an information object.
   */
  static vtkHierarchicalBoxDataSet* GetData(vtkInformation* info);
  static vtkHierarchicalBoxDataSet* GetData(vtkInformationVector* v, int i=0);
  //@}

protected:
  vtkHierarchicalBoxDataSet();
  ~vtkHierarchicalBoxDataSet() VTK_OVERRIDE;

private:
  vtkHierarchicalBoxDataSet(const vtkHierarchicalBoxDataSet&) VTK_DELETE_FUNCTION;
  void operator=(const vtkHierarchicalBoxDataSet&) VTK_DELETE_FUNCTION;
};

#endif
