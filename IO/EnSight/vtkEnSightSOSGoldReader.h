// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkEnSightSOSGoldReader
 * @brief   reader for EnSight SOS gold files
 *
 * Reads EnSight SOS files using vtkEnSightGoldCombinedReader.
 * When running in parallel, the decomposition of the data across ranks differs in this reader from
 * the parallel EnSight readers available in ParaView source (see
 * https://kitware.github.io/paraview-docs/latest/cxx/classvtkPVEnSightMasterServerReader2.html,
 * https://kitware.github.io/paraview-docs/latest/cxx/classvtkPEnSightGoldBinaryReader.html, and
 * https://kitware.github.io/paraview-docs/latest/cxx/classvtkPEnSightGoldReader.html). This reader
 * makes the assumption that most users will already have a good decomposition in the way their
 * files are written out from their solvers, so we will honor that.
 *
 * The format allows for partitions of parts to be contained in different casefiles making up the
 * SOS file. The old reader treated these partitions of the same part across casefiles as different
 * parts that were not related to each other, so then a part that is split across casefiles would
 * end up with a very weird decomposition.
 *
 * The initial strategy for this reader assigns whole casefiles to the available
 * processes as evenly as possible. This means that using the same number of processes as number of
 * servers listed in the SOS file is the most efficient - each rank will read one casefile. If there
 * are more processes than casefiles, some ranks will do no work, while if there are fewer processes
 * than casefiles, some rank(s) will read more than one casefile. In the future, we will add a
 * strategy that will consider the partition of a part in the casefile as an atomic unit, and those
 * partitions could be more evenly distributed across ranks.
 *
 * The output of the reader is a vtkPartitionedDataSetCollection. When a process reads multiple
 * casefiles, it will combine the output vtkPartitionedDataSetCollection from each of the
 * vtkEnSightGoldCombinedReader instances to output a single vtkPartitionedDataSetCollection where
 * the portions of parts read by different ranks are partitions of their respective
 * vtkPartitionedDataSet. For instance if 'Part 1' is empty in casefile1, and has its data split
 * between casefile2 and casefile3, the resulting vtkPartitionedDataSetCollection will contain only
 * one vtkPartitionedDataSet containing 'Part 1' with two partitions.
 */

#ifndef vtkEnSightSOSGoldReader_h
#define vtkEnSightSOSGoldReader_h

#include "vtkIOEnSightModule.h" // For export macro
#include "vtkPartitionedDataSetCollectionAlgorithm.h"
#include "vtkSmartPointer.h" // for vtkSmartPointer

VTK_ABI_NAMESPACE_BEGIN

class vtkEnSightGoldCombinedReader;
class vtkDataArraySelection;
class vtkMultiProcessController;

class VTKIOENSIGHT_EXPORT vtkEnSightSOSGoldReader : public vtkPartitionedDataSetCollectionAlgorithm
{
public:
  vtkTypeMacro(vtkEnSightSOSGoldReader, vtkPartitionedDataSetCollectionAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  static vtkEnSightSOSGoldReader* New();

  ///@{
  /**
   * Get/Set the controller
   */
  vtkMultiProcessController* GetController();
  void SetController(vtkMultiProcessController* controller);
  ///@}

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

  vtkSmartPointer<vtkMultiProcessController> Controller;

private:
  vtkEnSightSOSGoldReader(const vtkEnSightSOSGoldReader&) = delete;
  void operator=(const vtkEnSightSOSGoldReader&) = delete;

  struct ReaderImpl;
  ReaderImpl* Impl;
};

VTK_ABI_NAMESPACE_END
#endif // vtkEnSightSOSGoldReader_h
