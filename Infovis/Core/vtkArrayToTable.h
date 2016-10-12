/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayToTable.h

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkArrayToTable
 * @brief   Converts one- and two-dimensional vtkArrayData
 * objects to vtkTable
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
*/

#ifndef vtkArrayToTable_h
#define vtkArrayToTable_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkTableAlgorithm.h"

class VTKINFOVISCORE_EXPORT vtkArrayToTable : public vtkTableAlgorithm
{
public:
  static vtkArrayToTable* New();
  vtkTypeMacro(vtkArrayToTable, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkArrayToTable();
  ~vtkArrayToTable();

  int FillInputPortInformation(int, vtkInformation*);

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

private:
  vtkArrayToTable(const vtkArrayToTable&) VTK_DELETE_FUNCTION;
  void operator=(const vtkArrayToTable&) VTK_DELETE_FUNCTION;
};

#endif

