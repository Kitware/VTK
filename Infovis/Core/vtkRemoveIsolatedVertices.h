/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRemoveIsolatedVertices.h

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
// .NAME vtkRemoveIsolatedVertices - remove vertices of a vtkGraph with
//    degree zero.
//
// .SECTION Description

#ifndef __vtkRemoveIsolatedVertices_h
#define __vtkRemoveIsolatedVertices_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkGraphAlgorithm.h"

class vtkDataSet;

class VTKINFOVISCORE_EXPORT vtkRemoveIsolatedVertices : public vtkGraphAlgorithm
{
public:
  static vtkRemoveIsolatedVertices* New();
  vtkTypeMacro(vtkRemoveIsolatedVertices,vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

protected:
  vtkRemoveIsolatedVertices();
  ~vtkRemoveIsolatedVertices();

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

private:
  vtkRemoveIsolatedVertices(const vtkRemoveIsolatedVertices&); // Not implemented
  void operator=(const vtkRemoveIsolatedVertices&);   // Not implemented
};

#endif

