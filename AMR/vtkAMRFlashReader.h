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

#include "vtkAMRBaseReader.h"

class vtkHierarchicalBoxDataSet;
class vtkFlashReaderInternal;

class VTK_AMR_EXPORT vtkAMRFlashReader : public vtkAMRBaseReader
{
  public:
    static vtkAMRFlashReader* New();
    vtkTypeMacro( vtkAMRFlashReader, vtkAMRBaseReader );
    void PrintSelf( std::ostream &os, vtkIndent indent );

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
    // See vtkAMRBaseReader::GenerateBlockMap
    void GenerateBlockMap();

    // Description:
    // See vtkAMRBaseReader::GetBlockLevel
    int GetBlockLevel( const int blockIdx );

    // Description:
    // See vtkAMRBaseReader::GetNumberOfBlocks
    int GetNumberOfBlocks();

    // Description:
    // See vtkAMRBaseReader::GetNumberOfLevels
    int GetNumberOfLevels();

    // Description:
    // See vtkAMRBaseReader::FillMetaData
    int FillMetaData( vtkHierarchicalBoxDataSet *metadata );

    // Description:
    // See vtkAMRBaseReader::GetBlock
    void GetBlock(
        int index, vtkHierarchicalBoxDataSet *hbds,
        vtkstd::vector< int > &idxcounter);

    // Description:
    // See vtkAMRBaseReader::SetUpDataArraySelections
    void SetUpDataArraySelections();

  private:
    vtkAMRFlashReader( const vtkAMRFlashReader& ); // Not implemented
    void operator=(const vtkAMRFlashReader& ); // Not implemented

    vtkFlashReaderInternal *Internal;
};

#endif /* VTKAMRFLASHREADER_H_ */
