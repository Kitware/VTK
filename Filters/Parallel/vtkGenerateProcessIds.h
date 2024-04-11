// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkGenerateProcessIds
 * @brief   Sets ProcessIds attribute for PointData and/or CellData.
 *
 *
 * vtkGenerateProcessIds is meant to fill in the ProcessIds attribute array,
 * to know which processor owns which cells and points. It can generate it
 * for both PointData and CellData. The ProcessIds array's name will be
 * "PointProcessIds" for PointData, and "CellProcessIds" for CellData.
 */

#ifndef vtkGenerateProcessIds_h
#define vtkGenerateProcessIds_h

#include "vtkDataSetAlgorithm.h"
#include "vtkFiltersParallelModule.h" // For export macro
#include "vtkSmartPointer.h"          // For vtkSmartPointer
#include "vtkWeakPointer.h"           // For vtkWeakPointer

VTK_ABI_NAMESPACE_BEGIN
class vtkMultiProcessController;
template <typename T>
class vtkImplicitArray;
template <typename T>
struct vtkConstantImplicitBackend;
template <typename Type>
using vtkConstantArray = vtkImplicitArray<vtkConstantImplicitBackend<Type>>;

class VTKFILTERSPARALLEL_EXPORT vtkGenerateProcessIds : public vtkDataSetAlgorithm
{
public:
  static vtkGenerateProcessIds* New();

  vtkTypeMacro(vtkGenerateProcessIds, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get whether to generate process ids for PointData.
   * Default is true.
   */
  vtkSetMacro(GeneratePointData, bool);
  vtkGetMacro(GeneratePointData, bool);
  vtkBooleanMacro(GeneratePointData, bool);
  ///@}

  ///@{
  /**
   * Set/Get whether to generate process ids for CellData.
   * Default is false.
   */
  vtkSetMacro(GenerateCellData, bool);
  vtkGetMacro(GenerateCellData, bool);
  vtkBooleanMacro(GenerateCellData, bool);
  ///@}

  ///@{
  /**
   * By default this filter uses the global controller,
   * but this method can be used to set another instead.
   */
  virtual void SetController(vtkMultiProcessController*);
  vtkMultiProcessController* GetController();
  ///@}

protected:
  vtkGenerateProcessIds();
  ~vtkGenerateProcessIds() override;

  // Append the pieces.
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkGenerateProcessIds(const vtkGenerateProcessIds&) = delete;
  void operator=(const vtkGenerateProcessIds&) = delete;

  /**
   * Generate a vtkIdTypeArray filled with piece id.
   */
  vtkSmartPointer<vtkConstantArray<vtkIdType>> GenerateProcessIds(
    vtkIdType piece, vtkIdType numberOfTuples);

  bool GeneratePointData = true;
  bool GenerateCellData = false;
  vtkWeakPointer<vtkMultiProcessController> Controller;
};

VTK_ABI_NAMESPACE_END
#endif
