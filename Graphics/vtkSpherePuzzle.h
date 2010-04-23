/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSpherePuzzle.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSpherePuzzle - create a polygonal sphere centered at the origin
// .SECTION Description
// vtkSpherePuzzle creates 

#ifndef __vtkSpherePuzzle_h
#define __vtkSpherePuzzle_h

#include "vtkPolyDataAlgorithm.h"

#define VTK_MAX_SPHERE_RESOLUTION 1024

class vtkTransform;

class VTK_GRAPHICS_EXPORT vtkSpherePuzzle : public vtkPolyDataAlgorithm 
{
public:
  vtkTypeMacro(vtkSpherePuzzle,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  static vtkSpherePuzzle *New();

  // Description:
  // Reset the state of this puzzle back to its original state.
  void Reset();

  // Description:
  // Move the top/bottom half one segment either direction.
  void MoveHorizontal(int section, int percentage, int rightFlag);

  // Description:
  // Rotate vertical half of sphere along one of the longitude lines.
  void MoveVertical(int section, int percentage, int rightFlag);

  // Description:
  // SetPoint will be called as the mouse moves over the screen.
  // The output will change to indicate the pending move.
  // SetPoint returns zero if move is not activated by point.
  // Otherwise it encodes the move into a unique integer so that
  // the caller can determine if the move state has changed.
  // This will answer the question, "Should I render."
  int SetPoint(double x, double y, double z);  

  // Description:
  // Move actually implements the pending move. When percentage
  // is 100, the pending move becomes inactive, and SetPoint
  // will have to be called again to setup another move.
  void MovePoint(int percentage);  

  // Description:
  // For drawing state as arrows.
  int *GetState() {return this->State;}

protected:
  vtkSpherePuzzle();
  ~vtkSpherePuzzle();

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  void MarkVertical(int section);
  void MarkHorizontal(int section);  
  
  int State[32];
  
  // Stuff for storing a partial move.
  int PieceMask[32];
  vtkTransform *Transform;

  // Colors for faces.
  unsigned char Colors[96];

  // State for potential move.
  int Active;
  int VerticalFlag;
  int RightFlag;
  int Section;

private:
  vtkSpherePuzzle(const vtkSpherePuzzle&); // Not implemented
  void operator=(const vtkSpherePuzzle&); // Not implemented
};

#endif
