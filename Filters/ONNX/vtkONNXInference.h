// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkONNXInference
 * @brief Infer an ONNX model
 *
 * vtkONNXInference is a filter that can read the weights of an ONNX model and perform
 * inference based on user provided tabular parameters (list of float32 basically). The prediction
 * is appended to the data arrays of the vtkDataObject input (@see SetArrayAssociation).
 *
 * One of the parameters can represent the time: the pipeline time step can be used instead
 * of the provided one.
 * To do that, set TimeStepIndex to the time index in the InputParameters list (@see
 * SetInputParameters, SetTimeStepIndex), and provide a TimeStepValues list (@see
 * SetTimeStepValues).
 *
 * In that case this filter generates its own time steps and is thus not meant to be used with
 * temporal data.
 */
#ifndef vtkONNXInference_h
#define vtkONNXInference_h

#include "vtkFiltersONNXModule.h" // For export macro
#include "vtkPassInputTypeAlgorithm.h"

#include "vtkDataObject.h" // For AttributeTypes

#include <memory> // For std::unique_ptr
#include <vector> // For std::vector

VTK_ABI_NAMESPACE_BEGIN
struct vtkONNXInferenceInternals;
namespace Ort
{
class AllocatorWithDefaultOptions;
class Value;
}

class VTKFILTERSONNX_EXPORT vtkONNXInference : public vtkPassInputTypeAlgorithm
{
public:
  vtkTypeMacro(vtkONNXInference, vtkPassInputTypeAlgorithm);
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
   * Time Steps.
   *
   * When the InputParamters list contains a time parameter, you can set TimeStepIndex to its index
   * in the list.
   * Then the time value will be set based on the pipeline time, overriding the value provided by
   * SetInputParameter. In that case, the time step values list should be provided to inform
   * downstream pipeline of available times.
   */
  ///@{
  /**
   * Set the list of time step values
   */
  void SetTimeStepValues(const std::vector<double>& times);

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

  ///@{
  /**
   * Set/Get the index of time value in the array of input parameters.
   * (default: -1, meaning no input parameter correspond to time)
   */
  vtkSetMacro(TimeStepIndex, int);
  vtkGetMacro(TimeStepIndex, int);
  ///@}

  /**
   * Input Parameters
   *
   * A list of parameters that will be forwarded to the inference model.
   * If TimeStepIndex >= 0, this index in the list of parameters will be
   * replaced by the current time value based on requested time and on TimeStepValues
   * @see SetTimeStepValues
   */
  /**
   * Set the input parameters that will be forwarded to the inference model.
   */
  void SetInputParameters(const std::vector<float>& params);

  /**
   * Set an input parameter at a given index.
   * You should call SetNumberOfInputParameters before.
   */
  void SetInputParameter(vtkIdType idx, float InputParameter);

  /**
   * Clear the input parameters vector. Useful when loading a new model to reset the
   * internal state.
   */
  void ClearInputParameters();

  ///@{
  /**
   * Set/Get the shape of the input. Also, the first element of the shape defines the shape of
   * InputParameters (default: {0}).
   * While passing a vector is more convenient, index/value API is required for ParaView support.
   */
  void SetInputShape(const std::vector<int64_t>& shape);
  void SetInputShape(vtkIdType idx, int shapeElement);
  void SetInputShape(vtkIdType nb);
  const std::vector<int64_t>& GetInputShape() const;
  ///@}

  /**
   * Set the number of input shape values. This basically resize the vector
   * of input shape.
   */
  void SetNumberOfInputShapeElements(vtkIdType nb);

  /**
   * Clear the input shape vector. Useful when loading a new model to reset the
   * internal state.
   */
  void ClearInputShape();

  ///@{
  /**
   * Set/Get whether the model input comes from prescribed parameters given through the
   * SetInputParameters API or if an existing cell/point data is used. (default: false)
   */
  vtkSetMacro(FieldArrayInput, bool);
  vtkGetMacro(FieldArrayInput, bool);
  vtkBooleanMacro(FieldArrayInput, bool);
  ///@}

  ///@{
  /**
   * Set/Get the name of the input array to be processed. (default: "")
   */
  vtkSetMacro(ProcessedFieldArrayName, const std::string&);
  vtkGetMacro(ProcessedFieldArrayName, const std::string&);
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
   * (default: vtkDataObject::CELL)
   */
  vtkSetMacro(ArrayAssociation, int);
  vtkGetMacro(ArrayAssociation, int);
  ///@}

protected:
  vtkONNXInference();
  ~vtkONNXInference() override = default;

  /**
   * This is required to inform the pipeline of the time steps.
   */
  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Execute the inference and add the resulting array on the given data object.
   * The input and output are expected to not be a CompositeDataSet subclass.
   */
  int ExecuteData(vtkDataObject* input, vtkDataObject* output, double timevalue);

private:
  vtkONNXInference(const vtkONNXInference&) = delete;
  void operator=(const vtkONNXInference&) = delete;

  /**
   * This instanciates the ONNX runtime session by reading the file specified
   * by this->ModelFile.
   */
  void InitializeSession();

  /**
   * Return true if the filter should generate time steps.
   * In that case, RequestInformation will fill the appropriate pipeline key
   * and the inference uses the pipeline time as one of its paremeter.
   */
  bool ShouldGenerateTimeSteps();

  /**
   * Create the input tensor in the format required by the ONNX Runtime API.
   * This is generated from the parameters given through SetInputParameters.
   */
  bool GenerateInputTensorFromParameters(
    std::vector<float>& parameters, Ort::Value& inputTensor, double timeValue);

  /**
   * Create the input tensor in the format required by the ONNX Runtime API.
   * This is generated from the field array specified by ProcessedFieldArrayName.
   */
  bool GenerateInputTensorFromFieldArray(
    Ort::Value& inputTensor, vtkDataSetAttributes* inAttributes);

  /**
   * Run the ONNX model with the provided input parameters. The ONNX session
   * must be initialized first.
   */
  std::vector<Ort::Value> RunModel(Ort::Value& inputTensor);

  // Input related parameters
  std::string ModelFile;
  std::vector<int64_t> InputShape = { 0 };
  std::vector<float> InputParameters;
  std::vector<double> TimeStepValues;
  int TimeStepIndex = -1;
  bool FieldArrayInput = false;
  std::string ProcessedFieldArrayName;

  // Output related parameters
  int OutputDimension = 1;

  int ArrayAssociation = vtkDataObject::CELL;

  bool Initialized = false;
  std::unique_ptr<vtkONNXInferenceInternals> Internals;
};
VTK_ABI_NAMESPACE_END

#endif // vtkONNXInference_h
