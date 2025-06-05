// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkONNXInference
 * @brief Infer an ONNX model
 *
 * vtkONNXInference is a filter that can read the weights of an ONNX model and perform
 * inference based on user provided tabular parameters (list of float32 basically). The prediction
 * is appended to the data arrays of the vtkUnstructuredGrid input.
 *
 * Moreover, the filter handles time steps. Basically, this represents the inference of the model
 * with a varying parameters which happens to represent time. Note that this filter generates its
 * own time steps and is thus not meant to be used with temporal data.
 */
#ifndef vtkONNXInference_h
#define vtkONNXInference_h

#include "vtkFiltersONNXModule.h" // For export macro
#include "vtkUnstructuredGridAlgorithm.h"

#include <memory> // For std::unique_ptr
#include <vector> // For std::vector

VTK_ABI_NAMESPACE_BEGIN
struct vtkONNXInferenceInternals;
namespace Ort
{
class AllocatorWithDefaultOptions;
class Value;
}

class VTKFILTERSONNX_EXPORT vtkONNXInference : public vtkUnstructuredGridAlgorithm
{
public:
  vtkTypeMacro(vtkONNXInference, vtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkONNXInference* New();

  ///@{
  /**
   * Get/Set the path to the ONNX model and load it. (default: "")
   */
  void SetModelFile(const std::string& file);
  vtkGetMacro(ModelFile, std::string);
  ///@}

  /**
   * Set a time value at a given index.
   */
  void SetTimeStepValue(vtkIdType idx, double timeStepValue);

  /**
   * Set the number of time step values. This basically allocates the vector
   * of time values.
   */
  void SetNumberOfTimeStepValues(vtkIdType nb);

  /**
   * Clear the time step values vector. Useful when loading a new model to reset the
   * internal state.
   */
  void ClearTimeStepValues();

  /**
   * Set an input parameter at a given index.
   */
  void SetInputParameter(vtkIdType idx, float InputParameter);

  /**
   * Set the number of input parameters. This basically allocates the vector
   * of parameters and sets the input dimension.
   */
  void SetNumberOfInputParameters(vtkIdType nb);

  /**
   * Set/Get the number of input elements. (default: 0)
   */
  vtkGetMacro(InputSize, int);

  /**
   * Clear the input parameters vector. Useful when loading a new model to reset the
   * internal state.
   */
  void ClearInputParameters();

  ///@{
  /**
   * Set/Get the index of time value in the array of input parameters.
   * (default: -1, meaning no input parameter correspond to time)
   */
  vtkSetMacro(TimeStepIndex, int);
  vtkGetMacro(TimeStepIndex, int);
  ///@}

  ///@{
  /**
   * Set/Get the dimension of the output. (default: 1)
   */
  vtkSetMacro(OutputDimension, int);
  vtkGetMacro(OutputDimension, int);
  ///@}

  ///@{
  /**
   * Set/Get whether the output values should be attached to the cells or the points.
   * (default: true, meaning data are attached to the cells)
   */
  vtkSetMacro(OnCellData, bool);
  vtkGetMacro(OnCellData, bool);
  ///@}

protected:
  vtkONNXInference();
  ~vtkONNXInference() override = default;

  /**
   * This is required to inform the pipeline of the time steps.
   */
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  vtkONNXInference(const vtkONNXInference&) = delete;
  void operator=(const vtkONNXInference&) = delete;

  /**
   * This instanciates the ONNX runtime session by reading the file specified
   * by this->ModelFile.
   */
  void InitializeSession();

  /**
   * Run the ONNX model with the provided input parameters. The ONNX session
   * must be initialized first.
   */
  std::vector<Ort::Value> RunModel(Ort::Value& inputTensor);

  // Input related parameters
  std::string ModelFile;
  int64_t InputSize = 0;
  std::vector<float> InputParameters;
  std::vector<double> TimeStepValues;
  int TimeStepIndex = -1;

  // Output related parameters
  int OutputDimension = 1;
  bool OnCellData = true;

  bool Initialized = false;
  std::unique_ptr<vtkONNXInferenceInternals> Internals;
};
VTK_ABI_NAMESPACE_END

#endif // vtkONNXInference_h
