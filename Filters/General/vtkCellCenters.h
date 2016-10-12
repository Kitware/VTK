/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellCenters.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCellCenters
 * @brief   generate points at center of cells
 *
 * vtkCellCenters is a filter that takes as input any dataset and
 * generates on output points at the center of the cells in the dataset.
 * These points can be used for placing glyphs (vtkGlyph3D) or labeling
 * (vtkLabeledDataMapper). (The center is the parametric center of the
 * cell, not necessarily the geometric or bounding box center.) The cell
 * attributes will be associated with the points on output.
 *
 * @warning
 * You can choose to generate just points or points and vertex cells.
 * Vertex cells are drawn during rendering; points are not. Use the ivar
 * VertexCells to generate cells.
 *
 * @sa
 * vtkGlyph3D vtkLabeledDataMapper
*/

#ifndef vtkCellCenters_h
#define vtkCellCenters_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class VTKFILTERSGENERAL_EXPORT vtkCellCenters : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkCellCenters,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  /**
   * Construct object with vertex cell generation turned off.
   */
  static vtkCellCenters *New();

  //@{
  /**
   * Enable/disable the generation of vertex cells. The default
   * is Off.
   */
  vtkSetMacro(VertexCells,int);
  vtkGetMacro(VertexCells,int);
  vtkBooleanMacro(VertexCells,int);
  //@}

protected:
  vtkCellCenters();
  ~vtkCellCenters() VTK_OVERRIDE {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  int FillInputPortInformation(int port, vtkInformation *info) VTK_OVERRIDE;

  int VertexCells;
private:
  vtkCellCenters(const vtkCellCenters&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCellCenters&) VTK_DELETE_FUNCTION;
};

#endif
