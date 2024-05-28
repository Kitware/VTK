// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkTemporalSmoothing
 * @brief   Smooth point or cell data over a sliding time window.
 *
 * vtkTemporalSmoothing computes an average of every point and cell data over
 * a sliding temporal window.
 */

#ifndef vtkTemporalSmoothing_h
#define vtkTemporalSmoothing_h

#include "vtkFiltersTemporalModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

#include <memory> // For smart pointers

VTK_ABI_NAMESPACE_BEGIN
struct vtkTemporalSmoothingInternals;
class vtkCompositeDataSet;
class vtkFieldData;
class vtkDataSet;
class vtkInformationVector;
class vtkInformation;
class vtkGraph;

class VTKFILTERSTEMPORAL_EXPORT vtkTemporalSmoothing : public vtkPassInputTypeAlgorithm
{
public:
  ///@{
  /**
   * Standard Type-Macro
   */
  static vtkTemporalSmoothing* New();
  vtkTypeMacro(vtkTemporalSmoothing, vtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  ///@{
  /**
   * Set/Get the size of the sliding temporal window. The average will
   * be computed using this value on each side of a considered time step.
   * Defaults to 10.
   */
  vtkGetMacro(TemporalWindowHalfWidth, int);
  vtkSetMacro(TemporalWindowHalfWidth, int);
  ///@}

protected:
  vtkTemporalSmoothing();
  ~vtkTemporalSmoothing() override = default;

  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

private:
  vtkTemporalSmoothing(const vtkTemporalSmoothing&) = delete;
  void operator=(const vtkTemporalSmoothing&) = delete;

  void Initialize(vtkDataObject* input, vtkDataObject* output, vtkDataObject* cache);
  void Initialize(vtkDataSet* input, vtkDataSet* output, vtkDataSet* cache);
  void Initialize(vtkGraph* input, vtkGraph* output, vtkGraph* cache);
  void Initialize(
    vtkCompositeDataSet* input, vtkCompositeDataSet* output, vtkCompositeDataSet* cache);
  void InitializeArrays(vtkFieldData* inFd, vtkFieldData* outFd);
  void InitializeArray(vtkDataArray* array, vtkFieldData* outFd);

  void AccumulateSum(vtkDataObject* input, vtkDataObject* output);
  void AccumulateSum(vtkDataSet* input, vtkDataSet* output);
  void AccumulateSum(vtkGraph* input, vtkGraph* output);
  void AccumulateSum(vtkCompositeDataSet* input, vtkCompositeDataSet* output);
  void AccumulateArrays(vtkFieldData* inFd, vtkFieldData* outFd);

  void PostExecute(vtkDataObject* input, vtkDataObject* output);
  void PostExecute(vtkDataSet* input, vtkDataSet* output);
  void PostExecute(vtkGraph* input, vtkGraph* output);
  void PostExecute(vtkCompositeDataSet* input, vtkCompositeDataSet* output);
  void FinishArrays(vtkFieldData* inFd, vtkFieldData* outFd);

  int TemporalWindowHalfWidth = 10;
  std::shared_ptr<vtkTemporalSmoothingInternals> Internals;
};

VTK_ABI_NAMESPACE_END
#endif
