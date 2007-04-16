/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVertexDegree.h

  Copyright 2007 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
  license for use of this work by or on behalf of the
  U.S. Government. Redistribution and use in source and binary forms, with
  or without modification, are permitted provided that this Notice and any
  statement of authorship are reproduced on all copies.

=========================================================================*/
// .NAME vtkVertexDegree - Adds an attribute array with the degree of each vertex
//
// .SECTION Description
// Adds an attribute array with the degree of each vertex.

#ifndef __vtkVertexDegree_h
#define __vtkVertexDegree_h

#include "vtkAbstractGraphAlgorithm.h"

class VTK_INFOVIS_EXPORT vtkVertexDegree : public vtkAbstractGraphAlgorithm 
{
public:
  static vtkVertexDegree *New();

  vtkTypeRevisionMacro(vtkVertexDegree, vtkAbstractGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the output array name. If no output array name is
  // set then the name 'Degree' is used.
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

