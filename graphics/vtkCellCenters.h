/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellCenters.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkCellCenters - generate points at center of cells
// .SECTION Description
// vtkCellCenters is a filter that takes as input any dataset and 
// generates on output points at the center of the cells in the dataset.
// These points can be used for placing glyphs (vtkGlyph3D) or labeling 
// (vtkLabeledDataMapper). (The center is the parametric center of the
// cell, not necessarily the geometric or bounding box center.) The cell
// attributes will be associated with the points on output.
// 
// .SECTION Caveats
// You can choose to generate just points or points and vertex cells.
// Vertex cells are drawn during rendering; points are not. Use the ivar
// VertexCells to generate cells.

// .SECTION See Also
// vtkGlyph3D vtkLabeledDataMapper

#ifndef __vtkCellCenters_h
#define __vtkCellCenters_h

#include "vtkDataSetToPolyDataFilter.h"

class VTK_EXPORT vtkCellCenters : public vtkDataSetToPolyDataFilter
{
public:

// Description:
// Construct object with vertex cell generation turned off.
  vtkCellCenters();

  static vtkCellCenters *New() {return new vtkCellCenters;};
  const char *GetClassName() {return "vtkCellCenters";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Enable/disable the generation of vertex cells.
  vtkSetMacro(VertexCells,int);
  vtkGetMacro(VertexCells,int);
  vtkBooleanMacro(VertexCells,int);

protected:
  void Execute();

  int VertexCells;
};

#endif
