// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkFidesReader
 * @brief   Read ADIOS2 streams using Fides data model
 *
 * vtkFidesReader is a data source that reads ADIOS2 files or data
 * streams (SST engine, inline engine etc.). The data model in these
 * data streams is defined by the Fides library:
 * (https://gitlab.kitware.com/vtk/fides/)
 * See the Fides documentation for the details of the schema used to
 * represent VTK/Viskores data models.
 * The reader can create partitioned dataset collection containing
 * native VTK dataset or  VTK Viskores datasets.
 * Time and time streaming is supported. Note that the interface for
 * time streaming is different. It requires calling PrepareNextStep()
 * and Update() for each new step.
 * Partitioned (in ADIOS2 terminology blocks) data is supported.
 *
 */

#ifndef vtkFidesReader_h
#define vtkFidesReader_h

#include "vtkAlgorithm.h"
#include "vtkDeprecation.h"   // For VTK_DEPRECATED_IN_9_6_0
#include "vtkIOFidesModule.h" // For export macro
#include <memory>             // for std::unique_ptr
#include <string>             // for std::string

#ifdef __VTK_WRAP__
#define vtkPyObjectFwd PyObject
#else
struct _object;
#define vtkPyObjectFwd _object
#endif

// Forward declare the Conduit C-API opaque pointer type exactly as
// Conduit does to avoid struct vs typedef collisions in C++.
struct conduit_node_impl;
typedef struct conduit_node_impl conduit_node;

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArraySelection;
class vtkMultiProcessController;

class VTKIOFIDES_EXPORT vtkFidesReader : public vtkAlgorithm
{
public:
  /**
   * When using streaming mode instead of random access,
   * PrepareNextStep receives a step status from Fides/ADIOS
   */
  enum StepStatus
  {
    OK,
    NotReady,
    EndOfStream
  };

  vtkTypeMacro(vtkFidesReader, vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Construct a new reader instance.
   */
  static vtkFidesReader* New();

  /**
   * Test whether or not a given file should even be attempted for use with this
   * reader.
   */
  int CanReadFile(VTK_FILEPATH const std::string& name);

  /**
   * Set the filename to be read
   */
  void SetFileName(VTK_FILEPATH const std::string& fname);

  ///@{
  /**
   * Given a json filename, parse and internally store a data
   * model. Has to be called before any data input can take place.
   * See the Fides documentation for the description of the schema.
   */
  void ParseDataModel(VTK_FILEPATH const std::string& fname);
  void ParseDataModel();
  ///@}

  /**
   * Set the path for a Fides data source. This can be a file, an
   * SST stream or an inline data source. The name of the data source
   * corresponds to what is in the data model.
   */
  void SetDataSourcePath(const std::string& name, VTK_FILEPATH const std::string& path);

  /**
   * Set the engine for a Fides data source. Defaults to BP engine.
   */
  void SetDataSourceEngine(const std::string& name, const std::string& engine);

  /**
   * Set the ADIOS2::IO object to be used for setting up the Inline engine reader.
   * This should not be used for any other engine type.
   * ioAddress is a string containing the address of the IO object, which Fides
   * will cast to a IO pointer.
   */
  void SetDataSourceIO(const std::string& name, const std::string& ioAddress);

  /**
   * Set the Conduit node to be associated with the named datasource.
   * This should only be used when the data model indicates the type is "conduit".
   * This is the C++ entry point, relying on the Conduit C-API.
   *
   * @warning **Memory Ownership:** This reader does not take ownership of the
   * memory backing the conduit node parameter. The caller is strictly responsible
   * for ensuring the data backing this node remains valid and unmodified in memory
   * until the VTK pipeline execution has finished.
   *
   * @param name The name of the Fides datasource.
   * @param node An opaque pointer to a C-API conduit_node.
   * @return true if the node was accepted and registered successfully.
   */
  bool SetDataSourceNode(const std::string& name, conduit_node* node);

  /**
   * Set the Conduit node to be associated with the named datasource.
   * This should only be used when the data model indicates the type is "conduit".
   * This function is wrapped and available from Python.
   *
   * @warning **Memory Ownership:** The reader increments the reference count
   * of the provided Python object, preventing Python from garbage collecting it
   * while this reader is alive. However, if the Conduit node is backed by
   * externally managed memory (e.g., a simulation's C++ arrays), the caller
   * must ensure that external memory is not freed while the reader holds this
   * reference.
   *
   * @param name The name of the Fides datasource.
   * @param conduitNode A Python conduit node object.
   * @return true if the node was accepted and registered successfully.
   */
  bool SetDataSourceNode(const std::string& name, vtkPyObjectFwd* conduitNode);

  /**
   * Remove a specific Conduit node associated with the named datasource.
   * * Calling this method releases the reader's hold on the provided Conduit node.
   * It unregisters the underlying memory from the Fides external data registry
   * and, if applicable, decrements the reference count of the associated Python
   * object.
   *
   * @param name The name of the Fides datasource to remove.
   */
  void RemoveDataSourceNode(const std::string& name);

  /**
   * Remove all associated Conduit nodes from the reader.
   * * This method clears all registered Conduit nodes, unregistering their memory
   * from Fides and decrementing any held Python reference counts.
   * * @note **State Reset:** This method acts as a reset for the reader's input
   * mode. If the reader was previously configured to read from in-memory Conduit
   * nodes, calling this method clears that state, allowing the reader to fall
   * back to standard file-reading mode (e.g., reading a standard ADIOS2 .bp file)
   * upon the next pipeline update.
   */
  void RemoveAllDataSourceNodes();

  /**
   * Implements various pipeline passes.
   */
  int ProcessRequest(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * This method has to be called before each step when streaming.
   * It will move to the next step and initialize any meta-data
   * necessary. The loading of the next step is done in the update
   * pipeline pass. Note that this internally calls Modified() on
   * the reader to force the next update to cause an execution.
   */
  void PrepareNextStep();

  /**
   * Get the StepStatus of the next step reported by Fides.
   * See enum vtkFidesReader::StepStatus for potential return values.
   * This is updated in PrepareNextStep() and set back to
   * NotReady after reading a step.
   */
  int GetNextStepStatus();

  /**
   * Gets the time (from the specified ADIOS variable) of the current step.
   * Should only be used in streaming mode.
   */
  double GetTimeOfCurrentStep();

  ///@{
  /**
   * Methods to determine whether to output a set of vtkmDataSets
   * or native VTK datasets. If the pipeline following the reader
   * is mainly VTK filters (as opposed to Viskores accelerated VTK
   * filters), set this to on. False by default.
   */
  VTK_DEPRECATED_IN_9_6_0("ConvertToVTK is deprecated since vtkmDataSet was deprecated.")
  virtual void SetConvertToVTK(bool) {};
  VTK_DEPRECATED_IN_9_6_0("ConvertToVTK is deprecated since vtkmDataSet was deprecated.")
  virtual bool GetConvertToVTK() { return true; };
  VTK_DEPRECATED_IN_9_6_0("ConvertToVTK is deprecated since vtkmDataSet was deprecated.")
  virtual void ConvertToVTKOn() {}
  VTK_DEPRECATED_IN_9_6_0("ConvertToVTK is deprecated since vtkmDataSet was deprecated.")
  virtual void ConvertToVTKOff() {}
  ///@}

  ///@{
  /**
   * Methods to determine whether streaming mode is used. False by default.
   */
  vtkBooleanMacro(StreamSteps, bool);
  vtkSetMacro(StreamSteps, bool);
  vtkGetMacro(StreamSteps, bool);
  ///@}

  ///@{
  /**
   * Determines whether to close gaps between blocks of structured grids with the use of shared
   * points.
   */
  vtkBooleanMacro(CreateSharedPoints, bool);
  vtkSetMacro(CreateSharedPoints, bool);
  vtkGetMacro(CreateSharedPoints, bool);
  ///@}

  /**
   * Object to perform point array selection before update.
   */
  vtkGetObjectMacro(PointDataArraySelection, vtkDataArraySelection);

  /**
   * Object to perform cell array selection before update.
   */
  vtkGetObjectMacro(CellDataArraySelection, vtkDataArraySelection);

  /**
   * Object to perform field array selection before update.
   */
  vtkGetObjectMacro(FieldDataArraySelection, vtkDataArraySelection);

  ///@{
  /**
   * Get the number of point or cell arrays available in the input.
   */
  int GetNumberOfPointArrays();
  int GetNumberOfCellArrays();
  int GetNumberOfFieldArrays();
  ///@}

  ///@{
  /**
   * Get the name of the point or cell array with the given index in
   * the input.
   */
  const char* GetPointArrayName(int index);
  const char* GetCellArrayName(int index);
  const char* GetFieldArrayName(int index);
  ///@}

  ///@{
  /**
   * Get/Set whether the point or cell array with the given name is to
   * be read.
   */
  int GetPointArrayStatus(const char* name);
  int GetCellArrayStatus(const char* name);
  int GetFieldArrayStatus(const char* name);
  void SetPointArrayStatus(const char* name, int status);
  void SetCellArrayStatus(const char* name, int status);
  void SetFieldArrayStatus(const char* name, int status);
  ///@}

  /**
   * Overridden to take into account mtimes for vtkDataArraySelection instances.
   */
  vtkMTimeType GetMTime() override;

  ///@{
  /**
   * Set/Get the multiprocess controller. If no controller is set,
   * the global controller will be used by default.
   */
  virtual void SetController(vtkMultiProcessController* controller);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

protected:
  vtkFidesReader();
  ~vtkFidesReader() override;

  struct vtkFidesReaderImpl;
  std::unique_ptr<vtkFidesReaderImpl> Impl;

  std::string FileName;
  bool StreamSteps;
  StepStatus NextStepStatus;
  bool CreateSharedPoints;

  virtual int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);
  virtual int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);
  virtual int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  int FillOutputPortInformation(int port, vtkInformation* info) override;

  vtkDataArraySelection* PointDataArraySelection;
  vtkDataArraySelection* CellDataArraySelection;
  vtkDataArraySelection* FieldDataArraySelection;

  vtkMultiProcessController* Controller;

  int ADIOSAttributeCheck(const std::string& name);

private:
  vtkFidesReader(const vtkFidesReader&) = delete;
  void operator=(const vtkFidesReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
