// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkCellGridCrinkle
 * @brief   Adds sides .
 *
 * This filter outputs subsets of the input that cross the zero of or lie
 * within a halfspace of some implicit function, F(x,y,z).
 *
 * The implicit function is specified as a vtkImplicitFunction object.
 * Use SetFunction() to provide a function and SetHalfSpace() to choose
 * whether F > 0 or F < 0 denotes the "inside" of the output.
 *
 * ## Zero-crossing mode
 *
 * In zero-crossing mode, only input cells or sides that straddle the
 * zero of the implicit function (i.e., F(x,y,z) = 0) are copied to
 * the output.
 *
 * If you set SidesOfInput to true, then any input side or cell of
 * dimension D that straddles the implicit-function's zero will not
 * be directly copied but have its sides of dimension D-1 that straddle
 * or lie to one side of F = 0 added to the output. This is used
 * to obtain behavior like ParaView's existing crinkle-slice filter.
 *
 * ```text
 *          Input data                                  Output data
 *
 *          \<-- F = 0    SidesOfInput:     true           true          false
 *           \            HalfSpace:      Positive        Negative        N/A
 *  F < 0     \    F > 0
 *          +--\-----+                   +--\-----+     +--\-----+     +--\-----+
 *          |xxx\xxxx|                       \    |     |   \          |xxx\xxxx|
 *          |xxxx\xxx|        ===>            \   |     |    \         |xxxx\xxx|
 *          |xxxxx\xx|                         \  |     |     \        |xxxxx\xx|
 *          +------\-+                   +------\-+     +------\-+     +------\-+
 *                  \
 *    + = cell vertex
 *    | = cell edge
 *    x = cell interior
 * ```
 *
 * If SidesOfInput is false, then the input is subsetted rather than
 * a (D-1)-dimensional surface extracted. The output subset consists
 * solely of inputs which straddle F = 0.
 *
 * ## Halfspace mode
 *
 * Cells or sides of the input that lie entirely within the halfspace
 * (or if IncludeOverlapping is true, those that straddle F = 0 as well)
 * are copied to the output. Other inputs are omitted from the output.
 *
 * ## Cell-trimming mode
 *
 * In cell-trimming mode, inputs that straddle F = 0 are augmented with
 * a simplicial decomposition of their parameter-space that has been
 * trimmed to match the shape of F = 0.
 *
 * @sa vtkCellGridCrinkleQuery
 */
#ifndef vtkCellGridCrinkle_h
#define vtkCellGridCrinkle_h

#include "vtkCellGridAlgorithm.h"
#include "vtkCellGridSidesQuery.h"    // For SideFlags enum.
#include "vtkFiltersCellGridModule.h" // For export macro.
#include "vtkNew.h"                   // For ivar.

VTK_ABI_NAMESPACE_BEGIN

class VTKFILTERSCELLGRID_EXPORT vtkCellGridCrinkle : public vtkCellGridAlgorithm
{
public:
  static vtkCellGridCrinkle* New();
  vtkTypeMacro(vtkCellGridCrinkle, vtkCellGridAlgorithm);
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

  /// Re-export the bit-values that SetOutputDimensionControl accepts.
  using SelectionMode = vtkCellGridSidesQuery::SelectionMode;

  /// Set/get the selection type.
  ///
  /// This determines what shapes should be selected when output sides of
  /// this filter are picked by a user.
  virtual void SetSelectionType(SelectionMode selectionType);
  SelectionMode GetSelectionType();

  static vtkStringToken GetSideAttribute();

protected:
  vtkCellGridCrinkle();
  ~vtkCellGridCrinkle() override = default;

  int RequestData(
    vtkInformation* request, vtkInformationVector** inInfo, vtkInformationVector* ouInfo) override;

  vtkNew<vtkCellGridSidesQuery> Request;

private:
  vtkCellGridCrinkle(const vtkCellGridCrinkle&) = delete;
  void operator=(const vtkCellGridCrinkle&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCellGridCrinkle_h
