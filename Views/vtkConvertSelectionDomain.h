/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkConvertSelectionDomain.h

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
// .NAME vtkConvertSelectionDomain - Convert a selection from one domain to another
//
// .SECTION Description
// vtkConvertSelectionDomain converts a selection from one domain to another
// using known domain mappings. The domain mappings are described by a
// vtkMultiBlockDataSet containing one or more vtkTables.
//
// The first input port is for the input selection, while the second port
// is for the multi-block of mappings, and the third port is for the
// data that is being selected on.
//
// If the second or third port is not set, this filter will pass the
// selection to the output unchanged.

#ifndef __vtkConvertSelectionDomain_h
#define __vtkConvertSelectionDomain_h

#include "vtkSelectionAlgorithm.h"

class VTK_VIEWS_EXPORT vtkConvertSelectionDomain : public vtkSelectionAlgorithm 
{
public:
  static vtkConvertSelectionDomain *New();
  vtkTypeRevisionMacro(vtkConvertSelectionDomain, vtkSelectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkConvertSelectionDomain();
  ~vtkConvertSelectionDomain();

  virtual int RequestData(
    vtkInformation *, 
    vtkInformationVector **, 
    vtkInformationVector *);
  
  virtual int FillInputPortInformation(
    int port, vtkInformation* info);
  
private:
  vtkConvertSelectionDomain(const vtkConvertSelectionDomain&);  // Not implemented.
  void operator=(const vtkConvertSelectionDomain&);  // Not implemented.
};

#endif
