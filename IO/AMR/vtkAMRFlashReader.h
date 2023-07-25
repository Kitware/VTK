// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAMREnzoReader
 * @brief   A concrete instance of vtkAMRBaseReader that implements functionality
 * for reading Flash AMR datasets.
 */

#ifndef vtkAMRFlashReader_h
#define vtkAMRFlashReader_h

#include "vtkAMRBaseReader.h"
#include "vtkIOAMRModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkOverlappingAMR;
class vtkFlashReaderInternal;

class VTKIOAMR_EXPORT vtkAMRFlashReader : public vtkAMRBaseReader
{
public:
  static vtkAMRFlashReader* New();
  vtkTypeMacro(vtkAMRFlashReader, vtkAMRBaseReader);
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
  vtkAMRFlashReader();
  ~vtkAMRFlashReader() override;

  /**
   * See vtkAMRBaseReader::ReadMetaData
   */
  void ReadMetaData() override;

  /**
   * See vtkAMRBaseReader::GetBlockLevel
   */
  int GetBlockLevel(int blockIdx) override;

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
   */
  void GetAMRGridPointData(int vtkNotUsed(blockIdx), vtkUniformGrid* vtkNotUsed(block),
    const char* vtkNotUsed(field)) override
  {
  }

  /**
   * See vtkAMRBaseReader::SetUpDataArraySelections
   */
  void SetUpDataArraySelections() override;

  bool IsReady;

private:
  vtkAMRFlashReader(const vtkAMRFlashReader&) = delete;
  void operator=(const vtkAMRFlashReader&) = delete;

  void ComputeStats(vtkFlashReaderInternal* internal, std::vector<int>& numBlocks, double min[3]);
  vtkFlashReaderInternal* Internal;
};

VTK_ABI_NAMESPACE_END
#endif /* vtkAMRFlashReader_h */
