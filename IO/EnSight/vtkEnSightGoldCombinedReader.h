// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class vtkEnSightGoldCombinedReader.h
 * @brief class to read EnSight Gold files
 *
 * vtkEnSightGoldCombinedReader is a class to read EnSight Gold files into vtk.
 * This reader produces a vtkPartitionedDataSetCollection.
 * Currently this reader cannot be used in parallel; this support will be added in the future.
 */

#ifndef vtkEnSightGoldCombinedReader_h
#define vtkEnSightGoldCombinedReader_h

#include "vtkDoubleArray.h"
#include "vtkIOEnSightModule.h" // For export macro
#include "vtkPartitionedDataSetCollectionAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArraySelection;
class vtkPartitionedDataSetCollection;

class VTKIOENSIGHT_EXPORT vtkEnSightGoldCombinedReader
  : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
  static vtkEnSightGoldCombinedReader* New();
  vtkTypeMacro(vtkEnSightGoldCombinedReader, vtkPartitionedDataSetCollectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/Get the case file name.
   */
  vtkSetStringMacro(CaseFileName);
  vtkGetStringMacro(CaseFileName);
  ///@}

  ///@{
  /**
   * Set/Get the case file name.
   */
  vtkSetStringMacro(FilePath);
  vtkGetStringMacro(FilePath);
  ///@}

  ///@{
  /**
   * Get the time values per time set
   */
  vtkGetObjectMacro(AllTimeSteps, vtkDoubleArray);
  ///@}

  ///@{
  /**
   * Set/Get the time value.
   */
  vtkSetMacro(TimeValue, double);
  vtkGetMacro(TimeValue, double);
  ///@}

  /**
   * Checks version information in the case file to determine if the file
   * can be read by this reader.
   */
  int CanReadFile(VTK_FILEPATH const char* casefilename);

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
  vtkEnSightGoldCombinedReader();
  ~vtkEnSightGoldCombinedReader() override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  char* CaseFileName;
  char* FilePath;

  vtkSmartPointer<vtkDoubleArray> AllTimeSteps;
  double TimeValue;

private:
  vtkEnSightGoldCombinedReader(const vtkEnSightGoldCombinedReader&) = delete;
  void operator=(const vtkEnSightGoldCombinedReader&) = delete;

  struct ReaderImpl;
  ReaderImpl* Impl;
};

VTK_ABI_NAMESPACE_END
#endif // vtkEnSightGoldCombinedReader_h
