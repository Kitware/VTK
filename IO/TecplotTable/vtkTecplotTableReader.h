/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTecplotTableReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2016 Menno Deij - van Rijswijk (MARIN)
-------------------------------------------------------------------------*/


/**
 * @class   vtkTecplotTableReader
 * @brief   reads in Tecplot tabular data
 * and outputs a vtkTable data structure.
 *
 *
 * vtkTecplotTableReader is an interface for reading tabulat data in Tecplot
 * ascii format.
 *
 * @par Thanks:
 * Thanks to vtkDelimitedTextReader authors.
 *
*/

#ifndef vtkTecplotTableReader_h
#define vtkTecplotTableReader_h

#include "vtkIOTecplotTableModule.h" // For export macro
#include "vtkTableAlgorithm.h"
#include "vtkUnicodeString.h" // Needed for vtkUnicodeString
#include "vtkStdString.h" // Needed for vtkStdString

class VTKIOTECPLOTTABLE_EXPORT vtkTecplotTableReader : public vtkTableAlgorithm
{
public:
  static vtkTecplotTableReader* New();
  vtkTypeMacro(vtkTecplotTableReader, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Specifies the delimited text file to be loaded.
   */
  vtkGetStringMacro(FileName);
  vtkSetStringMacro(FileName);
  //@}

  //@{
  /**
   * Specifies the maximum number of records to read from the file.  Limiting the
   * number of records to read is useful for previewing the contents of a file.
   */
  vtkGetMacro(MaxRecords, vtkIdType);
  vtkSetMacro(MaxRecords, vtkIdType);
  //@}

  //@{
  /**
   * Specifies the number of lines that form the header of the file. Default is 2.
   */
  vtkGetMacro(HeaderLines, vtkIdType);
  vtkSetMacro(HeaderLines, vtkIdType);
  //@}

  //@{
  /**
   * Specifies the line number that holds the column names. Default is 1.
   */
  vtkGetMacro(ColumnNamesOnLine, vtkIdType);
  vtkSetMacro(ColumnNamesOnLine, vtkIdType);
  //@}

  //@{
  /**
   * Specifies the number of fields to skip while reading the column names. Default is 1.
   */
  vtkGetMacro(SkipColumnNames, vtkIdType);
  vtkSetMacro(SkipColumnNames, vtkIdType);
  //@}



  //@{
  /**
   * The name of the array for generating or assigning pedigree ids
   * (default "id").
   */
  vtkSetStringMacro(PedigreeIdArrayName);
  vtkGetStringMacro(PedigreeIdArrayName);
  //@}

  //@{
  /**
   * If on (default), generates pedigree ids automatically.
   * If off, assign one of the arrays to be the pedigree id.
   */
  vtkSetMacro(GeneratePedigreeIds, bool);
  vtkGetMacro(GeneratePedigreeIds, bool);
  vtkBooleanMacro(GeneratePedigreeIds, bool);
  //@}

  //@{
  /**
   * If on, assigns pedigree ids to output. Defaults to off.
   */
  vtkSetMacro(OutputPedigreeIds, bool);
  vtkGetMacro(OutputPedigreeIds, bool);
  vtkBooleanMacro(OutputPedigreeIds, bool);
  //@}

  /**
   * Returns a human-readable description of the most recent error, if any.
   * Otherwise, returns an empty string.  Note that the result is only valid
   * after calling Update().
   */
  vtkStdString GetLastError();

protected:
  vtkTecplotTableReader();
  ~vtkTecplotTableReader() VTK_OVERRIDE;

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*) VTK_OVERRIDE;

  char* FileName;
  vtkIdType MaxRecords;
  vtkIdType HeaderLines;
  vtkIdType ColumnNamesOnLine;
  vtkIdType SkipColumnNames;
  char* PedigreeIdArrayName;
  bool GeneratePedigreeIds;
  bool OutputPedigreeIds;
  vtkStdString LastError;

private:
  vtkTecplotTableReader(const vtkTecplotTableReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkTecplotTableReader&) VTK_DELETE_FUNCTION;

};

#endif
