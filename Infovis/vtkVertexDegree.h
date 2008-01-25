/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVertexDegree.h

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
// .NAME vtkVertexDegree - Adds an attribute array with the degree of each vertex
//
// .SECTION Description
// Adds an attribute array with the degree of each vertex.

#ifndef __vtkVertexDegree_h
#define __vtkVertexDegree_h

#include "vtkGraphAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkVertexDegree : public vtkGraphAlgorithm 
{
public:
  static vtkVertexDegree *New();

  vtkTypeRevisionMacro(vtkVertexDegree, vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the output array name. If no output array name is
  // set then the name 'VertexDegree' is used.
  vtkSetStringMacro(OutputArrayName);

protected:
  vtkVertexDegree();
  ~vtkVertexDegree();
  
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

private:

  char* OutputArrayName;

  vtkVertexDegree(const vtkVertexDegree&);  // Not implemented.
  void operator=(const vtkVertexDegree&);  // Not implemented.
};

#endif

