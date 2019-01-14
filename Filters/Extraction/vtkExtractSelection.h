/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtractSelection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkExtractSelection
 * @brief   extract a subset from a vtkDataSet.
 *
 * vtkExtractSelection extracts some subset of cells and points from
 * its input dataobject. The dataobject is given on its first input port.
 * The subset is described by the contents of the vtkSelection on its
 * second input port.  Depending on the contents of the vtkSelection
 * this will create various vtkSelectors to identify the
 * selected elements.
 *
 * This filter supports vtkCompositeDataSet (output is vtkMultiBlockDataSet),
 * vtkTable and vtkDataSet (output is vtkUnstructuredGrid).
 * Other types of input are not processed and the corresponding output is a
 * default constructed object of the input type.
 *
 * @sa
 * vtkSelection vtkSelector vtkSelectionNode
*/

#ifndef vtkExtractSelection_h
#define vtkExtractSelection_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkDataObjectAlgorithm.h"

#include "vtkSelectionNode.h" // for vtkSelectionNode::SelectionContent
#include "vtkSmartPointer.h"  // for smart pointer

class vtkSignedCharArray;
class vtkSelection;
class vtkSelectionNode;
class vtkSelector;
class vtkUnstructuredGrid;
class vtkTable;

class VTKFILTERSEXTRACTION_EXPORT vtkExtractSelection : public vtkDataObjectAlgorithm
{
public:
  static vtkExtractSelection *New();
  vtkTypeMacro(vtkExtractSelection, vtkDataObjectAlgorithm);
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
   * This flag tells the extraction filter not to extract a subset of the
   * data, but instead to produce a vtkInsidedness array and add it to the
   * input dataset. Default value is false(0).
   */
  vtkSetMacro(PreserveTopology, bool);
  vtkGetMacro(PreserveTopology, bool);
  vtkBooleanMacro(PreserveTopology, bool);
  //@}


  //@{
  /**
   * These functions is provided for compile-time compatibility with the old
   * vtkExtractSelection which is now renamed to vtkExtractSelectionLegacy and deprecated.
   * These functions do not have any effect on the behavior or vtkExtractSelection.
   */
  VTK_LEGACY(void SetShowBounds(bool));
  VTK_LEGACY(bool GetShowBounds());
  VTK_LEGACY(void ShowBoundsOn());
  VTK_LEGACY(void ShowBoundsOff());
  //@}

  //@{
  /**
   * These functions is provided for compile-time compatibility with the old
   * vtkExtractSelection which is now renamed to vtkExtractSelectionLegacy and deprecated.
   * These functions do not have any effect on the behavior or vtkExtractSelection.
   */
  VTK_LEGACY(void SetUseProbeForLocations(bool));
  VTK_LEGACY(bool GetUseProbeForLocations());
  VTK_LEGACY(void UseProbeForLocationsOn());
  VTK_LEGACY(void UseProbeForLocationsOff());
  //@}

protected:
  vtkExtractSelection();
  ~vtkExtractSelection() override;

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

  /**
   * Creates a new vtkSelector for the given content type.
   * May return null if not supported.
   */
  virtual vtkSmartPointer<vtkSelector> NewSelectionOperator(
    vtkSelectionNode::SelectionContent type);

  /**
   * Given a non-composite input data object (either a block of a larger composite
   * or the whole input), along with the element type being extracted and the
   * computed insidedness array this method either copies the input and adds the
   * insidedness array (if PreserveTopology is on) or returns a new data object
   * containing only the elements to be extracted.
   */
  vtkSmartPointer<vtkDataObject> ExtractElements(vtkDataObject* block,
    vtkDataObject::AttributeTypes elementType, vtkSignedCharArray* insidednessArray);

  int FillInputPortInformation(int port, vtkInformation* info) override;

  /**
   * Given a vtkDataSet and an array of which cells to extract, this populates
   * the given vtkUnstruturedGrid with the selected cells.
   */
  void ExtractSelectedCells(vtkDataSet* input,
                            vtkUnstructuredGrid* output,
                            vtkSignedCharArray* cellInside);
  /**
   * Given a vtkDataSet and an array of which points to extract, the populates
   * the given vtkUnstructuredGrid with the selected points and a cell of type vertex
   * for each point.
   */
  void ExtractSelectedPoints(vtkDataSet* input,
                             vtkUnstructuredGrid* output,
                             vtkSignedCharArray* pointInside);
  /**
   * Given an input vtkTable and an array of which rows to extract, this populates
   * the output table with the selected rows.
   */
  void ExtractSelectedRows(vtkTable* input,
                           vtkTable* output,
                           vtkSignedCharArray* rowsInside);

  bool PreserveTopology;

private:
  vtkExtractSelection(const vtkExtractSelection&) = delete;
  void operator=(const vtkExtractSelection&) = delete;

};

#endif
