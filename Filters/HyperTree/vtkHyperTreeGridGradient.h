/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkHyperTreeGridGradient.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkHyperTreeGridGradient
 * @brief   Compute the gradient of a scalar field
 * on a Hyper Tree Grid.
 *
 * This filter compute the gradient of a given cell scalars array on a
 * Hyper Tree Grid. This result in a new array attached to the original input.
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

#include <string> // for internal fields

class vtkHyperTreeGridNonOrientedGeometryCursor;
class vtkHyperTreeGridNonOrientedMooreSuperCursor;
class vtkBitArray;
class vtkDoubleArray;
class vtkUnsignedCharArray;

class VTKFILTERSHYPERTREE_EXPORT vtkHyperTreeGridGradient : public vtkHyperTreeGridAlgorithm
{
public:
  static vtkHyperTreeGridGradient* New();
  vtkTypeMacro(vtkHyperTreeGridGradient, vtkHyperTreeGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the name of computed vector array.
   */
  vtkSetMacro(ResultArrayName, std::string);
  vtkGetMacro(ResultArrayName, std::string);
  ///@}

protected:
  vtkHyperTreeGridGradient();
  ~vtkHyperTreeGridGradient() override;

  /**
   * Main routine to generate gradient of hyper tree grid.
   */
  int ProcessTrees(vtkHyperTreeGrid*, vtkDataObject*) override;

  /**
   * Recursively descend into tree down to leaves
   */
  template <class Worker>
  void RecursivelyProcessTree(vtkHyperTreeGridNonOrientedMooreSuperCursor*, Worker&);

  // Fields

  std::string ResultArrayName = "Gradient";

  /**
   * Keep track of selected input scalars
   */
  vtkSmartPointer<vtkDataArray> InScalars;

  /**
   * Computed gradient
   */
  vtkNew<vtkDoubleArray> OutGradient;

  // shortcut to HTG fields
  vtkBitArray* InMask = nullptr;
  vtkUnsignedCharArray* InGhostArray = nullptr;

private:
  vtkHyperTreeGridGradient(const vtkHyperTreeGridGradient&) = delete;
  void operator=(const vtkHyperTreeGridGradient&) = delete;
};

#endif // vtkHyperTreeGridGradient_h
