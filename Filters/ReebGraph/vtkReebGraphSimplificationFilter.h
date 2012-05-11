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
// .NAME vtkReebGraphSimplificationFilter - simplify an input Reeb graph.
// .SECTION Description
// The filter takes an input vtkReebGraph object and outputs a
// vtkReebGraph object.

#ifndef __vtkReebGraphSimplificationFilter_h
#define __vtkReebGraphSimplificationFilter_h

#include "vtkFiltersReebGraphModule.h" // For export macro
#include "vtkDirectedGraphAlgorithm.h"

class vtkReebGraph;
class vtkReebGraphSimplificationMetric;

class VTKFILTERSREEBGRAPH_EXPORT vtkReebGraphSimplificationFilter :
  public vtkDirectedGraphAlgorithm
{
public:
  static vtkReebGraphSimplificationFilter* New();
  vtkTypeMacro(vtkReebGraphSimplificationFilter, vtkDirectedGraphAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the persistence threshold for simplification (from 0 to 1).
  // Default value: 0 (no simplification).
  vtkSetMacro(SimplificationThreshold, double);
  vtkGetMacro(SimplificationThreshold, double);

  // Description:
  // Set the persistence metric evaluation code
  // Default value: NULL (standard topological persistence).
  void SetSimplificationMetric(vtkReebGraphSimplificationMetric *metric);

  vtkReebGraph* GetOutput();

protected:
  vtkReebGraphSimplificationFilter();
  ~vtkReebGraphSimplificationFilter();

  double SimplificationThreshold;

  vtkReebGraphSimplificationMetric *SimplificationMetric;

  int FillInputPortInformation(int portNumber, vtkInformation *);
  int FillOutputPortInformation(int, vtkInformation *);

  int RequestData(vtkInformation *request,
    vtkInformationVector **inputVector, vtkInformationVector *outputVector);

private:
  vtkReebGraphSimplificationFilter(const vtkReebGraphSimplificationFilter&); // Not implemented.
  void operator=(const vtkReebGraphSimplificationFilter&); // Not implemented.
};

#endif
