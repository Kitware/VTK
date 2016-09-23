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
/**
 * @class   vtkAMREnzoReader
 *
 *
 * A concrete instance of vtkAMRBaseReader that implements functionality
 * for reading Enzo AMR datasets.
*/

#ifndef vtkAMREnzoReader_h
#define vtkAMREnzoReader_h

#include "vtkIOAMRModule.h" // For export macro
#include "vtkAMRBaseReader.h"

#include <map>     // For STL map
#include <string>  // For std::string

class vtkOverlappingAMR;
class vtkEnzoReaderInternal;

class VTKIOAMR_EXPORT vtkAMREnzoReader : public vtkAMRBaseReader
{
public:
  static vtkAMREnzoReader* New();
  vtkTypeMacro(vtkAMREnzoReader,vtkAMRBaseReader);
  void PrintSelf(ostream &os, vtkIndent indent );

  //@{
  /**
   * Set/Get whether data should be converted to CGS
   */
  vtkSetMacro( ConvertToCGS, int );
  vtkGetMacro( ConvertToCGS, int );
  vtkBooleanMacro( ConvertToCGS, int );
  //@}

  /**
   * See vtkAMRBaseReader::GetNumberOfBlocks
   */
  int GetNumberOfBlocks();

  /**
   * See vtkAMRBaseReader::GetNumberOfLevels
   */
  int GetNumberOfLevels();

  /**
   * See vtkAMRBaseReader::SetFileName
   */
  void SetFileName( const char* fileName );

protected:
  vtkAMREnzoReader();
  ~vtkAMREnzoReader();

  /**
   * Parses the parameters file and extracts the
   * conversion factors that are used to convert
   * to CGS units.
   */
  void ParseConversionFactors();

  /**
   * Given an array name of the form "array[idx]" this method
   * extracts and returns the corresponding index idx.
   */
  int GetIndexFromArrayName( std::string arrayName );

  /**
   * Given the label string, this method parses the attribute label and
   * the string index.
   */
  void ParseLabel(const std::string &labelString, int &idx, std::string &label);

  /**
   * Given the label string, this method parses the corresponding attribute
   * index and conversion factor
   */
  void ParseCFactor(const std::string &labelString, int &idx, double &factor );

  /**
   * Given the variable name, return the conversion factor used to convert
   * the data to CGS. These conversion factors are read directly from the
   * parameters file when the filename is set.
   */
  double GetConversionFactor( const std::string& name );

  /**
   * See vtkAMRBaseReader::ReadMetaData
   */
  void ReadMetaData();

  /**
   * See vtkAMRBaseReader::GetBlockLevel
   */
  int GetBlockLevel( const int blockIdx );

  void ComputeStats(vtkEnzoReaderInternal* internal, std::vector<int>& blocksPerLevel, double min[3]);

  /**
   * See vtkAMRBaseReader::FillMetaData
   */
  int FillMetaData( );

  /**
   * See vtkAMRBaseReader::GetAMRGrid
   */
  vtkUniformGrid* GetAMRGrid( const int blockIdx );

  /**
   * See vtkAMRBaseReader::GetAMRGridData
   */
  void GetAMRGridData(
      const int blockIdx, vtkUniformGrid *block, const char *field);

  /**
   * See vtkAMRBaseReader::GetAMRGridData
   */
  void GetAMRGridPointData(
      const int vtkNotUsed(blockIdx), vtkUniformGrid *vtkNotUsed(block), const char *vtkNotUsed(field)) {;};

  /**
   * See vtkAMRBaseReader::SetUpDataArraySelections
   */
  void SetUpDataArraySelections();

  int ConvertToCGS;
  bool IsReady;

private:
  vtkAMREnzoReader( const vtkAMREnzoReader& ) VTK_DELETE_FUNCTION;
  void operator=(const vtkAMREnzoReader& ) VTK_DELETE_FUNCTION;

  vtkEnzoReaderInternal *Internal;

  std::map< std::string, int > label2idx;
  std::map< int, double >    conversionFactors;
};

#endif /* vtkAMREnzoReader_h */
