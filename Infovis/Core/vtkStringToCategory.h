/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkStringToCategory.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   vtkStringToCategory
 * @brief   Creates a category array from a string array
 *
 *
 * vtkStringToCategory creates an integer array named "category" based on the
 * values in a string array.  You may use this filter to create an array that
 * you may use to color points/cells by the values in a string array.  Currently
 * there is not support to color by a string array directly.
 * The category values will range from zero to N-1,
 * where N is the number of distinct strings in the string array.  Set the string
 * array to process with SetInputArrayToProcess(0,0,0,...).  The array may be in
 * the point, cell, or field data of the data object.
 *
 * The list of unique strings, in the order they are mapped, can also be
 * retrieved from output port 1. They are in a vtkTable, stored in the "Strings"
 * column as a vtkStringArray.
*/

#ifndef vtkStringToCategory_h
#define vtkStringToCategory_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkDataObjectAlgorithm.h"

class VTKINFOVISCORE_EXPORT vtkStringToCategory : public vtkDataObjectAlgorithm
{
public:
  static vtkStringToCategory* New();
  vtkTypeMacro(vtkStringToCategory,vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * The name to give to the output vtkIntArray of category values.
   */
  vtkSetStringMacro(CategoryArrayName);
  vtkGetStringMacro(CategoryArrayName);
  //@}

  /**
   * This is required to capture REQUEST_DATA_OBJECT requests.
   */
  int ProcessRequest(vtkInformation* request,
                             vtkInformationVector** inputVector,
                             vtkInformationVector* outputVector) override;

protected:
  vtkStringToCategory();
  ~vtkStringToCategory() override;

  /**
   * Creates the same output type as the input type.
   */
  int RequestDataObject(vtkInformation* request,
                                vtkInformationVector** inputVector,
                                vtkInformationVector* outputVector) override;

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*) override;

  int FillOutputPortInformation(int port, vtkInformation* info) override;

  char *CategoryArrayName;

private:
  vtkStringToCategory(const vtkStringToCategory&) = delete;
  void operator=(const vtkStringToCategory&) = delete;
};

#endif

