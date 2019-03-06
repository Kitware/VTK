/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAMReXGridReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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
  void SetFileName(const char* fileName) override;

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
  int GetBlockLevel(const int blockIdx) override;

  /**
   * GetLevelBlockID
   *
   * @param blockIdx
   *
   * @return int representing block in level blockIdx is contained in
   */
  int GetLevelBlockID(const int blockIdx);

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
    ;
  }

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

#endif
