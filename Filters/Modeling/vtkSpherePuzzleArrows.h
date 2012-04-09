/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSpherePuzzleArrows.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSpherePuzzleArrows - Visualize permutation of the sphere puzzle.
// .SECTION Description
// vtkSpherePuzzleArrows creates 

#ifndef __vtkSpherePuzzleArrows_h
#define __vtkSpherePuzzleArrows_h

#include "vtkPolyDataAlgorithm.h"

class vtkCellArray;
class vtkPoints;
class vtkSpherePuzzle;

class VTK_GRAPHICS_EXPORT vtkSpherePuzzleArrows : public vtkPolyDataAlgorithm 
{
public:
  vtkTypeMacro(vtkSpherePuzzleArrows,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkSpherePuzzleArrows *New();

  // Description:
  // Permutation is an array of puzzle piece ids.
  // Arrows will be generated for any id that does not contain itself.
  // Permutation[3] = 3 will produce no arrow.
  // Permutation[3] = 10 will draw an arrow from location 3 to 10.
  vtkSetVectorMacro(Permutation,int,32);
  vtkGetVectorMacro(Permutation,int,32);
  void SetPermutationComponent(int comp, int val);
  void SetPermutation(vtkSpherePuzzle *puz);

protected:
  vtkSpherePuzzleArrows();
  ~vtkSpherePuzzleArrows();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  void AppendArrow(int id0, int id1, vtkPoints *pts, vtkCellArray *polys);
  
  int Permutation[32];

  double Radius;

private:
  vtkSpherePuzzleArrows(const vtkSpherePuzzleArrows&); // Not implemented
  void operator=(const vtkSpherePuzzleArrows&); // Not implemented
};

#endif
