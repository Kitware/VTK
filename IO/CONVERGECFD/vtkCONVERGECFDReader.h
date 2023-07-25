// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class    vtkCONVERGECFDReader
 * @brief    Reader for CONVERGE CFD post files.
 *
 * This class reads CONVERGE CFD post files containing meshes, surfaces,
 * and parcels. Each stream in a file is read as a top-level
 * block and meshes, surfaces, and parcels are datasets under
 * each stream block.
 *
 * Cell data arrays associated with mesh cells can be individually
 * selected for reading using the CellArrayStatus API.
 *
 * Point data arrays associated with parcels can be individually selected
 * for reading using the ParcelArrayStatus API.
 *
 * Time series are supported. The reader assumes a time series is defined
 * in a sequence of files that follow the naming convention
 *
 * \code
 * <prefix><zero-padded index>[_][<time>].h5
 * \endcode
 *
 * where the prefix is determined from the FileName property passed to
 * the reader. The underscore and time elements are optional. The time
 * value associated with each file is read from metadata in the file.
 *
 * Parallel data loading is not supported.
 */

#ifndef vtkCONVERGECFDReader_h
#define vtkCONVERGECFDReader_h

#include "vtkIOCONVERGECFDModule.h" // For export macro

#include "vtkNew.h" // for vtkNew
#include "vtkPartitionedDataSetCollectionAlgorithm.h"

#include <string> // for std::string
#include <vector> // for std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArraySelection;

class VTKIOCONVERGECFD_EXPORT vtkCONVERGECFDReader : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
  static vtkCONVERGECFDReader* New();
  vtkTypeMacro(vtkCONVERGECFDReader, vtkPartitionedDataSetCollectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Access the cell data array selection to specify which cell data arrays
   * should be read. Only the specified cell data arrays will be read from the file.
   */
  vtkGetObjectMacro(CellDataArraySelection, vtkDataArraySelection);

  /**
   * Access the parcel data array selection to specify which point data arrays
   * should be read and associated parcel. Only the specified parcel data arrays will
   * be read from the file.
   */
  vtkGetObjectMacro(ParcelDataArraySelection, vtkDataArraySelection);

  /**
   * Determine if the file can be read with this reader.
   */
  virtual int CanReadFile(VTK_FILEPATH const char* fname);

  ///@{
  /**
   * Specify file name of the Exodus file.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

protected:
  vtkCONVERGECFDReader();
  ~vtkCONVERGECFDReader() override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  int RequestData(
    vtkInformation*, vtkInformationVector**, vtkInformationVector* outputVector) override;

  // Look for series of files defining timesteps
  void ReadTimeSteps(vtkInformation* outInfo);

  // Get the OUTPUT_TIME attribute from the file
  bool ReadOutputTime(const std::string& filePath, double& time);

  // From the given information request, return the index of the file that supplies the timestep
  size_t SelectTimeStepIndex(vtkInformation* info);

  // Name of file chosen in the file system
  char* FileName;

  // List of files that match the chosen file name
  std::vector<std::string> FileNames;

  vtkNew<vtkDataArraySelection> CellDataArraySelection;
  vtkNew<vtkDataArraySelection> ParcelDataArraySelection;

  class vtkInternal;
  vtkInternal* Internal;

private:
  vtkCONVERGECFDReader(const vtkCONVERGECFDReader&) = delete;
  void operator=(const vtkCONVERGECFDReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif // vtkCONVERGECFDReader_h
