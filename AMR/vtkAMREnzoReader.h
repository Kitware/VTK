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

#include "vtkstd/map"     // For STL map

class vtkHierarchicalBoxDataSet;
class vtkEnzoReaderInternal;

class VTK_AMR_EXPORT vtkAMREnzoReader : public vtkAMRBaseReader
{
  public:
    static vtkAMREnzoReader* New();
    vtkTypeMacro(vtkAMREnzoReader,vtkAMRBaseReader);
    void PrintSelf(std::ostream &os, vtkIndent indent );

    // Description:
    // Set/Get whether data should be converted to CGS
    vtkSetMacro( ConvertToCGS, int );
    vtkGetMacro( ConvertToCGS, int );
    vtkBooleanMacro( ConvertToCGS, int );

    // Description:
    // See vtkAMRBaseReader::SetFileName
    void SetFileName( const char* fileName );

  protected:
    vtkAMREnzoReader();
    ~vtkAMREnzoReader();

    // Description:
    // Parses the parameters file and extracts the
    // conversion factors that are used to convert
    // to CGS units.
    void ParseConversionFactors();

    // Description:
    // Given an array name of the form "array[idx]" this method
    // extracts and returns the corresponding index idx.
    int GetIndexFromArrayName( std::string arrayName );

    // Description:
    // Given the label string, this method parses the attribute label and
    // the string index.
    void ParseLabel(const std::string labelString,int &idx,std::string &label);

    // Description:
    // Given the label string, this method parses the corresponding attribute
    // index and conversion factor
    void ParseCFactor(const std::string labelString, int &idx, double &factor );

    // Description:
    // Given the variable name, return the conversion factor used to convert
    // the data to CGS. These conversion factors are read directly from the
    // parameters file when the filename is set.
    double GetConversionFactor( const std::string name );

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
    int FillMetaData( );

    // Description:
    // See vtkAMRBaseReader::GetBlock
    void GetBlock(
        int index, vtkHierarchicalBoxDataSet *hbds,
        vtkstd::vector< int > &idxcounter);

    // Description:
    // See vtkAMRBaseReader::SetUpDataArraySelections
    void SetUpDataArraySelections();

    int ConvertToCGS;

  private:
    vtkAMREnzoReader( const vtkAMREnzoReader& ); // Not Implemented
    void operator=(const vtkAMREnzoReader& ); // Not Implemented

    vtkEnzoReaderInternal *Internal;

    vtkstd::map< vtkstd::string, int > label2idx;
    vtkstd::map< int, double >    conversionFactors;
};

#endif /* VTKAMRENZOREADER_H_ */
