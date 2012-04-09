/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSplitColumnComponents.h

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
// .NAME vtkSplitColumnComponents - split multicomponent table columns
//
// .SECTION Description
// Splits any columns in a table that have more than one component into
// individual columns. Single component columns are passed through without
// any data duplication. So if column names "Points" had three components
// this column would be split into "Points (0)", "Points (1)" and Points (2)".

#ifndef __vtkSplitColumnComponents_h
#define __vtkSplitColumnComponents_h

#include "vtkTableAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkSplitColumnComponents : public vtkTableAlgorithm
{
public:
  static vtkSplitColumnComponents* New();
  vtkTypeMacro(vtkSplitColumnComponents,vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // If on this filter will calculate an additional magnitude column for all
  // columns it splits with two or more components.
  // Default is on.
  vtkSetMacro(CalculateMagnitudes, bool);
  vtkGetMacro(CalculateMagnitudes, bool);

protected:
  vtkSplitColumnComponents();
  ~vtkSplitColumnComponents();

  bool CalculateMagnitudes;

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

private:
  vtkSplitColumnComponents(const vtkSplitColumnComponents&); // Not implemented
  void operator=(const vtkSplitColumnComponents&);   // Not implemented
};

#endif
