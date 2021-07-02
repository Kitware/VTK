/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergeVectorComponents.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkMergeVectorComponents
 * @brief   merge components of many single-component arrays into one vector
 *
 * vtkMergeVectorComponents is a filter that merges three single-component arrays
 * into one vector. This is accomplished by creating one output vector with
 * 3 components. The type of the output vector is vtkDoubleArray. The user
 * needs to define the names of the single-component arrays and the attribute-type
 * of the arrays, i.e. point-data or cell-data.
 */

#ifndef vtkMergeVectorComponents_h
#define vtkMergeVectorComponents_h

#include "vtkDataObject.h"           // For attribute types
#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkMergeVectorComponents : public vtkPassInputTypeAlgorithm
{
public:
  static vtkMergeVectorComponents* New();
  vtkTypeMacro(vtkMergeVectorComponents, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set the name of the array to use as the X component of the combination vector
   */
  vtkSetStringMacro(XArrayName);
  vtkGetStringMacro(XArrayName);
  ///@}

  ///@{
  /**
   * Set the name of the array to use as the Y component of the combination vector
   */
  vtkSetStringMacro(YArrayName);
  vtkGetStringMacro(YArrayName);
  ///@}

  ///@{
  /**
   * Set the name of the array to use as the Z attribute of the combination vector
   */
  vtkSetStringMacro(ZArrayName);
  vtkGetStringMacro(ZArrayName);
  ///@}

  ///@{
  /**
   * Set the name of the output combination vector. If name is undefined, the output vector will
   * be named, "combinationVector".
   */
  vtkSetStringMacro(OutputVectorName);
  vtkGetStringMacro(OutputVectorName);
  ///@}

  ///@{
  /**
   * Control which AttributeType the filter operates on (point data or cell data
   * for vtkDataSets). The default value is vtkDataObject::POINT. The input value for
   * this function should be either vtkDataObject::POINT or vtkDataObject::CELL.
   */
  vtkSetMacro(AttributeType, int);
  vtkGetMacro(AttributeType, int);
  void SetAttributeTypeToPointData() { this->SetAttributeType(vtkDataObject::POINT); }
  void SetAttributeTypeToCellData() { this->SetAttributeType(vtkDataObject::CELL); }
  ///@}

protected:
  vtkMergeVectorComponents();
  ~vtkMergeVectorComponents() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  char* XArrayName;
  char* YArrayName;
  char* ZArrayName;
  char* OutputVectorName;
  int AttributeType;

private:
  vtkMergeVectorComponents(const vtkMergeVectorComponents&) = delete;
  void operator=(const vtkMergeVectorComponents&) = delete;
};

#endif
