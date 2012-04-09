/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectedRows.h

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
// .NAME vtkExtractSelectedRows - return selected rows of a table
//
// .SECTION Description
// The first input is a vtkTable to extract rows from.
// The second input is a vtkSelection containing the selected indices.
// The third input is a vtkAnnotationLayers containing selected indices.
// The field type of the input selection is ignored when converted to row
// indices.

#ifndef __vtkExtractSelectedRows_h
#define __vtkExtractSelectedRows_h

#include "vtkTableAlgorithm.h"


class VTK_GRAPHICS_EXPORT vtkExtractSelectedRows : public vtkTableAlgorithm
{
public:
  static vtkExtractSelectedRows* New();
  vtkTypeMacro(vtkExtractSelectedRows,vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // A convenience method for setting the second input (i.e. the selection).
  void SetSelectionConnection(vtkAlgorithmOutput* in);

  // Description:
  // A convenience method for setting the third input (i.e. the annotation layers).
  void SetAnnotationLayersConnection(vtkAlgorithmOutput* in);
  
  // Description:
  // Specify the first vtkGraph input and the second vtkSelection input.
  int FillInputPortInformation(int port, vtkInformation* info);

  // Description:
  // When set, a column named vtkOriginalRowIds will be added to the output.
  // False by default.
  vtkSetMacro(AddOriginalRowIdsArray, bool);
  vtkGetMacro(AddOriginalRowIdsArray, bool);
  vtkBooleanMacro(AddOriginalRowIdsArray, bool);

protected:
  vtkExtractSelectedRows();
  ~vtkExtractSelectedRows();

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);
   
  bool AddOriginalRowIdsArray;
private:
  vtkExtractSelectedRows(const vtkExtractSelectedRows&); // Not implemented
  void operator=(const vtkExtractSelectedRows&);   // Not implemented
};

#endif

