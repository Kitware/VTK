/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMySQLToTableReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMySQLToTableReader - Read a MySQL table as a vtkTable
// .SECTION Description
// vtkMySQLToTableReader reads a table from a MySQL database and
// outputs it as a vtkTable.

#ifndef __vtkMySQLToTableReader_h
#define __vtkMySQLToTableReader_h

#include "vtkDatabaseToTableReader.h"

class vtkMySQLDatabase;

class VTK_IO_EXPORT vtkMySQLToTableReader :
  public vtkDatabaseToTableReader
{
public:
  static vtkMySQLToTableReader *New();
  vtkTypeMacro(vtkMySQLToTableReader,vtkDatabaseToTableReader);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
   vtkMySQLToTableReader();
  ~vtkMySQLToTableReader();
  int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *);
private:
  vtkMySQLToTableReader(const vtkMySQLToTableReader&); // Not implemented.
  void operator=(const vtkMySQLToTableReader&); // Not implemented

};

#endif
