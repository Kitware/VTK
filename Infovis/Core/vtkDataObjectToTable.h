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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   vtkDataObjectToTable
 * @brief   extract field data as a table
 *
 *
 * This filter is used to extract either the field, cell or point data of
 * any data object as a table.
*/

#ifndef vtkDataObjectToTable_h
#define vtkDataObjectToTable_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkTableAlgorithm.h"

class VTKINFOVISCORE_EXPORT vtkDataObjectToTable : public vtkTableAlgorithm
{
public:
  static vtkDataObjectToTable* New();
  vtkTypeMacro(vtkDataObjectToTable,vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  enum
  {
    FIELD_DATA = 0,
    POINT_DATA = 1,
    CELL_DATA = 2,
    VERTEX_DATA = 3,
    EDGE_DATA = 4
  };

  //@{
  /**
   * The field type to copy into the output table.
   * Should be one of FIELD_DATA, POINT_DATA, CELL_DATA, VERTEX_DATA, EDGE_DATA.
   */
  vtkGetMacro(FieldType, int);
  vtkSetClampMacro(FieldType, int, 0, 4);
  //@}

protected:
  vtkDataObjectToTable();
  ~vtkDataObjectToTable() VTK_OVERRIDE;

  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*) VTK_OVERRIDE;

  int FieldType;

private:
  vtkDataObjectToTable(const vtkDataObjectToTable&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDataObjectToTable&) VTK_DELETE_FUNCTION;
};

#endif

