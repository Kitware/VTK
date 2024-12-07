// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause

/**
 * @class vtkEnSightGoldCombinedReader.h
 * @brief class to read EnSight Gold files
 *
 * vtkEnSightGoldCombinedReader is a class to read EnSight Gold files into vtk.
 * This reader produces a vtkPartitionedDataSetCollection.
 *
 * The reader allows for selecting which parts to load, with all parts being loaded by default.
 * It also caches geometry when it is determined to be static instead of rereading the geometry
 * file on every time step.
 */

#ifndef vtkEnSightGoldCombinedReader_h
#define vtkEnSightGoldCombinedReader_h

#include "vtkDoubleArray.h"
#include "vtkIOEnSightModule.h" // For export macro
#include "vtkPartitionedDataSetCollectionAlgorithm.h"
#include "vtkSmartPointer.h" // for vtkSmartPointer

VTK_ABI_NAMESPACE_BEGIN
class vtkDataArraySelection;
class vtkIdTypeArray;
class vtkMultiProcessController;
class vtkPartitionedDataSetCollection;
class vtkStringArray;

class VTKIOENSIGHT_EXPORT vtkEnSightGoldCombinedReader
  : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
  static vtkEnSightGoldCombinedReader* New();
  vtkTypeMacro(vtkEnSightGoldCombinedReader, vtkPartitionedDataSetCollectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Get/Set the controller
   */
  vtkMultiProcessController* GetController();
  void SetController(vtkMultiProcessController* controller);
  ///@}

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

  ///@{
  /**
   * Set/Get PartOfSOSFile. If true, this reader is being read as part of an SOS file and this
   * reader will skip some communication (if running in parallel), to allow vtkEnSightSOSGoldReader
   * to handle that.
   */
  vtkSetMacro(PartOfSOSFile, bool);
  vtkGetMacro(PartOfSOSFile, bool);
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

  /**
   * Get the names of all parts that are found in this casefile during
   * EnSightDataSet::GetPartInfo().
   */
  vtkSmartPointer<vtkStringArray> GetPartNames();

  /**
   * Sets information about parts to be loaded.
   *
   * This must be called when loading data through a SOS file. It's possible that some casefiles may
   * not include info on all parts (even as an empty part). The vtkEnSightSOSGoldReader looks at
   * which parts are to be loaded, assigns them ids in the output vtkPartitionedDataSetCollection,
   * and provides the part names, since they may not be available in the current casefile. This
   * ensures that all ranks will have the same structure for the output PDC and matching name
   * metadata.
   *
   * @param indices Provides the index into the output vtkPartitionedDataSetCollection for all
   * parts. It should be the same size as the total number of parts across all casefiles being
   * loaded by an SOS file. If a part is not to be loaded, its value should be -1.
   * @param names The names of only the parts to actually be loaded. This is indexed by its index in
   * the output PDC.
   */
  void SetPDCInfoForLoadedParts(
    vtkSmartPointer<vtkIdTypeArray> indices, vtkSmartPointer<vtkStringArray> names);

protected:
  vtkEnSightGoldCombinedReader();
  ~vtkEnSightGoldCombinedReader() override;

  int RequestInformation(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;
  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  vtkSmartPointer<vtkMultiProcessController> Controller;

  char* CaseFileName;
  char* FilePath;

  vtkSmartPointer<vtkDoubleArray> AllTimeSteps;
  double TimeValue;

  bool PartOfSOSFile;

private:
  vtkEnSightGoldCombinedReader(const vtkEnSightGoldCombinedReader&) = delete;
  void operator=(const vtkEnSightGoldCombinedReader&) = delete;

  struct ReaderImpl;
  ReaderImpl* Impl;
};

VTK_ABI_NAMESPACE_END
#endif // vtkEnSightGoldCombinedReader_h
