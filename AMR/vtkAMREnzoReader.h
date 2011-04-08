/*=========================================================================

 Program:   Visualization Toolkit
 Module:    vtkAMREnzoReader.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
// .NAME vtkAMREnzoReader.h -- Reader for Enzo AMR datasets.
//
// .SECTION Description
// A concrete instance of vtkAMRBaseReader that implements functionality
// for reading Enzo AMR datasets.

#ifndef VTKAMRENZOREADER_H_
#define VTKAMRENZOREADER_H_

#include "vtkAMRBaseReader.h"
#include <vtkstd/vector> // STL vector Header

class vtkEnzoReaderInternal;

class VTK_AMR_EXPORT vtkAMREnzoReader : public vtkAMRBaseReader
{
  public:
    static vtkAMREnzoReader *New();
    vtkTypeMacro( vtkAMREnzoReader, vtkAMRBaseReader );
    void PrintSelf( std::ostream &s, vtkIndent indent );

    // Description:
    // Set the input filename.
    void SetFileName( const char* fileName );

  protected:
    vtkAMREnzoReader();
    virtual ~vtkAMREnzoReader();

    // Description:
    // See vtkAMRBaseReader::SetUpDataArraySelections
    void SetUpDataArraySelections();

    // Description:
    // See vtkAMRBaseReader::GetBlock
    void GetBlock( int index, vtkHierarchicalBoxDataSet *hbds,
                  vtkstd::vector< int > &idxcounter );

    // Description:
    // See vtkAMRBaseReader::GetBlock
    void ReadMetaData();

    // Description:
    // See vtkAMRBaseReader::GenerateBlockMap
    void GenerateBlockMap();

    // Description:
    // See vtkAMRBaseReader::GetNumberOfBlocks
    int GetNumberOfBlocks();

    // Description:
    // See vtkAMRBaseReader::GetNumberOfBlocks
    int GetNumberOfLevels();

  private:
    vtkEnzoReaderInternal *Internal;
    vtkAMREnzoReader( const vtkAMREnzoReader& ); // Not Implemented
    void operator=(const vtkAMREnzoReader& ); // Not Implemented
};

#endif /* VTKAMRENZOREADER_H_ */
