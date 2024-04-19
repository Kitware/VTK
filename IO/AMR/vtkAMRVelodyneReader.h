// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAMRVelodyneReader
 *
 *
 * A concrete instance of vtkAMRBaseReader that implements functionality
 * for reading Velodyne AMR datasets.
 */

#ifndef vtkAMRVelodyneReader_h
#define vtkAMRVelodyneReader_h
#include "vtkAMRBaseReader.h"
#include "vtkIOAMRModule.h" // For export macro
#include <string>           // for std::string
#include <unordered_map>    // for std::unordered_map
#include <vector>           // for std::vector

VTK_ABI_NAMESPACE_BEGIN
class vtkInformation;
class vtkInformationVector;
class vtkOverlappingAMR;
class vtkAMRVelodyneReaderInternal;

class VTKIOAMR_EXPORT vtkAMRVelodyneReader : public vtkAMRBaseReader
{
public:
  static vtkAMRVelodyneReader* New();
  vtkTypeMacro(vtkAMRVelodyneReader, vtkAMRBaseReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  int GetNumberOfBlocks() override;
  int GetNumberOfLevels() override;
  void SetFileName(VTK_FILEPATH const char* fileName) override;
  vtkOverlappingAMR* GetOutput();

protected:
  vtkAMRVelodyneReader();
  ~vtkAMRVelodyneReader() override;

  int RequestInformation(vtkInformation* rqst, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  int RequestData(vtkInformation* vtkNotUsed(request),
    vtkInformationVector** vtkNotUsed(inputVector), vtkInformationVector* outputVector) override;

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
  vtkAMRVelodyneReader(const vtkAMRVelodyneReader&) = delete;
  void operator=(const vtkAMRVelodyneReader&) = delete;
  vtkAMRVelodyneReaderInternal* Internal;
  void CalculateSpacing(double* dx, int lvl, double* spacing);
  void CalculateBlockDims(int* bDims, bool isFull, int* curDims);
  void MarkFileAsRead(char* fN);
  bool IsFileRead(char* fN);
  bool IsFileRead(const char* fN);
  void UpdateFileName(int index);
  std::vector<vtkOverlappingAMR*> amrVector;
  std::vector<double> timeList;
  std::vector<std::string> fileList;
  std::unordered_map<std::string, bool> LoadedHash;
  unsigned int currentIndex;
};
VTK_ABI_NAMESPACE_END
#endif
