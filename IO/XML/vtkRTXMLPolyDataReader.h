/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkRTXMLPolyDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkRTXMLPolyDataReader
 * @brief   Read RealTime VTK XML PolyData files.
 *
 * vtkRTXMLPolyDataReader reads the VTK XML PolyData file format in real time.
 *
*/

#ifndef vtkRTXMLPolyDataReader_h
#define vtkRTXMLPolyDataReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLPolyDataReader.h"

class vtkRTXMLPolyDataReaderInternals;

class VTKIOXML_EXPORT vtkRTXMLPolyDataReader : public vtkXMLPolyDataReader
{
public:
  vtkTypeMacro(vtkRTXMLPolyDataReader,vtkXMLPolyDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkRTXMLPolyDataReader* New();

  // This sets the DataLocation and also
  // Reset the reader by calling ResetReader()
  void SetLocation(const char* dataLocation);
  vtkGetStringMacro(DataLocation);

  /**
   * Reader will read in the next available data file
   * The filename is this->NextFileName maintained internally
   */
  virtual void UpdateToNextFile();

  /**
   * check if there is new data file available in the
   * given DataLocation
   */
  virtual int NewDataAvailable();

  /**
   * ResetReader check the data directory specified in
   * this->DataLocation, and reset the Internal data structure
   * specifically: this->Internal->ProcessedFileList
   * for monitoring the arriving new data files
   * if SetDataLocation(char*) is set by the user,
   * this ResetReader() should also be invoked.
   */
  virtual void ResetReader();

  /**
   * Return the name of the next available data file
   * assume NewDataAvailable() return VTK_OK
   */
  const char* GetNextFileName();

protected:
  vtkRTXMLPolyDataReader();
  ~vtkRTXMLPolyDataReader() VTK_OVERRIDE;

  //@{
  /**
   * Get/Set the location of the input data files.
   */
  vtkSetStringMacro(DataLocation);
  //@}

  void InitializeToCurrentDir();
  int IsProcessed(const char*);
  char* GetDataFileFullPathName(const char*);

  //@{
  /**
   * the DataLocation should be set and ResetReader()
   * should be called after SetDataLocation
   */
  char* DataLocation;
  vtkRTXMLPolyDataReaderInternals* Internal;
  //@}

private:
  vtkRTXMLPolyDataReader(const vtkRTXMLPolyDataReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkRTXMLPolyDataReader&) VTK_DELETE_FUNCTION;
};

#endif
