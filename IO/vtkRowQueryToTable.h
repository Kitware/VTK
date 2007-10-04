/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRowQueryToTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkRowQueryToTable - executes an sql query and retrieves results into a table
//
// .SECTION Description
// vtkRowQueryToTable creates a vtkTable with the results of an arbitrary SQL
// query.  To use this filter, you first need an instance of a vtkSQLDatabase
// subclass.  You may use the database class to obtain a vtkRowQuery instance.
// Set that query on this filter to extract the query as a table.
//
// .SECTION Thanks
// Thanks to Andrew Wilson from Sandia National Laboratories for his work
// on the database classes.
//
// .SECTION See Also
// vtkSQLDatabase vtkRowQuery

#ifndef __vtkRowQueryToTable_h
#define __vtkRowQueryToTable_h

#include "vtkTableAlgorithm.h"

class vtkRowQuery;

class VTK_IO_EXPORT vtkRowQueryToTable : public vtkTableAlgorithm
{
public:
  static vtkRowQueryToTable* New();
  vtkTypeRevisionMacro(vtkRowQueryToTable, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The query to execute.
  void SetQuery(vtkRowQuery* query);
  vtkGetObjectMacro(Query, vtkRowQuery);
  
  // Description:
  // Update the modified time based on the query.
  unsigned long GetMTime();

protected:
  vtkRowQueryToTable();
  ~vtkRowQueryToTable();

  vtkRowQuery* Query;

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);
    
private:
  vtkRowQueryToTable(const vtkRowQueryToTable&); // Not implemented
  void operator=(const vtkRowQueryToTable&);   // Not implemented
};

#endif

