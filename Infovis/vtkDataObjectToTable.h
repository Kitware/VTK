/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataObjectToTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataObjectToTable - extract field data as a table
//
// .SECTION Description

#ifndef __vtkDataObjectToTable_h
#define __vtkDataObjectToTable_h

#include "vtkTableAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkDataObjectToTable : public vtkTableAlgorithm
{
public:
  static vtkDataObjectToTable* New();
  vtkTypeRevisionMacro(vtkDataObjectToTable,vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  //BTX
  enum
    {
    FIELD_DATA = 0,
    POINT_DATA = 1,
    CELL_DATA = 2
    };
  //ETX
  
  // Description:
  // The field type to copy into the output table.
  // Should be one of FIELD_DATA, POINT_DATA, CELL_DATA.
  vtkGetMacro(FieldType, int);
  vtkSetClampMacro(FieldType, int, 0, 2);
  
protected:
  vtkDataObjectToTable();
  ~vtkDataObjectToTable();
  
  int FillInputPortInformation(int port, vtkInformation* info);
  
  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);
  
  int FieldType;

private:
  vtkDataObjectToTable(const vtkDataObjectToTable&); // Not implemented
  void operator=(const vtkDataObjectToTable&);   // Not implemented
};

#endif

