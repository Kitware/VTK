/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelectedPoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkExtractSelectedPoints - extract cells within a dataset that 
// contain the points listen in the vtkSelection.
// .SECTION Description
// vtkExtractSelectedPoints extracts all cells whose volume contain at least 
// one point listed in the POINTS content of the vtkSelection. 
// .SECTION See Also
// vtkSelection

#ifndef __vtkExtractSelectedPoints_h
#define __vtkExtractSelectedPoints_h

#include "vtkUnstructuredGridAlgorithm.h"

class vtkSelection;

class VTK_GRAPHICS_EXPORT vtkExtractSelectedPoints : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeRevisionMacro(vtkExtractSelectedPoints,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object
  static vtkExtractSelectedPoints *New();

protected:
  vtkExtractSelectedPoints();
  ~vtkExtractSelectedPoints();

  // Usual data generation method
  int RequestData(vtkInformation *, 
                  vtkInformationVector **, 
                  vtkInformationVector *);


  virtual int FillInputPortInformation(int port, vtkInformation* info);

private:
  vtkExtractSelectedPoints(const vtkExtractSelectedPoints&);  // Not implemented.
  void operator=(const vtkExtractSelectedPoints&);  // Not implemented.
};

#endif
