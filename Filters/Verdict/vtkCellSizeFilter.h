/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellSizeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCellSizeFilter
 * @brief   Computes cell sizes.
 *
 * Computes the cell sizes for all types of cells in VTK. For triangles,
 * quads, tets and hexes the static methods in vtkMeshQuality are used.
 * This is done through Verdict for higher accuracy.
 * Other cell types are individually done analytically where possible
 * and breaking into triangles or tets when not possible. When cells are
 * broken into triangles or tets the accuracy may be diminished. By default
 * all sizes are computed but point, length, area and volumetric cells
 * can each be optionally ignored. For dimensions of cells that do not
 * have their size computed, a value of 0 will be given. For cells that
 * should have their size computed but can't, the filter will return -1.
*/

#ifndef vtkCellSizeFilter_h
#define vtkCellSizeFilter_h

#include "vtkFiltersVerdictModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

class vtkDataSet;
class vtkIdList;
class vtkImageData;
class vtkPointSet;

class VTKFILTERSVERDICT_EXPORT vtkCellSizeFilter : public vtkPassInputTypeAlgorithm
{
public:
  vtkTypeMacro(vtkCellSizeFilter, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkCellSizeFilter* New();

  //@{
  /**
   * Specify whether or not to compute sizes for vertex and polyvertex
   * cells. The computed value is the number of points in the cell.
   */
  vtkSetMacro(ComputePoint, bool);
  vtkGetMacro(ComputePoint, bool);
  vtkBooleanMacro(ComputePoint, bool);
  //@}

  //@{
  /**
   * Specify whether or not to compute sizes for 1D cells
   * cells. The computed value is the length of the cell.
   */
  vtkSetMacro(ComputeLength, bool);
  vtkGetMacro(ComputeLength, bool);
  vtkBooleanMacro(ComputeLength, bool);
  //@}

  //@{
  /**
   * Specify whether or not to compute sizes for 2D cells
   * cells. The computed value is the area of the cell.
   */
  vtkSetMacro(ComputeArea, bool);
  vtkGetMacro(ComputeArea, bool);
  vtkBooleanMacro(ComputeArea, bool);
  //@}

  //@{
  /**
   * Specify whether or not to compute sizes for 3D cells
   * cells. The computed value is the volume of the cell.
   */
  vtkSetMacro(ComputeVolume, bool);
  vtkGetMacro(ComputeVolume, bool);
  vtkBooleanMacro(ComputeVolume, bool);
  //@}

  //@{
  /**
   * Set/Get the name of the computed array. Default name is "size".
   */
  vtkSetStringMacro(ArrayName);
  vtkGetStringMacro(ArrayName);
  //@}

protected:
  vtkCellSizeFilter();
  ~vtkCellSizeFilter();

  virtual int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;

  void IntegrateImageData(vtkImageData* input, vtkImageData* output);

  double IntegratePolyLine(vtkDataSet* input, vtkIdList* cellPtIds);
  double IntegratePolygon(vtkPointSet* input, vtkIdList* cellPtIds);
  double IntegrateTriangleStrip(vtkPointSet* input, vtkIdList* cellPtIds);
  double IntegratePixel(vtkDataSet* input, vtkIdList* cellPtIds);
  double IntegrateVoxel(vtkDataSet* input, vtkIdList* cellPtIds);
  double IntegrateGeneral1DCell(vtkDataSet* input, vtkIdList* cellPtIds);
  double IntegrateGeneral2DCell(vtkPointSet* input, vtkIdList* cellPtIds);
  double IntegrateGeneral3DCell(vtkPointSet* input, vtkIdList* cellPtIds);

private:
  vtkCellSizeFilter(const vtkCellSizeFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCellSizeFilter&) VTK_DELETE_FUNCTION;

  void ExecuteBlock(vtkDataSet* input, vtkDataSet* output);

  bool ComputePoint;
  bool ComputeLength;
  bool ComputeArea;
  bool ComputeVolume;

  char* ArrayName;
};

#endif
