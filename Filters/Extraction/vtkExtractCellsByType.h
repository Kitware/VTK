/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractCellsByType.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   vtkExtractCellsByType
 * @brief   extract cells of a specified type
 *
 *
 * Given an input vtkDataSet and a list of cell types, produce an output
 * dataset containing only cells of the specified type(s). Note that if the
 * input dataset is homogeneous (e.g., all cells are of the same type) and
 * the cell type is one of the cells specified, then the input dataset is
 * shallow copied to the output.
 *
 * The type of output dataset is always the same as the input type. Since
 * structured types of data (i.e., vtkImageData, vtkStructuredGrid,
 * vtkRectilnearGrid, vtkUniformGrid) are all composed of a cell of the same
 * type, the output is either empty, or a shallow copy of the
 * input. Unstructured data (vtkUnstructuredGrid, vtkPolyData) input may
 * produce a subset of the input data (depending on the selected cell types).
 *
 * Note this filter can be used in a pipeline with composite datasets to
 * extract blocks of (a) particular cell type(s).
 *
 * @warning
 * Unlike the filter vtkExtractCells which always produces
 * vtkUnstructuredGrid output, this filter produces the same output type as
 * input type (i.e., it is a vtkDataSetAlgorithm). Also, vtkExtractCells
 * extracts cells based on their ids.

 * @sa
 * vtkExtractBlock vtkExtractCells
 */

#ifndef vtkExtractCellsByType_h
#define vtkExtractCellsByType_h

#include "vtkDataSetAlgorithm.h"
#include "vtkFiltersExtractionModule.h" // For export macro

struct vtkCellTypeSet;
class vtkIdTypeArray;

class VTKFILTERSEXTRACTION_EXPORT vtkExtractCellsByType : public vtkDataSetAlgorithm
{
public:
  //@{
  /**
   * Standard methods for construction, type info, and printing.
   */
  static vtkExtractCellsByType* New();
  vtkTypeMacro(vtkExtractCellsByType, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  //@}

  //@{
  /**
   * Specify the cell types to extract. Any cells of the type specified are
   * extracted. Methods for clearing the set of cells, adding all cells, and
   * determining if a cell is in the set are also provided.
   */
  void AddCellType(unsigned int type);
  void AddAllCellTypes();
  void RemoveCellType(unsigned int type);
  void RemoveAllCellTypes();
  bool ExtractCellType(unsigned int type);
  //@}

protected:
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

  void ExtractUnstructuredData(vtkDataSet* input, vtkDataSet* output);
  void ExtractPolyDataCells(
    vtkDataSet* input, vtkDataSet* output, vtkIdType* ptMap, vtkIdType& numNewPts);
  void ExtractUnstructuredGridCells(
    vtkDataSet* input, vtkDataSet* output, vtkIdType* ptMap, vtkIdType& numNewPts);

  vtkExtractCellsByType();
  ~vtkExtractCellsByType() override;

private:
  vtkExtractCellsByType(const vtkExtractCellsByType&) = delete;
  void operator=(const vtkExtractCellsByType&) = delete;

  vtkCellTypeSet* CellTypes;
};

#endif
