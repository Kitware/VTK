// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkHyperTreeGridGradient
 * @brief   Compute the gradient of a scalar field
 * on a Hyper Tree Grid.
 *
 * This filter compute the gradient of a given cell scalars array on a
 * Hyper Tree Grid. This result in a new array attached to the original input.
 * This filter does not support masks.
 * In practice the mask is ignored during the processing of this filters and re-attached to the
 * output, leading to masked cell being taken into account for the gradient computation of visible
 * cells. This leads to the gradient being influenced by masked cells. This should only impact cells
 * on the boundary, where gradient is already ill-defined.
 *
 * @sa
 * vtkHyperTreeGrid vtkHyperTreeGridAlgorithm vtkGradientFilter
 *
 * @par Thanks:
 * This class was created by Charles Gueunet, 2022
 * This work was supported by Commissariat a l'Energie Atomique
 * CEA, DAM, DIF, F-91297 Arpajon, France.
 */

#ifndef vtkHyperTreeGridGradient_h
#define vtkHyperTreeGridGradient_h

#include "vtkFiltersHyperTreeModule.h" // For export macro

#include "vtkHyperTreeGridAlgorithm.h"
#include "vtkNew.h"          // for internal fields
#include "vtkSmartPointer.h" // for internal fields

#include <cstring> // for strdup, to initialize char*

VTK_ABI_NAMESPACE_BEGIN

class vtkHyperTreeGridNonOrientedCursor;
class vtkBitArray;
class vtkDoubleArray;
class vtkUnsignedCharArray;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridGradient : public vtkHyperTreeGridAlgorithm
{
public:
  enum ComputeMode
  {
    UNLIMITED = 0,
    UNSTRUCTURED
  };

  static vtkHyperTreeGridGradient* New();
  vtkTypeMacro(vtkHyperTreeGridGradient, vtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Enable / disable gradient computation.
   * default is On.
   */
  vtkSetMacro(ComputeGradient, bool);
  vtkGetMacro(ComputeGradient, bool);
  vtkBooleanMacro(ComputeGradient, bool);
  ///@}

  ///@{
  /**
   * Set/Get the name of gradient vector array.
   */
  vtkSetStringMacro(GradientArrayName);
  vtkGetStringMacro(GradientArrayName);
  ///@}

  ///@{
  /**
   * Set/Get the gradient computation method to use:
   * * Unlimited: virtually reffine neighbors
   * * Unstructured: compute gradient like in UG
   * Dfault is UNLIMITED
   */
  vtkSetClampMacro(Mode, int, UNLIMITED, UNSTRUCTURED);
  vtkGetMacro(Mode, int);
  ///@}

  ///@{
  /**
   * Do we apply ratio in unlimited mode for the gradient computation ?
   * No effect in Unstructured mode
   * Default is false (intensive computation)
   */
  vtkSetMacro(ExtensiveComputation, bool);
  vtkGetMacro(ExtensiveComputation, bool);
  vtkBooleanMacro(ExtensiveComputation, bool);
  ///@}

  ///@{
  /**
   * Enable / disable divergence computation.
   * default is Off.
   */
  vtkSetMacro(ComputeDivergence, bool);
  vtkGetMacro(ComputeDivergence, bool);
  vtkBooleanMacro(ComputeDivergence, bool);
  ///@}

  ///@{
  /**
   * Set/Get the name of divergence vector array.
   */
  vtkSetStringMacro(DivergenceArrayName);
  vtkGetStringMacro(DivergenceArrayName);
  ///@}

  ///@{
  /**
   * Enable / disable vorticity computation.
   * default is Off.
   */
  vtkSetMacro(ComputeVorticity, bool);
  vtkGetMacro(ComputeVorticity, bool);
  vtkBooleanMacro(ComputeVorticity, bool);
  ///@}

  ///@{
  /**
   * Set/Get the name of vorticity array.
   */
  vtkSetStringMacro(VorticityArrayName);
  vtkGetStringMacro(VorticityArrayName);
  ///@}

  ///@{
  /**
   * Enable / disable q-criterion computation.
   * default is Off.
   */
  vtkSetMacro(ComputeQCriterion, bool);
  vtkGetMacro(ComputeQCriterion, bool);
  vtkBooleanMacro(ComputeQCriterion, bool);
  ///@}

  ///@{
  /**
   * Set/Get the name of q-criterion array.
   */
  vtkSetStringMacro(QCriterionArrayName);
  vtkGetStringMacro(QCriterionArrayName);
  ///@}

protected:
  vtkHyperTreeGridGradient();
  ~vtkHyperTreeGridGradient() override;

  /**
   * Main routine to generate gradient of hyper tree grid.
   */
  int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) override;

  /**
   * Recursively descend into tree down to leaves to compute gradient
   * Uses a heavy supercursor
   */
  template <class Cursor, class GradWorker>
  void RecursivelyProcessGradientTree(Cursor*, GradWorker&);

  /**
   * Compute Vorticity, Divergence and QCriterion upon request,
   * from the Gradient cell array
   */
  template <class FieldsWorker>
  void ProcessFields(FieldsWorker&);

  // Fields
  // ------

  // Gradient
  bool ComputeGradient = true;
  vtkNew<vtkDoubleArray> OutGradArray;
  char* GradientArrayName = strdup("Gradient");
  int Mode = ComputeMode::UNLIMITED;
  bool ExtensiveComputation = false;

  // Divergence
  bool ComputeDivergence = false;
  vtkNew<vtkDoubleArray> OutDivArray;
  char* DivergenceArrayName = strdup("Divergence");

  // Vorticity
  bool ComputeVorticity = false;
  vtkNew<vtkDoubleArray> OutVortArray;
  char* VorticityArrayName = strdup("Vorticity");

  // QCriterion
  bool ComputeQCriterion = false;
  vtkNew<vtkDoubleArray> OutQCritArray;
  char* QCriterionArrayName = strdup("QCriterion");

  /**
   * Keep track of selected input scalars / vectors
   */
  vtkSmartPointer<vtkDataArray> InArray;

  // shortcut to HTG fields
  vtkBitArray* InMask = nullptr;
  vtkUnsignedCharArray* InGhostArray = nullptr;

private:
  vtkHyperTreeGridGradient(const vtkHyperTreeGridGradient&) = delete;
  void operator=(const vtkHyperTreeGridGradient&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkHyperTreeGridGradient_h
