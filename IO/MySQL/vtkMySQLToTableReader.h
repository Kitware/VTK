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
/**
 * @class   vtkMySQLToTableReader
 * @brief   Read a MySQL table as a vtkTable
 *
 * vtkMySQLToTableReader reads a table from a MySQL database and
 * outputs it as a vtkTable.
 */

#ifndef vtkMySQLToTableReader_h
#define vtkMySQLToTableReader_h

#include "vtkDatabaseToTableReader.h"
#include "vtkIOMySQLModule.h" // For export macro

class vtkMySQLDatabase;

class VTKIOMYSQL_EXPORT vtkMySQLToTableReader : public vtkDatabaseToTableReader
{
public:
  static vtkMySQLToTableReader* New();
  vtkTypeMacro(vtkMySQLToTableReader, vtkDatabaseToTableReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkMySQLToTableReader();
  ~vtkMySQLToTableReader() override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkMySQLToTableReader(const vtkMySQLToTableReader&) = delete;
  void operator=(const vtkMySQLToTableReader&) = delete;
};

#endif
