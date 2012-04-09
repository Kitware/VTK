/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPBGLConnectedComponents.h

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
// .NAME vtkPBGLConnectedComponents - Compute connected components for a
// distributed vtkGraph. For directed graphs, this computes the connected
// components; for undirected graphs, this computes the strongly-connected
// components.
//
// .SECTION Description
// This VTK class uses the Parallel BGL's implementation of connected
// components and strongly-connectd components.

#ifndef __vtkPBGLConnectedComponents_h
#define __vtkPBGLConnectedComponents_h

#include "vtkInfovisParallelModule.h" // For export macro
#include "vtkStdString.h" // For string type
#include "vtkVariant.h" // For variant type

#include "vtkGraphAlgorithm.h"

class vtkSelection;

class VTKINFOVISPARALLEL_EXPORT vtkPBGLConnectedComponents : public vtkGraphAlgorithm
{
public:
  static vtkPBGLConnectedComponents *New();
  vtkTypeMacro(vtkPBGLConnectedComponents, vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the name of the component number output array, which contains the
  // component number of each vertex (a non-negative value). If no component
  // array name is set then the name 'Component' is used.
  vtkSetStringMacro(ComponentArrayName);

protected:
  vtkPBGLConnectedComponents();
  ~vtkPBGLConnectedComponents();

  virtual int RequestData(
    vtkInformation *,
    vtkInformationVector **,
    vtkInformationVector *);

  virtual int FillInputPortInformation(
    int port, vtkInformation* info);

  virtual int FillOutputPortInformation(
    int port, vtkInformation* info);

private:
  char* ComponentArrayName;

  vtkPBGLConnectedComponents(const vtkPBGLConnectedComponents&);  // Not implemented.
  void operator=(const vtkPBGLConnectedComponents&);  // Not implemented.
};

#endif
