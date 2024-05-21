// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
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

#include "vtkFiltersGeneralModule.h" // For export macro
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

class VTKFILTERSGENERAL_EXPORT vtkTemporalSmoothing : public vtkPassInputTypeAlgorithm
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
    Specify the size of the sliding temporal window. The average will
    be computed using this value on each side of a considered time step.
  */
  vtkGetMacro(TemporalWindowHalfWidth, vtkTypeUInt32);
  vtkSetMacro(TemporalWindowHalfWidth, vtkTypeUInt32);
  ///@}

protected:
  vtkTemporalSmoothing();
  ~vtkTemporalSmoothing() override = default;

  vtkTypeUInt32 TemporalWindowHalfWidth = 20;
  std::vector<double> SmoothingWeights;

  ///@{
  /**
   * The necessary parts of the standard pipeline update mechanism
   */
  int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestUpdateExtent(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;
  ///@}

  virtual void InitializeStatistics(
    vtkDataObject* input, vtkDataObject* output, vtkDataObject* cache);
  virtual void InitializeStatistics(vtkDataSet* input, vtkDataSet* output, vtkDataSet* cache);
  virtual void InitializeStatistics(vtkGraph* input, vtkGraph* output, vtkGraph* cache);
  virtual void InitializeStatistics(
    vtkCompositeDataSet* input, vtkCompositeDataSet* output, vtkCompositeDataSet* cache);
  virtual void InitializeArrays(vtkFieldData* inFd, vtkFieldData* outFd);
  virtual void InitializeArray(vtkDataArray* array, vtkFieldData* outFd);

  virtual void AccumulateStatistics(vtkDataObject* input, vtkDataObject* output);
  virtual void AccumulateStatistics(vtkDataSet* input, vtkDataSet* output);
  virtual void AccumulateStatistics(vtkGraph* input, vtkGraph* output);
  virtual void AccumulateStatistics(vtkCompositeDataSet* input, vtkCompositeDataSet* output);
  virtual void AccumulateArrays(vtkFieldData* inFd, vtkFieldData* outFd);

  virtual void PostExecute(vtkDataObject* input, vtkDataObject* output);
  virtual void PostExecute(vtkDataSet* input, vtkDataSet* output);
  virtual void PostExecute(vtkGraph* input, vtkGraph* output);
  virtual void PostExecute(vtkCompositeDataSet* input, vtkCompositeDataSet* output);
  virtual void FinishArrays(vtkFieldData* inFd, vtkFieldData* outFd);

private:
  vtkTemporalSmoothing(const vtkTemporalSmoothing&) = delete;
  void operator=(const vtkTemporalSmoothing&) = delete;

  std::shared_ptr<vtkTemporalSmoothingInternals> Internals;
};

VTK_ABI_NAMESPACE_END
#endif
