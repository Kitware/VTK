// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkBlockIdScalars
 * @brief   generates scalars from blocks.
 *
 * vtkBlockIdScalars is a filter that generates scalars using the block index
 * for each block. Note that, by default, all sub-blocks within a block get the same scalar.
 * The new scalars array is named \c BlockIdScalars.
 *
 * @sa vtkDataObjectTreeIterator
 */

#ifndef vtkBlockIdScalars_h
#define vtkBlockIdScalars_h

#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKFILTERSGENERAL_EXPORT vtkBlockIdScalars : public vtkPassInputTypeAlgorithm
{
public:
  static vtkBlockIdScalars* New();
  vtkTypeMacro(vtkBlockIdScalars, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get if the subtrees should be visited
   * Default is false
   */
  vtkSetMacro(TraverseSubTree, bool);
  vtkGetMacro(TraverseSubTree, bool);
  vtkBooleanMacro(TraverseSubTree, bool);
  ///@}

  ///@{
  /**
   * Set/Get if only leaves should be visited
   * Default is false
   */
  vtkSetMacro(VisitOnlyLeaves, bool);
  vtkGetMacro(VisitOnlyLeaves, bool);
  vtkBooleanMacro(VisitOnlyLeaves, bool);
  ///@}

protected:
  vtkBlockIdScalars();
  ~vtkBlockIdScalars() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int FillInputPortInformation(int port, vtkInformation* info) override;

private:
  vtkBlockIdScalars(const vtkBlockIdScalars&) = delete;
  void operator=(const vtkBlockIdScalars&) = delete;

  bool TraverseSubTree = false;
  bool VisitOnlyLeaves = false;
};

VTK_ABI_NAMESPACE_END
#endif
