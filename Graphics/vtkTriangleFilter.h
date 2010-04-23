/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkTriangleFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkTriangleFilter - create triangle polygons from input polygons and triangle strips
// .SECTION Description
// vtkTriangleFilter generates triangles from input polygons and triangle 
// strips. The filter also will pass through vertices and lines, if
// requested.

#ifndef __vtkTriangleFilter_h
#define __vtkTriangleFilter_h

#include "vtkPolyDataAlgorithm.h"

class VTK_GRAPHICS_EXPORT vtkTriangleFilter : public vtkPolyDataAlgorithm
{
public:
  static vtkTriangleFilter *New();
  vtkTypeMacro(vtkTriangleFilter,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Turn on/off passing vertices through filter.
  vtkBooleanMacro(PassVerts,int);
  vtkSetMacro(PassVerts,int);
  vtkGetMacro(PassVerts,int);

  // Description:
  // Turn on/off passing lines through filter.
  vtkBooleanMacro(PassLines,int);
  vtkSetMacro(PassLines,int);
  vtkGetMacro(PassLines,int);

protected:
  vtkTriangleFilter() : PassVerts(1), PassLines(1) {};
  ~vtkTriangleFilter() {};

  // Usual data generation method
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  int PassVerts;
  int PassLines;
private:
  vtkTriangleFilter(const vtkTriangleFilter&);  // Not implemented.
  void operator=(const vtkTriangleFilter&);  // Not implemented.
};

#endif
