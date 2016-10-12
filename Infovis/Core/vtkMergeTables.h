/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMergeTables.h

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
 * @class   vtkMergeTables
 * @brief   combine two tables
 *
 *
 * Combines the columns of two tables into one larger table.
 * The number of rows in the resulting table is the sum of the number of
 * rows in each of the input tables.
 * The number of columns in the output is generally the sum of the number
 * of columns in each input table, except in the case where column names
 * are duplicated in both tables.
 * In this case, if MergeColumnsByName is on (the default), the two columns
 * will be merged into a single column of the same name.
 * If MergeColumnsByName is off, both columns will exist in the output.
 * You may set the FirstTablePrefix and SecondTablePrefix to define how
 * the columns named are modified.  One of these prefixes may be the empty
 * string, but they must be different.
*/

#ifndef vtkMergeTables_h
#define vtkMergeTables_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkTableAlgorithm.h"

class VTKINFOVISCORE_EXPORT vtkMergeTables : public vtkTableAlgorithm
{
public:
  static vtkMergeTables* New();
  vtkTypeMacro(vtkMergeTables,vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * The prefix to give to same-named fields from the first table.
   * Default is "Table1.".
   */
  vtkSetStringMacro(FirstTablePrefix);
  vtkGetStringMacro(FirstTablePrefix);
  //@}

  //@{
  /**
   * The prefix to give to same-named fields from the second table.
   * Default is "Table2.".
   */
  vtkSetStringMacro(SecondTablePrefix);
  vtkGetStringMacro(SecondTablePrefix);
  //@}

  //@{
  /**
   * If on, merges columns with the same name.
   * If off, keeps both columns, but calls one
   * FirstTablePrefix + name, and the other SecondTablePrefix + name.
   * Default is on.
   */
  vtkSetMacro(MergeColumnsByName, bool);
  vtkGetMacro(MergeColumnsByName, bool);
  vtkBooleanMacro(MergeColumnsByName, bool);
  //@}

  //@{
  /**
   * If on, all columns will have prefixes except merged columns.
   * If off, only unmerged columns with the same name will have prefixes.
   * Default is off.
   */
  vtkSetMacro(PrefixAllButMerged, bool);
  vtkGetMacro(PrefixAllButMerged, bool);
  vtkBooleanMacro(PrefixAllButMerged, bool);
  //@}

protected:
  vtkMergeTables();
  ~vtkMergeTables();

  bool MergeColumnsByName;
  bool PrefixAllButMerged;
  char* FirstTablePrefix;
  char* SecondTablePrefix;

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

private:
  vtkMergeTables(const vtkMergeTables&) VTK_DELETE_FUNCTION;
  void operator=(const vtkMergeTables&) VTK_DELETE_FUNCTION;
};

#endif

