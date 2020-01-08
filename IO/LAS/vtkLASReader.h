/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGDALRasterReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkLASRasterReader
 * @brief   Reads LIDAR data saved using the LAS file format.
 *
 * vtkLASReader is a source object that reads LIDAR data saved using
 * the LAS file format. This reader uses the libLAS library.
 * It produces a vtkPolyData with point data arrays:
 * "intensity": vtkUnsignedShortArray
 * "classification": vtkUnsignedCharArray (optional)
 * "color": vtkUnsignedShortArray (optional)
 *
 *
 * @sa
 * vtkPolyData
 */

#ifndef vtkLASReader_h
#define vtkLASReader_h

#include <vtkIOLASModule.h> // For export macro

#include <vtkPolyDataAlgorithm.h>

namespace liblas
{
class Header;
class Reader;
};

class VTKIOLAS_EXPORT vtkLASReader : public vtkPolyDataAlgorithm
{
public:
  vtkLASReader(const vtkLASReader&) = delete;
  void operator=(const vtkLASReader&) = delete;
  static vtkLASReader* New();
  vtkTypeMacro(vtkLASReader, vtkPolyDataAlgorithm);
  virtual void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Accessor for name of the file that will be opened
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

protected:
  vtkLASReader();
  ~vtkLASReader() override;

  /**
   * Core implementation of the data set reader
   */
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * Read point record data i.e. position and visualisation data
   */
  void ReadPointRecordData(liblas::Reader& reader, vtkPolyData* pointsPolyData);

  char* FileName;
};

#endif // vtkLASReader_h
