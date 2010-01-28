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

#include "vtkDataSetAlgorithm.h"
#include "vtkMutableDirectedGraph.h"
#include "vtkReebGraph.h"
#include "vtkSmartPointer.h"

class VTK_FILTERING_EXPORT vtkReebGraphToGraphFilter :
  public vtkDataSetAlgorithm
{
public:
  static vtkReebGraphToGraphFilter* New();
  vtkTypeRevisionMacro(vtkReebGraphToGraphFilter,
    vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  int FillInputPortInformation(int portNumber, vtkInformation *);
  int FillOutputPortInformation(int, vtkInformation *);

  vtkMutableDirectedGraph* GetOutput();

protected:
  vtkReebGraphToGraphFilter();
  ~vtkReebGraphToGraphFilter();

  int RequestDataObject(vtkInformation *request,
                        vtkInformationVector **inputVector,
                        vtkInformationVector *outputVector);

private:
  vtkReebGraphToGraphFilter(
    const vtkReebGraphToGraphFilter&);
  // Not implemented.
  void operator=(const vtkReebGraphToGraphFilter&);
  // Not implemented.
};

#endif
