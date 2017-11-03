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
 * @class   vtkPdalReader
 * @brief   Reads LIDAR data using the PDAL library.
 *
 * vtkPdalReader reads LIDAR data using the PDAL library.  See the
 * readers section on www.pdal.io for the supported formats. It produces a
 * vtkPolyData with point data arrays for attributes such as Intensity,
 * Classification, Color, ...
 *
 *
 * @sa
 * vtkPolyData
*/

#ifndef vtkPdalReader_h
#define vtkPdalReader_h

#include <vtkIOPDALModule.h> // For export macro

#include <vtkPolyDataAlgorithm.h>

namespace pdal
{
  class Reader;
};

class VTKIOPDAL_EXPORT vtkPdalReader: public vtkPolyDataAlgorithm
{
public:
  vtkPdalReader(const vtkPdalReader&) = delete;
  void operator=(const vtkPdalReader&) = delete;
  static vtkPdalReader* New();
  vtkTypeMacro(vtkPdalReader,vtkPolyDataAlgorithm);
  virtual void PrintSelf(ostream &os, vtkIndent indent) override;

  /**
   * Name of the file that will be opened
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

protected:
  vtkPdalReader();
  virtual ~vtkPdalReader();

  /**
   * Core implementation of the data set reader
   */
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
                  vtkInformationVector* outputVector);

  /**
   * Read point record data i.e. position and visualisation data
   */
  void ReadPointRecordData(pdal::Reader &reader, vtkPolyData* pointsPolyData);

  char* FileName;
};

#endif // vtkPdalReader_h
