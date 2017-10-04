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
/**
 * @class   vtkVertexGlyphFilter
 * @brief   Make a vtkPolyData with a vertex on each point.
 *
 *
 *
 * This filter throws away all of the cells in the input and replaces them with
 * a vertex on each point.  The intended use of this filter is roughly
 * equivalent to the vtkGlyph3D filter, except this filter is specifically for
 * data that has many vertices, making the rendered result faster and less
 * cluttered than the glyph filter. This filter may take a graph or point set
 * as input.
 *
*/

#ifndef vtkVertexGlyphFilter_h
#define vtkVertexGlyphFilter_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkVertexGlyphFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkVertexGlyphFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream &os, vtkIndent indent) override;
  static vtkVertexGlyphFilter *New();

protected:
  vtkVertexGlyphFilter();
  ~vtkVertexGlyphFilter() override;

  int RequestData(vtkInformation *,
                  vtkInformationVector **, vtkInformationVector *) override;
  int FillInputPortInformation(int, vtkInformation *) override;

private:
  vtkVertexGlyphFilter(const vtkVertexGlyphFilter &) = delete;
  void operator=(const vtkVertexGlyphFilter &) = delete;
};

#endif //_vtkVertexGlyphFilter_h
