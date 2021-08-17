/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFidesReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkFidesReader
 * @brief   Read ADIOS2 streams using Fides data model
 *
 * vtkFidesReader is a data source that reads ADIOS2 files or data
 * streams (SST engine, inline engine etc.). The data model in these
 * data streams is defined by the Fides library:
 * (https://gitlab.kitware.com/vtk/fides/)
 * See the Fides documentation for the details of the schema used to
 * represent VTK/VTK-m data models.
 * The reader can create partitioned datasets containing
 * native VTK dataset or  VTK VTK-m datasets.
 * Time and time streaming is supported. Note that the interface for
 * time streaming is different. It requires calling PrepareNextStep()
 * and Update() for each new step.
 * Partitioned (in ADIOS2 terminology blocks) data is supported.
 *
 */

#ifndef vtkFidesReader_h
#define vtkFidesReader_h

#include "vtkAlgorithm.h"
#include "vtkIOFidesModule.h" // For export macro
#include <memory>             // for std::unique_ptr
#include <string>             // for std::string

class vtkDataArraySelection;
class vtkInformationIntegerKey;

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

  ///@{
  /**
   * Methods to determine whether to output a set of vtkmDataSets
   * or native VTK datasets. If the pipeline following the reader
   * is mainly VTK filters (as opposed to VTK-m accelerated VTK
   * filters), set this to on. False by default.
   */
  vtkBooleanMacro(ConvertToVTK, bool);
  vtkSetMacro(ConvertToVTK, bool);
  vtkGetMacro(ConvertToVTK, bool);
  ///@}

  /**
   * Object to perform point array selection before update.
   */
  vtkGetObjectMacro(PointDataArraySelection, vtkDataArraySelection);

  /**
   * Object to perform cell array selection before update.
   */
  vtkGetObjectMacro(CellDataArraySelection, vtkDataArraySelection);

protected:
  vtkFidesReader();
  ~vtkFidesReader() override;

  struct vtkFidesReaderImpl;
  std::unique_ptr<vtkFidesReaderImpl> Impl;

  std::string FileName;
  bool ConvertToVTK;
  bool StreamSteps;
  StepStatus NextStepStatus;

  virtual int RequestDataObject(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);
  virtual int RequestInformation(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);
  virtual int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector);

  int FillOutputPortInformation(int port, vtkInformation* info) override;

  vtkDataArraySelection* PointDataArraySelection;
  vtkDataArraySelection* CellDataArraySelection;

  static vtkInformationIntegerKey* NUMBER_OF_BLOCKS();

  int ADIOSAttributeCheck(const std::string& name);

private:
  vtkFidesReader(const vtkFidesReader&) = delete;
  void operator=(const vtkFidesReader&) = delete;
};

#endif
