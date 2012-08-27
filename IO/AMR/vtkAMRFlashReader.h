/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMRFlashReader.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkAMREnzoReader.h -- Reader for Flash AMR datasets.
//
// .SECTION Description
// A concrete instance of vtkAMRBaseReader that implements functionality
// for reading Flash AMR datasets.

#ifndef VTKAMRFLASHREADER_H_
#define VTKAMRFLASHREADER_H_

#include "vtkIOAMRModule.h" // For export macro
#include "vtkAMRBaseReader.h"

class vtkOverlappingAMR;
class vtkFlashReaderInternal;

class VTKIOAMR_EXPORT vtkAMRFlashReader : public vtkAMRBaseReader
{
public:
  static vtkAMRFlashReader* New();
  vtkTypeMacro( vtkAMRFlashReader, vtkAMRBaseReader );
  void PrintSelf(ostream &os, vtkIndent indent );

  // Description:
  // See vtkAMRBaseReader::GetNumberOfBlocks
  int GetNumberOfBlocks();

  // Description:
  // See vtkAMRBaseReader::GetNumberOfLevels
  int GetNumberOfLevels();

  // Description:
  // See vtkAMRBaseReader::SetFileName
  void SetFileName( const char* fileName );

protected:
  vtkAMRFlashReader();
  ~vtkAMRFlashReader();

  // Description:
  // See vtkAMRBaseReader::ReadMetaData
  void ReadMetaData();

  // Description:
  // See vtkAMRBaseReader::GetBlockLevel
  int GetBlockLevel( const int blockIdx );

  // Description:
  // See vtkAMRBaseReader::FillMetaData
  int FillMetaData( );

  // Description:
  // See vtkAMRBaseReader::GetAMRGrid
  vtkUniformGrid* GetAMRGrid( const int blockIdx );

  // Description:
  // See vtkAMRBaseReader::GetAMRGridData
  void GetAMRGridData(
      const int blockIdx, vtkUniformGrid *block, const char *field);

  // Description:
  // See vtkAMRBaseReader::SetUpDataArraySelections
  void SetUpDataArraySelections();

  bool IsReady;

private:
  vtkAMRFlashReader( const vtkAMRFlashReader& ); // Not implemented
  void operator=(const vtkAMRFlashReader& ); // Not implemented

  void ComputeStats(vtkFlashReaderInternal* internal, std::vector<int>& numBlocks, double min[3]);
  vtkFlashReaderInternal *Internal;
};

#endif /* VTKAMRFLASHREADER_H_ */
