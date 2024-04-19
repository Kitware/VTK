// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class vtkAMReXGridReader
 * @brief reader for AMReX plotfiles grid data.
 *
 * vtkAMReXGridReader readers grid data from AMReX plotfiles.
 */

#ifndef vtkAMReXGridReader_h
#define vtkAMReXGridReader_h

#include "vtkAMRBaseReader.h"
#include "vtkIOAMRModule.h" // For export macro
#include "vtkNew.h"         // for vtkNew

#include <string> // for std::string.
#include <vector> // for std::vector.

VTK_ABI_NAMESPACE_BEGIN
class vtkOverlappingAMR;
class vtkAMReXGridReaderInternal;

class VTKIOAMR_EXPORT vtkAMReXGridReader : public vtkAMRBaseReader
{
public:
  static vtkAMReXGridReader* New();
  vtkTypeMacro(vtkAMReXGridReader, vtkAMRBaseReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * See vtkAMRBaseReader::GetNumberOfBlocks
   */
  int GetNumberOfBlocks() override;

  /**
   * See vtkAMRBaseReader::GetNumberOfLevels
   */
  int GetNumberOfLevels() override;

  /**
   * See vtkAMRBaseReader::SetFileName
   */
  void SetFileName(VTK_FILEPATH const char* fileName) override;

protected:
  vtkAMReXGridReader();
  ~vtkAMReXGridReader() override;

  /**
   * See vtkAMRBaseReader::ReadMetaData
   */
  void ReadMetaData() override;

  /**
   * See vtkAMRBaseReader::GetBlockLevel
   */
  int GetBlockLevel(int blockIdx) override;

  /**
   * GetLevelBlockID
   *
   * @param blockIdx
   *
   * @return int representing block in level blockIdx is contained in
   */
  int GetLevelBlockID(int blockIdx);

  /**
   * See vtkAMRBaseReader::FillMetaData
   */
  int FillMetaData() override;

  /**
   * See vtkAMRBaseReader::GetAMRGrid
   */
  vtkUniformGrid* GetAMRGrid(int blockIdx) override;

  /**
   * See vtkAMRBaseReader::GetAMRGridData
   */
  void GetAMRGridData(int blockIdx, vtkUniformGrid* block, const char* field) override;

  /**
   * See vtkAMRBaseReader::GetAMRGridData
   * Note: currently, nodal data is only supported in extra multifabs
   */
  void GetAMRGridPointData(int blockIdx, vtkUniformGrid* block, const char* field) override;

  /**
   * See vtkAMRBaseReader::SetUpDataArraySelections
   */
  void SetUpDataArraySelections() override;

  int GetDimension();
  bool IsReady;

private:
  vtkAMReXGridReader(const vtkAMReXGridReader&) = delete;
  void operator=(const vtkAMReXGridReader&) = delete;

  void ComputeStats(
    vtkAMReXGridReaderInternal* internal, std::vector<int>& numBlocks, double min[3]);
  vtkAMReXGridReaderInternal* Internal;
};

VTK_ABI_NAMESPACE_END
#endif
