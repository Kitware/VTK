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
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct object with vertex cell generation turned off.
   */
  static vtkCellCenters *New();

  //@{
  /**
   * Enable/disable the generation of vertex cells. The default
   * is Off.
   */
  vtkSetMacro(VertexCells,vtkTypeBool);
  vtkGetMacro(VertexCells,vtkTypeBool);
  vtkBooleanMacro(VertexCells,vtkTypeBool);
  //@}

protected:
  vtkCellCenters();
  ~vtkCellCenters() override {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) override;
  int FillInputPortInformation(int port, vtkInformation *info) override;

  vtkTypeBool VertexCells;
private:
  vtkCellCenters(const vtkCellCenters&) = delete;
  void operator=(const vtkCellCenters&) = delete;
};

#endif
