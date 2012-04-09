/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVertexGlyphFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
// .NAME vtkVertexGlyphFilter - Make a vtkPolyData with a vertex on each point.
//
// .SECTION Description
//
// This filter throws away all of the cells in the input and replaces them with
// a vertex on each point.  The intended use of this filter is roughly
// equivalent to the vtkGlyph3D filter, except this filter is specifically for
// data that has many vertices, making the rendered result faster and less
// cluttered than the glyph filter. This filter may take a graph or point set
// as input.
//

#ifndef _vtkVertexGlyphFilter_h
#define _vtkVertexGlyphFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkVertexGlyphFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkVertexGlyphFilter, vtkPolyDataAlgorithm);
  virtual void PrintSelf(ostream &os, vtkIndent indent);
  static vtkVertexGlyphFilter *New();

protected:
  vtkVertexGlyphFilter();
  ~vtkVertexGlyphFilter();

  int RequestData(vtkInformation *,
                  vtkInformationVector **, vtkInformationVector *);
  int FillInputPortInformation(int, vtkInformation *);

private:
  vtkVertexGlyphFilter(const vtkVertexGlyphFilter &); // Not implemented
  void operator=(const vtkVertexGlyphFilter &);    // Not implemented
};

#endif //_vtkVertexGlyphFilter_h
