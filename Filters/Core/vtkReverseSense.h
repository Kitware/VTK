/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkReverseSense.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkReverseSense - reverse the ordering of polygonal cells and/or vertex normals
// .SECTION Description
//
// vtkReverseSense is a filter that reverses the order of polygonal cells
// and/or reverses the direction of point and cell normals. Two flags are
// used to control these operations. Cell reversal means reversing the order
// of indices in the cell connectivity list. Normal reversal means
// multiplying the normal vector by -1 (both point and cell normals,
// if present).

// .SECTION Caveats
// Normals can be operated on only if they are present in the data.

#ifndef vtkReverseSense_h
#define vtkReverseSense_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSCORE_EXPORT vtkReverseSense : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkReverseSense,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Construct object so that behavior is to reverse cell ordering and
  // leave normal orientation as is.
  static vtkReverseSense *New();

  // Description:
  // Flag controls whether to reverse cell ordering.
  vtkSetMacro(ReverseCells,int);
  vtkGetMacro(ReverseCells,int);
  vtkBooleanMacro(ReverseCells,int);

  // Description:
  // Flag controls whether to reverse normal orientation.
  vtkSetMacro(ReverseNormals,int);
  vtkGetMacro(ReverseNormals,int);
  vtkBooleanMacro(ReverseNormals,int);


protected:
  vtkReverseSense();
  ~vtkReverseSense() {}

  // Usual data generation method
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  int ReverseCells;
  int ReverseNormals;
private:
  vtkReverseSense(const vtkReverseSense&);  // Not implemented.
  void operator=(const vtkReverseSense&);  // Not implemented.
};

#endif
