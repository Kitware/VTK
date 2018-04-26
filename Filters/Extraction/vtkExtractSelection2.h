/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelection2.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkExtractSelection2
 * @brief   extract a subset from a vtkDataSet.
 *
 * vtkExtractSelection extracts some subset of cells and points from
 * its input dataset. The dataset is given on its first input port.
 * The subset is described by the contents of the vtkSelection on its
 * second input port. Depending on the content of the vtkSelection,
 * this will use either a vtkExtractSelectedIds, vtkExtractSelectedFrustum
 * vtkExtractSelectedLocations or a vtkExtractSelectedThreshold to perform
 * the extraction.
 * @sa
 * vtkSelection vtkExtractSelectedIds vtkExtractSelectedFrustum
 * vtkExtractSelectedLocations vtkExtractSelectedThresholds
*/

#ifndef vtkExtractSelection2_h
#define vtkExtractSelection2_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkDataObjectAlgorithm.h"

#include "vtkSmartPointer.h" // for smart pointer

class vtkSignedCharArray;
class vtkSelection;
class vtkSelectionNode;
class vtkSelectionOperator;
class vtkUnstructuredGrid;

class VTKFILTERSEXTRACTION_EXPORT vtkExtractSelection2 : public vtkDataObjectAlgorithm
{
public:
  static vtkExtractSelection2 *New();
  vtkTypeMacro(vtkExtractSelection2, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Convenience method to specify the selection connection (2nd input
   * port)
   */
  void SetSelectionConnection(vtkAlgorithmOutput* algOutput)
  {
    this->SetInputConnection(1, algOutput);
  }

  //@{
  /**
   * This flag tells the extraction filter not to convert the selected
   * output into an unstructured grid, but instead to produce a vtkInsidedness
   * array and add it to the input dataset. Default value is false(0).
   */
  vtkSetMacro(PreserveTopology, vtkTypeBool);
  vtkGetMacro(PreserveTopology, vtkTypeBool);
  vtkBooleanMacro(PreserveTopology, vtkTypeBool);
  //@}

protected:
  vtkExtractSelection2();
  ~vtkExtractSelection2() override;

  /**
   * Sets up empty output dataset
   */
  int RequestDataObject(vtkInformation* request,
                        vtkInformationVector** inputVector,
                        vtkInformationVector* outputVector) override;
  /**
   * Sets up empty output dataset
   */
  int RequestData(vtkInformation* request,
                        vtkInformationVector** inputVector,
                        vtkInformationVector* outputVector) override;

  // Gets the attribute association of the selection.  Currently we support ROW, POINT, and CELL.
  // If the selection types are mismatched the boolean parameter will be set to false, otherwise
  // it will be true after the function returns.
  vtkDataObject::AttributeTypes GetAttributeTypeOfSelection(vtkSelection* sel, bool& sane);
  vtkSelectionOperator* GetOperatorForNode(vtkSelectionNode* node);

  void ComputeCellsContainingSelectedPoints(vtkDataObject* data,
                                            vtkSignedCharArray* selectedPoints,
                                            vtkSignedCharArray* selectedCells);

  vtkSmartPointer<vtkSignedCharArray> ComputeSelectedElements(vtkDataObject* data,
                                                              vtkIdType flatIndex,
                                                              vtkIdType level,
                                                              vtkIdType hbIndex,
                                                              vtkSelection* selection);

  vtkDataObject* ExtractFromBlock(vtkDataObject* block,
                                  vtkIdType flatIndex,
                                  vtkIdType level,
                                  vtkIdType hbIndex,
                                  vtkSelection* selection);

  int FillInputPortInformation(int port, vtkInformation* info) override;

  void ExtractSelectedCells(vtkDataSet* input,
                            vtkUnstructuredGrid* output,
                            vtkSignedCharArray* cellInside);
  void ExtractSelectedPoints(vtkDataSet* input,
                             vtkUnstructuredGrid* output,
                             vtkSignedCharArray* pointInside);

  vtkTypeBool PreserveTopology;
private:
  vtkExtractSelection2(const vtkExtractSelection2&) = delete;
  void operator=(const vtkExtractSelection2&) = delete;

};

#endif
