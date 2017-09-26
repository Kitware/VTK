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
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   vtkVertexDegree
 * @brief   Adds an attribute array with the degree of each vertex
 *
 *
 * Adds an attribute array with the degree of each vertex. By default the name
 * of the array will be "VertexDegree", but that can be changed by calling
 * SetOutputArrayName("foo");
*/

#ifndef vtkVertexDegree_h
#define vtkVertexDegree_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkGraphAlgorithm.h"

class VTKINFOVISCORE_EXPORT vtkVertexDegree : public vtkGraphAlgorithm
{
public:
  static vtkVertexDegree *New();

  vtkTypeMacro(vtkVertexDegree, vtkGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set the output array name. If no output array name is
   * set then the name 'VertexDegree' is used.
   */
  vtkSetStringMacro(OutputArrayName);
  //@}

protected:
  vtkVertexDegree();
  ~vtkVertexDegree() override;

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;

private:

  char* OutputArrayName;

  vtkVertexDegree(const vtkVertexDegree&) = delete;
  void operator=(const vtkVertexDegree&) = delete;
};

#endif

