/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCellTypeSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkCellTypeSource
 * @brief   Create cells of a given type
 *
 * vtkCellTypeSource is a source object that creates cells of the given
 * input type. BlocksDimensions specifies the number of cell "blocks" in each
 * direction. A cell block may be divided into multiple cells based on
 * the chosen cell type (e.g. 6 pyramid cells make up a single cell block).
 * If a 1D cell is selected then only the first dimension is
 * used to specify how many cells are generated. If a 2D cell is
 * selected then only the first and second dimensions are used to
 * determine how many cells are created. The source respects pieces.
 */

#ifndef vtkCellTypeSource_h
#define vtkCellTypeSource_h

#include "vtkFiltersSourcesModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

class VTKFILTERSSOURCES_EXPORT vtkCellTypeSource : public vtkUnstructuredGridAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiation, obtaining type and printing instance values.
   */
  static vtkCellTypeSource *New();
  vtkTypeMacro(vtkCellTypeSource,vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  //@}

  //@{
  /**
   * Set/Get the type of cells to be generated.
   */
  void SetCellType(int cellType);
  vtkGetMacro(CellType, int);
  //@}

  //@{
  /**
   * Get the dimension of the cell blocks to be generated
   */
  int GetCellDimension();
  //@}

  //@{
  /**
   * Set/get the desired precision for the output points.
   * vtkAlgorithm::SINGLE_PRECISION - Output single-precision floating point.
   * vtkAlgorithm::DOUBLE_PRECISION - Output double-precision floating point.
   */
  vtkSetMacro(OutputPointsPrecision,int);
  vtkGetMacro(OutputPointsPrecision,int);
  //@}

  //@{
  /**
   * Set the number of cells in each direction. If a 1D cell type is
   * selected then only the first dimension is used and if a 2D cell
   * type is selected then the first and second dimensions are used.
   * Default is (1, 1, 1), which results in a single block of cells.
   */
  void SetBlocksDimensions(int*);
  void SetBlocksDimensions(int, int, int);
  vtkGetVector3Macro(BlocksDimensions, int);
  //@}

protected:
  vtkCellTypeSource();
  ~vtkCellTypeSource() VTK_OVERRIDE {}

  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;

  void GenerateTriangles(vtkUnstructuredGrid*, int extent[6]);
  void GenerateQuads(vtkUnstructuredGrid*, int extent[6]);
  void GenerateQuadraticTriangles(vtkUnstructuredGrid*, int extent[6]);
  void GenerateQuadraticQuads(vtkUnstructuredGrid*, int extent[6]);
  void GenerateTetras(vtkUnstructuredGrid*, int extent[6]);
  void GenerateHexahedron(vtkUnstructuredGrid*, int extent[6]);
  void GenerateWedges(vtkUnstructuredGrid*, int extent[6]);
  void GeneratePyramids(vtkUnstructuredGrid*, int extent[6]);
  void GenerateQuadraticTetras(vtkUnstructuredGrid*, int extent[6]);
  void GenerateQuadraticHexahedron(vtkUnstructuredGrid*, int extent[6]);
  void GenerateQuadraticWedges(vtkUnstructuredGrid*, int extent[6]);
  void GenerateQuadraticPyramids(vtkUnstructuredGrid*, int extent[6]);

  int BlocksDimensions[3];
  int CellType;
  int OutputPointsPrecision;

private:
  vtkCellTypeSource(const vtkCellTypeSource&) VTK_DELETE_FUNCTION;
  void operator=(const vtkCellTypeSource&) VTK_DELETE_FUNCTION;
};

#endif
