// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkGroupTimeStepsFilter
 * @brief converts a temporal dataset into non-temporal dataset.
 *
 * vtkGroupTimeStepsFilter is intended to convert a temporal input with
 * multiple timesteps into a single dataset will all timesteps available.
 * The filter requests all timesteps from the upstream one after another and
 * then packages it into a single output vtkPartitionedDataSetCollection
 * or vtkMultiBlockDataSet based on the input data type.
 * In most cases vtkPartitionedDataSetCollection is produced. Only when
 * the input cannot be stored in a vtkPartitionedDataSetCollection,
 * vtkMultiBlockDataSet is created instead.
 *
 * @section vtkGroupTimeStepsFilter-Limitations Limitations
 *
 * The filter may not work correctly if the input dataset type changes over
 * time.
 */

#ifndef vtkGroupTimeStepsFilter_h
#define vtkGroupTimeStepsFilter_h

#include "vtkDataObjectAlgorithm.h"
#include "vtkFiltersGeneralModule.h" // For export macro
#include "vtkSmartPointer.h"         // for vtkSmartPointer

#include <vector> // for std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkPartitionedDataSet;
class vtkPartitionedDataSetCollection;
class vtkMultiBlockDataSet;
class vtkCompositeDataSet;

class VTKFILTERSGENERAL_EXPORT vtkGroupTimeStepsFilter : public vtkDataObjectAlgorithm
{
public:
  static vtkGroupTimeStepsFilter* New();
  vtkTypeMacro(vtkGroupTimeStepsFilter, vtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkGroupTimeStepsFilter();
  ~vtkGroupTimeStepsFilter() override;

  int RequestDataObject(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkGroupTimeStepsFilter(const vtkGroupTimeStepsFilter&) = delete;
  void operator=(const vtkGroupTimeStepsFilter&) = delete;

  bool AddTimeStep(double time, int timeStep, vtkDataObject*);
  bool AddTimeStep(double time, int timeStep, vtkPartitionedDataSet*);
  bool AddTimeStep(double time, int timeStep, vtkPartitionedDataSetCollection*);
  bool AddTimeStep(double time, int timeStep, vtkMultiBlockDataSet*);
  bool AddTimeStep(double time, int timeStep, vtkCompositeDataSet*);

  size_t UpdateTimeIndex;
  std::vector<double> TimeSteps;
  vtkSmartPointer<vtkDataObject> AccumulatedData;
};

VTK_ABI_NAMESPACE_END
#endif
