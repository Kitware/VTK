// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Kitware, Inc.
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkCGNSFileSeriesReader
 * @brief Adds support for reading temporal or partitioned CGNS files.
 *
 * vtkCGNSFileSeriesReader is a meta-reader that add support for reading
 * CGNS file series using vtkCGNSReader. We encounter two types of file series
 * with CGNS:
 * \li 1. temporal file series - where each file is simply a single timestep.
 * \li 2. partitioned file series - where each file corresponds to data dumped
 *        out from a rank but has all timesteps.
 *
 *  vtkCGNSFileSeriesReader determines the nature of the file series
 *  encountered and reads the files accordingly. For partitioned files, the
 *  files are distributed among data-processing ranks, while for temporal file
 *  series, blocks are distributed among data-processing ranks (using logic in
 *  vtkCGNSReader itself).
 *
 *  @sa vtkFileSeriesHelper
 */

#ifndef vtkCGNSFileSeriesReader_h
#define vtkCGNSFileSeriesReader_h

#include "vtkIOCGNSReaderModule.h" // for export macros
#include "vtkMultiBlockDataSetAlgorithm.h"
#include "vtkNew.h" // for vtkNew.

#include <string> // for std::string
#include <vector> // for std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkCGNSReader;
class vtkCGNSSubsetInclusionLattice;
class vtkFileSeriesHelper;
class vtkMultiProcessController;

class VTKIOCGNSREADER_EXPORT vtkCGNSFileSeriesReader : public vtkMultiBlockDataSetAlgorithm
{
public:
  static vtkCGNSFileSeriesReader* New();
  vtkTypeMacro(vtkCGNSFileSeriesReader, vtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the controller.
   */
  void SetController(vtkMultiProcessController* controller);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);
  ///@}

  ///@{
  /**
   * Get/Set the reader.
   */
  virtual void SetReader(vtkCGNSReader* reader);
  vtkGetObjectMacro(Reader, vtkCGNSReader);
  ///@}

  /**
   * Test a file for readability. Ensure that vtkCGNSFileSeriesReader::SetReader
   * is called before using this method.
   */
  int CanReadFile(VTK_FILEPATH const char* filename);

  ///@{
  /**
   * Add/remove files names in the file series.
   */
  void AddFileName(VTK_FILEPATH const char* fname);
  void RemoveAllFileNames();
  ///@}

  ///@{
  /**
   * If true, then treat file series like it does not contain any time step
   * values. False by default.
   */
  vtkGetMacro(IgnoreReaderTime, bool);
  vtkSetMacro(IgnoreReaderTime, bool);
  vtkBooleanMacro(IgnoreReaderTime, bool);
  ///@}

  /**
   * Returns the filename being used for current timesteps.
   * This is only reasonable for temporal file series. For a partitioned file
   * series, this will return the filename being used on the current rank.
   */
  VTK_FILEPATH const char* GetCurrentFileName() const;

  /**
   * Overridden to setup the `Reader` and then forward the pass to the reader.
   */
  vtkTypeBool ProcessRequest(
    vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

protected:
  vtkCGNSFileSeriesReader();
  ~vtkCGNSFileSeriesReader() override;

  /**
   * Handles the RequestData pass.
   */
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Update `this->ActiveFiles`, a collection of files to be read to satisfy the
   * current request.
   *
   * @returns false if the update failed for some reason, otherwise true.
   */
  bool UpdateActiveFileSet(vtkInformation* info);

  /**
   * Select the file from `this->ActiveFiles` at the given index and set that on
   * `this->Reader`.
   */
  void ChooseActiveFile(int index);

  vtkNew<vtkFileSeriesHelper> FileSeriesHelper;
  vtkCGNSReader* Reader;
  bool IgnoreReaderTime;

private:
  vtkCGNSFileSeriesReader(const vtkCGNSFileSeriesReader&) = delete;
  void operator=(const vtkCGNSFileSeriesReader&) = delete;
  void OnReaderModifiedEvent();

  vtkMultiProcessController* Controller;
  unsigned long ReaderObserverId;
  bool InProcessRequest;
  std::vector<std::string> ActiveFiles;
};

VTK_ABI_NAMESPACE_END
#endif
