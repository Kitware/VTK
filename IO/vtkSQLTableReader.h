/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSQLTableReader.h

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
// .NAME vtkSQLTableReader - executes an sql query and retrieves results into a table
//
// .SECTION Description
// vtkSQLTableReader creates a vtkTable with the results of an arbitrary SQL
// query.  To use this filter, you first need an instance of a vtkSQLDatabase
// subclass.  You may use the database class to obtain a vtkSQLQuery instance.
// Set that query on this filter to extract the query as a table.
//
// .SECTION Thanks
// Thanks to Andrew Wilson from Sandia National Laboratories for his work
// on the database classes.
//
// .SECTION See Also
// vtkSQLDatabase vtkSQLQuery

#ifndef __vtkSQLTableReader_h
#define __vtkSQLTableReader_h

#include "vtkTableAlgorithm.h"

class vtkSQLQuery;

class VTK_IO_EXPORT vtkSQLTableReader : public vtkTableAlgorithm
{
public:
  static vtkSQLTableReader* New();
  vtkTypeRevisionMacro(vtkSQLTableReader, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // The query to execute.
  void SetQuery(vtkSQLQuery* query);
  vtkGetObjectMacro(Query, vtkSQLQuery);
  
  // Description:
  // Update the modified time based on the query.
  unsigned long GetMTime();

protected:
  vtkSQLTableReader();
  ~vtkSQLTableReader();

  vtkSQLQuery* Query;

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);
    
private:
  vtkSQLTableReader(const vtkSQLTableReader&); // Not implemented
  void operator=(const vtkSQLTableReader&);   // Not implemented
};

#endif

