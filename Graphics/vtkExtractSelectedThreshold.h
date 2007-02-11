/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectedThreshold.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractSelectedThreshold - extract a cells or points from a dataset
// that have values within a threshold.
// .SECTION Description
// vtkExtractSelectedThreshold extracts all cells and points with attribute 
// values that lie within a vtkSelection's THRESHOLD contents. The selecion
// can specify to threshold a particular array within either the point or cell
// attribute data of the input. This uses vtkThreshold internally.
// .SECTION See Also
// vtkSelection vtkThreshold 

#ifndef __vtkExtractSelectedThreshold_h
#define __vtkExtractSelectedThreshold_h

#include "vtkUnstructuredGridAlgorithm.h"

class vtkSelection;
class vtkThreshold;

class VTK_GRAPHICS_EXPORT vtkExtractSelectedThreshold : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkExtractSelectedThreshold,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object with NULL vtkThresholdFilter
  static vtkExtractSelectedThreshold *New();

protected:
  vtkExtractSelectedThreshold();
  ~vtkExtractSelectedThreshold();

  // Usual data generation method
  int RequestData(vtkInformation *, 
                  vtkInformationVector **, 
                  vtkInformationVector *);


  virtual int FillInputPortInformation(int port, vtkInformation* info);

  vtkThreshold* ThresholdFilter;

private:
  vtkExtractSelectedThreshold(const vtkExtractSelectedThreshold&);  // Not implemented.
  void operator=(const vtkExtractSelectedThreshold&);  // Not implemented.
};

#endif
