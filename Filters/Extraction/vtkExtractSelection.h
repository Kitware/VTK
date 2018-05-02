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

#ifndef vtkExtractSelection_h
#define vtkExtractSelection_h

#include "vtkFiltersExtractionModule.h" // For export macro
#include "vtkDataObjectAlgorithm.h"

#include "vtkSelectionNode.h" // for vtkSelectionNode::SelectionContent
#include "vtkSmartPointer.h"  // for smart pointer

class vtkSignedCharArray;
class vtkSelection;
class vtkSelectionNode;
class vtkSelectionOperator;
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
   * This flag tells the extraction filter not to convert the selected
   * output into an unstructured grid, but instead to produce a vtkInsidedness
   * array and add it to the input dataset. Default value is false(0).
   */
  vtkSetMacro(PreserveTopology, bool);
  vtkGetMacro(PreserveTopology, bool);
  vtkBooleanMacro(PreserveTopology, bool);
  //@}


#ifndef VTK_LEGACY_REMOVE
  //@{
  /**
   * These functions is provided for compile-time compatibility with the old
   * vtkExtractSelection which is now renamed to vtkExtractSelectionLegacy and deprecated.
   * These functions do not have any effect on the behavior or vtkExtractSelection.
   */
  VTK_LEGACY(void SetShowBounds(vtkTypeBool));
  VTK_LEGACY(vtkTypeBool GetShowBounds());
  VTK_LEGACY(vtkBooleanMacro(ShowBounds,vtkTypeBool));
  //@}

  //@{
  /**
   * These functions is provided for compile-time compatibility with the old
   * vtkExtractSelection which is now renamed to vtkExtractSelectionLegacy and deprecated.
   * These functions do not have any effect on the behavior or vtkExtractSelection.
   */
  VTK_LEGACY(void SetUseProbeForLocations(vtkTypeBool));
  VTK_LEGACY(vtkTypeBool GetUseProbeForLocations());
  VTK_LEGACY(vtkBooleanMacro(UseProbeForLocations, vtkTypeBool));
  //@}
#endif

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
   * Creates a new vtkSelectionOperator for the given content type.
   * May return null if not supported.
   */
  virtual vtkSmartPointer<vtkSelectionOperator> NewSelectionOperator(
    vtkSelectionNode::SelectionContent type);

  vtkSmartPointer<vtkDataObject> ExtractElements(vtkDataObject* block,
    vtkDataObject::AttributeTypes elementType, vtkSignedCharArray* insidednessArray);

  int FillInputPortInformation(int port, vtkInformation* info) override;

  void ExtractSelectedCells(vtkDataSet* input,
                            vtkUnstructuredGrid* output,
                            vtkSignedCharArray* cellInside);
  void ExtractSelectedPoints(vtkDataSet* input,
                             vtkUnstructuredGrid* output,
                             vtkSignedCharArray* pointInside);
void ExtractSelectedRows(vtkTable* input,
                         vtkTable* output,
                         vtkSignedCharArray* rowsInside);

  bool PreserveTopology;

private:
  vtkExtractSelection(const vtkExtractSelection&) = delete;
  void operator=(const vtkExtractSelection&) = delete;

};

#endif
