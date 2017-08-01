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
class vtkDoubleArray;
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
   * This option is enabled by default.
   */
  vtkSetMacro(ComputePoint, bool);
  vtkGetMacro(ComputePoint, bool);
  vtkBooleanMacro(ComputePoint, bool);
  //@}

  //@{
  /**
   * Specify whether or not to compute sizes for 1D cells
   * cells. The computed value is the length of the cell.
   * This option is enabled by default.
   */
  vtkSetMacro(ComputeLength, bool);
  vtkGetMacro(ComputeLength, bool);
  vtkBooleanMacro(ComputeLength, bool);
  //@}

  //@{
  /**
   * Specify whether or not to compute sizes for 2D cells
   * cells. The computed value is the area of the cell.
   * This option is enabled by default.
   */
  vtkSetMacro(ComputeArea, bool);
  vtkGetMacro(ComputeArea, bool);
  vtkBooleanMacro(ComputeArea, bool);
  //@}

  //@{
  /**
   * Specify whether or not to compute sizes for 3D cells
   * cells. The computed value is the volume of the cell.
   * This option is enabled by default.
   */
  vtkSetMacro(ComputeVolume, bool);
  vtkGetMacro(ComputeVolume, bool);
  vtkBooleanMacro(ComputeVolume, bool);
  //@}

  //@{
  /**
   * Specify to compute sizes only for highest dimension cells in
   * a vtkDataSet. If the input dataset is a composite dataset the
   * highest cell dimension is computed individually for each leaf.
   * If this option is enabled then the ComputePoint, ComputeLength,
   * ComputeArea and ComputeVolume options are ignored. This option
   * is disabled by default.
   */
  vtkSetMacro(ComputeHighestDimension, bool);
  vtkGetMacro(ComputeHighestDimension, bool);
  vtkBooleanMacro(ComputeHighestDimension, bool);
  //@}

  //@{
  /**
   * Specify whether to sum the computed sizes and put the result in
   * a field data array. This option is disabled by default.
   */
  vtkSetMacro(ComputeSum, bool);
  vtkGetMacro(ComputeSum, bool);
  vtkBooleanMacro(ComputeSum, bool);
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
  ~vtkCellSizeFilter() VTK_OVERRIDE;

  virtual int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) VTK_OVERRIDE;
  bool ComputeDataSet(vtkDataSet* input, vtkDataSet* output, vtkDoubleArray* sum);

  void IntegrateImageData(vtkImageData* input, vtkImageData* output, vtkDoubleArray* sum);
  void ExecuteBlock(vtkDataSet* input, vtkDataSet* output, vtkDoubleArray* sum);

  //@{
  /**
   * Specify whether to sum the computed sizes and put the result in
   * a field data array. This option is disabled by default.
   */
  double IntegratePolyLine(vtkDataSet* input, vtkIdList* cellPtIds);
  double IntegratePolygon(vtkPointSet* input, vtkIdList* cellPtIds);
  double IntegrateTriangleStrip(vtkPointSet* input, vtkIdList* cellPtIds);
  double IntegratePixel(vtkDataSet* input, vtkIdList* cellPtIds);
  double IntegrateVoxel(vtkDataSet* input, vtkIdList* cellPtIds);
  double IntegrateGeneral1DCell(vtkDataSet* input, vtkIdList* cellPtIds);
  double IntegrateGeneral2DCell(vtkPointSet* input, vtkIdList* cellPtIds);
  double IntegrateGeneral3DCell(vtkPointSet* input, vtkIdList* cellPtIds);
  //@}

  //@{
  /**
   * Method to compute the global sum information. For serial operation this is a no-op.
   */
  virtual void ComputeGlobalSum(vtkDoubleArray*) {};
  //@}

private:
  vtkCellSizeFilter(const vtkCellSizeFilter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCellSizeFilter&) VTK_DELETE_FUNCTION;

  bool ComputePoint;
  bool ComputeLength;
  bool ComputeArea;
  bool ComputeVolume;
  bool ComputeHighestDimension;
  bool ComputeSum;

  char* ArrayName;
};

#endif
