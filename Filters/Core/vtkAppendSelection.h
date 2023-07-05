// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Sandia Corporation
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAppendSelection
 * @brief   appends one or more selections together
 *
 * vtkAppendSelection is a filter that appends one of more selections into
 * a single selection.
 *
 * If AppendByUnion is true, all selections must have the same content
 * type and they are combined together to form a single vtkSelection output.
 *
 * If AppendByUnion is false, the output is a composite selection with
 * input selections as the children of the composite selection. This allows
 * for selections with different content types and properties.
 *
 * If AppendByUnion is true, an Expression can be defined which uses the input
 * selection names to define the relation between the selections. If the Expression
 * is not defined, all the selection are combined using the '| 'boolean operator.
 *
 * @warning It should be noted that this filter is not responsible for checking if
 * the field type is the same across all selections nodes of all selections.
 */

#ifndef vtkAppendSelection_h
#define vtkAppendSelection_h

#include "vtkFiltersCoreModule.h" // For export macro
#include "vtkSelectionAlgorithm.h"

#include <memory> // For std::unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class vtkSelection;

class VTKFILTERSCORE_EXPORT vtkAppendSelection : public vtkSelectionAlgorithm
{
public:
  static vtkAppendSelection* New();

  vtkTypeMacro(vtkAppendSelection, vtkSelectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the expression that defines the boolean expression to combine the
   * selections. Expression consists of node name identifiers, `|` for
   * boolean-or, '^' for boolean-xor, '&' for boolean and, '!' for boolean not,
   * and parenthesis `(` and `)`. If you want to set the expression, be sure to
   * define the node names. If no expression is specified and there are multiple
   * selections, then the default expression simply combines all the selection nodes
   * using an `or` operator.
   *
   * `SetExpression` does not validate the expression. It will be validated
   * `internally.
   *
   * NOTE: If you want to use the expression, AppendByUnion MUST be set to false,
   * and the input selection names MUST be defined.
   */
  vtkSetStdStringFromCharMacro(Expression);
  vtkGetCharFromStdStringMacro(Expression);
  ///@}

  ///@{
  /**
   * Determines whether the selection describes what to include or exclude.
   *
   * The default is false, meaning include.
   *
   * NOTE: If you want to use Inverse, AppendByUnion MUST be set to false.
   * If Expression is set, then the inverse flag inverses it (internally) without modifying it.
   */
  vtkSetMacro(Inverse, bool);
  vtkBooleanMacro(Inverse, bool);
  vtkGetMacro(Inverse, bool);
  ///@}

  ///@{
  /**
   * Set/Get names for inputs selections.
   *
   * NOTE: Input selection names are useful only if you have set the Expression, and
   * AppendByUnion is set to false.
   */
  void SetInputName(int index, const char* name);
  const char* GetInputName(int index) const;
  ///@}

  /**
   * Remove all assigned input selection names.
   */
  void RemoveAllInputNames();

  ///@{
  /**
   * Set/Get colors for inputs selections.
   *
   * Assign a color to a selection specified by its index.
   * If defined, the given color will be used to display this selection.
   */
  void SetInputColor(int index, double r, double g, double b);
  double* GetInputColor(int index) const;
  ///@}

  /**
   * Remove all assigned input selection colors.
   */
  void RemoveAllInputColors();

  ///@{
  /**
   * UserManagedInputs allows the user to set inputs by number instead of
   * using the AddInput/RemoveInput functions. Calls to
   * SetNumberOfInputs/SetInputByNumber should not be mixed with calls
   * to AddInput/RemoveInput.
   *
   * The default is false.
   */
  vtkSetMacro(UserManagedInputs, vtkTypeBool);
  vtkGetMacro(UserManagedInputs, vtkTypeBool);
  vtkBooleanMacro(UserManagedInputs, vtkTypeBool);
  ///@}

  /**
   * Add a dataset to the list of data to append. Should not be
   * used when UserManagedInputs is true, use SetInputByNumber instead.
   */
  void AddInputData(vtkSelection*);

  /**
   * Remove a dataset from the list of data to append. Should not be
   * used when UserManagedInputs is true, use SetInputByNumber (nullptr) instead.
   */
  void RemoveInputData(vtkSelection*);

  ///@{
  /**
   * Get any input of this filter.
   */
  vtkSelection* GetInput(int idx);
  vtkSelection* GetInput() { return this->GetInput(0); }
  ///@}

  /**
   * Directly set(allocate) number of inputs, should only be used
   * when UserManagedInputs is true.
   */
  void SetNumberOfInputs(int num);

  // Set Nth input, should only be used when UserManagedInputs is true.
  void SetInputConnectionByNumber(int num, vtkAlgorithmOutput* input);

  ///@{
  /**
   * When set to true, all the selections are combined together to form a single
   * vtkSelection output.
   * When set to false, the output is a composite selection with
   * input selections as the children of the composite selection. This allows
   * for selections with different content types and properties.
   *
   * The default is true.
   */
  vtkSetMacro(AppendByUnion, vtkTypeBool);
  vtkGetMacro(AppendByUnion, vtkTypeBool);
  vtkBooleanMacro(AppendByUnion, vtkTypeBool);
  ///@}

  /**
   * Return the specific name used for the selection color array.
   */
  static const char* GetColorArrayName() { return "vtkSelectionColor"; }

protected:
  vtkAppendSelection();
  ~vtkAppendSelection() override;

  // Usual data generation method
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int, vtkInformation*) override;

private:
  // hide the superclass' AddInput() from the user and the compiler
  void AddInputData(vtkDataObject*)
  {
    vtkErrorMacro(<< "AddInput() must be called with a vtkSelection not a vtkDataObject.");
  }

  /**
   * Add on SelectionData a new special array named `vtkSelectionColor` containing the given color.
   */
  void SetColorArray(vtkSelectionNode* node, double* color);

  vtkTypeBool UserManagedInputs;
  vtkTypeBool AppendByUnion;
  std::string Expression;
  bool Inverse;
  class vtkInternals;
  std::unique_ptr<vtkInternals> Internals;

  vtkAppendSelection(const vtkAppendSelection&) = delete;
  void operator=(const vtkAppendSelection&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
