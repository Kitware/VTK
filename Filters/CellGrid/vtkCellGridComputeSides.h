// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellGridComputeSides
 * @brief   Generate sides of input data (cells and/or sides) based on a strategy.
 *
 * This filter simply adds or replaces a 2-component array for each type of
 * side, for each cell type which identifies the set sides which are "un-shared."
 * Internally, it uses a vtkCellGridSidesQuery to obtain sides, so
 * the cells in your vtkCellGrid must provide a responder for this query type.
 *
 * This filter can be set to indicate which output data is copied directly
 * from the input and which data is a set of sides generated from its input.
 * This is used by the cell-grid representation in ParaView to identify how
 * selections should be extracted: sides not marked as "original" will result
 * in their entire cell (not just the side) being extracted.
 *
 * @sa vtkCellGridSidesQuery
 */
#ifndef vtkCellGridComputeSides_h
#define vtkCellGridComputeSides_h

#include "vtkCellGridAlgorithm.h"
#include "vtkCellGridSidesQuery.h"    // For SideFlags enum.
#include "vtkFiltersCellGridModule.h" // For export macro.
#include "vtkNew.h"                   // For ivar.

VTK_ABI_NAMESPACE_BEGIN

class VTKFILTERSCELLGRID_EXPORT vtkCellGridComputeSides : public vtkCellGridAlgorithm
{
public:
  static vtkCellGridComputeSides* New();
  vtkTypeMacro(vtkCellGridComputeSides, vtkCellGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /// Set/get whether the output should include cells which are themselves
  /// renderable (surfaces, edges, or vertices) or should only include sides
  /// of the input cells.
  ///
  /// If you are implementing a responder, you are expected to employ this query parameter.
  virtual void SetPreserveRenderableInputs(bool preserve);
  bool GetPreserveRenderableInputs();
  vtkBooleanMacro(PreserveRenderableInputs, bool);

  /// Set/get whether sides should be computed if the cells are themselves
  /// renderable (surfaces, edges, or vertices).
  ///
  /// If this is true, no sides will be computed for inputs that are renderable.
  /// If false, then sides will be computed.
  /// Note that OmitSidesForRenderableInputs is distinct from PreserveRenderableInputs,
  /// which determines whether renderable cells should be copied to the output.
  ///
  /// The default is false.
  virtual void SetOmitSidesForRenderableInputs(bool omit);
  bool GetOmitSidesForRenderableInputs();
  vtkBooleanMacro(OmitSidesForRenderableInputs, bool);

  /// Re-export the bit-values that SetOutputDimensionControl accepts.
  using SideFlags = vtkCellGridSidesQuery::SideFlags;

  /// Set/get a bit-vector flag indicating which sides of which dimension to generate.
  ///
  /// \sa vtkCellGridSidesQuery
  virtual void SetOutputDimensionControl(int flags);
  int GetOutputDimensionControl();

  /// Re-export the enums SetStrategy accepts.
  using SummaryStrategy = vtkCellGridSidesQuery::SummaryStrategy;

  /// Set/get the strategy used to determine which input sides appear in the output.
  virtual void SetStrategy(SummaryStrategy strategy);
  SummaryStrategy GetStrategy();
  /// This method exists for ParaView to set the strategy.
  virtual void SetStrategy(int strategy);

  /// Re-export the bit-values that SetOutputDimensionControl accepts.
  using SelectionMode = vtkCellGridSidesQuery::SelectionMode;

  /// Set/get the selection type.
  ///
  /// This determines what shapes should be selected when output sides of
  /// this filter are picked by a user.
  virtual void SetSelectionType(SelectionMode selectionType);
  SelectionMode GetSelectionType();
  /// This method exists for ParaView to set the selection mode.
  virtual void SetSelectionType(int selnType);

  static vtkStringToken GetSideAttribute();

protected:
  vtkCellGridComputeSides();
  ~vtkCellGridComputeSides() override = default;

  int RequestData(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* ouInfo) override;

  vtkNew<vtkCellGridSidesQuery> Request;

private:
  vtkCellGridComputeSides(const vtkCellGridComputeSides&) = delete;
  void operator=(const vtkCellGridComputeSides&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellGridComputeSides_h
