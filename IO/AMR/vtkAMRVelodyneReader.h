/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRVelodyneReader.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
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
  int GetBlockLevel(const int blockIdx) override;

  /**
   * See vtkAMRBaseReader::FillMetaData
   */
  int FillMetaData() override;

  /**
   * See vtkAMRBaseReader::GetAMRGrid
   */
  vtkUniformGrid* GetAMRGrid(const int blockIdx) override;
  /**
   * See vtkAMRBaseReader::GetAMRGridData
   */
  void GetAMRGridData(const int blockIdx, vtkUniformGrid* block, const char* field) override;
  /**
   * See vtkAMRBaseReader::GetAMRGridData
   */

  void GetAMRGridPointData(const int vtkNotUsed(blockIdx), vtkUniformGrid* vtkNotUsed(block),
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
#endif
