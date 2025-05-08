// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class   vtkFidesWriter
 * @brief   Write ADIOS2 streams using Fides data model
 *
 * vtkFidesWriter uses ADIOS2 to write files using the Fides schema. Fides requires
 * data in Viskores format, so this vtkFidesWriter first converts VTK datasets to Viskores datasets.
 * This also writes out a Fides schema so it can be read back in using vtkFidesReader.
 * The schema is written as an attribute in the ADIOS2 file.
 *
 * Note: Currently only supports BP file engine.
 *
 * Typical usage is as follows:
 *
 * @code{.cpp}
 * vtkNew<vtkFidesWriter> writer;
 * writer->SetInputData(...)
 * writer->SetFileName(...);
 * writer->Write();
 * @endcode
 *
 * @section FidesWriterSelectArraysToWrite Selecting arrays to write
 *
 * By default, all arrays are enabled. To write specific arrays, set ChooseFieldsToWrite to true,
 * via `vtkFidesWriter::SetChooseFieldsToWrite(true)`. Then use the `vtkDataArraySelection` instance
 * returned using `vtkFidesWriter::GetArraySelection` (or one of the conviences variants) to enable
 * specific arrays.
 *
 * Typical usage is as follows:
 *
 * @code{.cpp}
 * vtkNew<vtkFidesWriter> writer;
 * writer->SetInputData(...)
 * writer->SetFileName(...);
 * writer->SetChooseFieldsToWrite(true);
 * writer->GetPointDataArraySelection()->EnableArray(fieldName);
 * writer->Write();
 * @endcode
 *
 * @section FidesWriterSelectTimeSteps Selecting TimeSteps
 *
 * `vtkFidesWriter::SetTimeStepRange(...)` and `vtkFidesWriter::SetTimeStepStride(...)`
 * can be used to write a subset of time steps using a range and a stride.
 *
 * @sa vtkFidesReader
 */

#ifndef vtkFidesWriter_h
#define vtkFidesWriter_h

#include "vtkIOFidesModule.h" // For export macro
#include "vtkNew.h"           // For vtkNew
#include "vtkWriter.h"

#include <memory> // for std::unique_ptr

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArraySelection;
class vtkInformationIntegerKey;
class vtkMultiProcessController;

class VTKIOFIDES_EXPORT vtkFidesWriter : public vtkWriter
{
public:
  enum EngineTypes
  {
    BPFile
  };

  static vtkFidesWriter* New();
  vtkTypeMacro(vtkFidesWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the filename to be written.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * Choose which fields to write. If this is true, then only the
   * arrays selected will be written. If this is false, then all arrays will be
   * written.
   *
   * The default is false.
   */
  vtkSetMacro(ChooseFieldsToWrite, bool);
  vtkGetMacro(ChooseFieldsToWrite, bool);
  vtkBooleanMacro(ChooseFieldsToWrite, bool);
  ///@}

  ///@{
  /**
   * Returns the array selection object for point, cell, or field data.
   */
  vtkDataArraySelection* GetArraySelection(int association);
  vtkDataArraySelection* GetPointDataArraySelection();
  vtkDataArraySelection* GetCellDataArraySelection();
  vtkDataArraySelection* GetFieldDataArraySelection();
  ///@}

  ///@{
  /**
   * `TimeStepRange` and `TimeStepStride` can be used to limit which timesteps will be written.
   *
   * If the range is invalid, i.e. `TimeStepRange[0] >= TimeStepRange[1]`, it's assumed
   * that no TimeStepRange overrides have been specified and both TimeStepRange and
   * TimeStepStride will be ignored. When valid, only the chosen subset of files
   * will be processed.
   */
  vtkSetVector2Macro(TimeStepRange, int);
  vtkGetVector2Macro(TimeStepRange, int);
  vtkSetClampMacro(TimeStepStride, int, 1, VTK_INT_MAX);
  vtkGetMacro(TimeStepStride, int);
  ///@}

  ///@{
  /**
   * Set/Get the ADIOS engine to use (currently BPFile only!)
   */
  vtkSetMacro(Engine, int);
  vtkGetMacro(Engine, int);
  ///@}

  ///@{
  /**
   * Get/Set the controller to use when working in parallel. Initialized to
   * `vtkMultiProcessController::GetGlobalController` in the constructor.
   *
   */
  void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

protected:
  vtkFidesWriter();
  ~vtkFidesWriter() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  int ProcessRequest(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestUpdateExtent(vtkInformation*, vtkInformationVector**, vtkInformationVector*);
  int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  void WriteData() override;

private:
  vtkFidesWriter(const vtkFidesWriter&) = delete;
  void operator=(const vtkFidesWriter&) = delete;

  struct FidesWriterImpl;
  std::unique_ptr<FidesWriterImpl> Impl;

  vtkMultiProcessController* Controller;
  char* FileName;
  bool ChooseFieldsToWrite;
  int TimeStepRange[2];
  int TimeStepStride;
  int Engine;

  vtkNew<vtkDataArraySelection> ArraySelection[3];
};

VTK_ABI_NAMESPACE_END
#endif
