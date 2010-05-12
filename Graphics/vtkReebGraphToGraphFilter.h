/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile$

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkReebGraphToGraphFilter - generate a vtkGraph from a vtkReebGraph
// (traversal convenience).
// .SECTION Description
// The filter takes an input vtkReebGraph object and outputs a
// vtkMutableDirectedGraph object.

#ifndef __vtkReebGraphToGraphFilter_h
#define __vtkReebGraphToGraphFilter_h

#include  "vtkDirectedGraphAlgorithm.h"
#include  "vtkMutableDirectedGraph.h"

class VTK_FILTERING_EXPORT vtkReebGraphToGraphFilter :
  public vtkDirectedGraphAlgorithm
{
public:
  static vtkReebGraphToGraphFilter* New();
  vtkTypeRevisionMacro(vtkReebGraphToGraphFilter,
    vtkDirectedGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  vtkMutableDirectedGraph* GetOutput();

protected:
  vtkReebGraphToGraphFilter();
  ~vtkReebGraphToGraphFilter();

  int FillInputPortInformation(int portNumber, vtkInformation *);

  int RequestData(vtkInformation *request,
    vtkInformationVector **inputVector, vtkInformationVector *outputVector);

private:
  vtkReebGraphToGraphFilter(
    const vtkReebGraphToGraphFilter&);
  // Not implemented.
  void operator=(const vtkReebGraphToGraphFilter&);
  // Not implemented.
};

#endif
