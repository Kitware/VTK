/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCollapseVerticesByArray.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkThresholdGraph - Returns a subgraph of a vtkGraph.
//
// .SECTION Description
// Requires input array, lower and upper threshold. This filter than
// extracts the subgraph based on these three parameters.

#ifndef __vtkThresholdGraph_h
#define __vtkThresholdGraph_h

#include "vtkInfovisCoreModule.h" // For export macro
#include "vtkGraphAlgorithm.h"

class VTKINFOVISCORE_EXPORT vtkThresholdGraph : public vtkGraphAlgorithm
{
public:

  static vtkThresholdGraph* New();
  vtkTypeMacro(vtkThresholdGraph, vtkGraphAlgorithm);

  void PrintSelf(ostream &os, vtkIndent indent);

  // Description:
  // Get/Set lower threshold. This would be the value against which
  // edge or vertex data array value will be compared.
  vtkGetMacro(LowerThreshold, double);
  vtkSetMacro(LowerThreshold, double);

  // Description:
  // Get/Set upper threshold. This would be the value against which
  // edge or vertex data array value will be compared.
  vtkGetMacro(UpperThreshold, double);
  vtkSetMacro(UpperThreshold, double);

//BTX
protected:

  vtkThresholdGraph();
 ~vtkThresholdGraph();

  virtual int RequestData(vtkInformation*,
                          vtkInformationVector**,
                          vtkInformationVector*);


private:

  double LowerThreshold;
  double UpperThreshold;


  vtkThresholdGraph(const vtkThresholdGraph&);  // Not implemented.
  void operator =(const vtkThresholdGraph&);    // Not implemented.
//ETX
};

#endif // __vtkThresholdGraph_h
