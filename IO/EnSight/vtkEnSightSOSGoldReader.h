// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkEnSightSOSGoldReader
 * @brief   reader for EnSight SOS gold files
 *
 * Reads EnSight SOS files using vtkEnSightGoldCombinedReader.
 * Note: Currently this reader does not work in parallel.
 */

#ifndef vtkEnSightSOSGoldReader_h
#define vtkEnSightSOSGoldReader_h

#include "vtkIOEnSightModule.h" // For export macro
#include "vtkPartitionedDataSetCollectionAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN

class vtkEnSightGoldCombinedReader;
class vtkDataArraySelection;

class VTKIOENSIGHT_EXPORT vtkEnSightSOSGoldReader : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
  vtkTypeMacro(vtkEnSightSOSGoldReader, vtkPartitionedDataSetCollectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkEnSightSOSGoldReader* New();

  ///@{
  /**
   * Get/Set the SOS file name that will be read.
   */
  vtkGetFilePathMacro(CaseFileName);
  vtkSetFilePathMacro(CaseFileName);
  ///@}

  int CanReadFile(VTK_FILEPATH const char* fname);

  /**
   * Part selection, to determine which blocks/parts
   * are loaded.
   */
  vtkDataArraySelection* GetPartSelection();

  /**
   * Point array selection, to determine which point arrays
   * are loaded.
   */
  vtkDataArraySelection* GetPointArraySelection();

  /**
   * Cell array selection, to determine which cell arrays
   * are loaded.
   */
  vtkDataArraySelection* GetCellArraySelection();

  /**
   * Field data array selection, to determine which arrays
   * are loaded.
   */
  vtkDataArraySelection* GetFieldArraySelection();

  /**
   * Overridden to take into account mtimes for vtkDataArraySelection instances.
   */
  vtkMTimeType GetMTime() override;

protected:
  vtkEnSightSOSGoldReader();
  ~vtkEnSightSOSGoldReader() override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  char* CaseFileName;

private:
  vtkEnSightSOSGoldReader(const vtkEnSightSOSGoldReader&) = delete;
  void operator=(const vtkEnSightSOSGoldReader&) = delete;

  struct ReaderImpl;
  ReaderImpl* Impl;
};

VTK_ABI_NAMESPACE_END
#endif // vtkEnSightSOSGoldReader_h
