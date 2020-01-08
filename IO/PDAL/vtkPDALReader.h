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
 * @class   vtkPDALReader
 * @brief   Reads LIDAR data using the PDAL library.
 *
 * vtkPDALReader reads LIDAR data using the PDAL library.  See the
 * readers section on www.pdal.io for the supported formats. It produces a
 * vtkPolyData with point data arrays for attributes such as Intensity,
 * Classification, Color, ...
 *
 *
 * @sa
 * vtkPolyData
 */

#ifndef vtkPDALReader_h
#define vtkPDALReader_h

#include <vtkIOPDALModule.h> // For export macro

#include <vtkPolyDataAlgorithm.h>

namespace pdal
{
class Stage;
};

class VTKIOPDAL_EXPORT vtkPDALReader : public vtkPolyDataAlgorithm
{
public:
  vtkPDALReader(const vtkPDALReader&) = delete;
  void operator=(const vtkPDALReader&) = delete;
  static vtkPDALReader* New();
  vtkTypeMacro(vtkPDALReader, vtkPolyDataAlgorithm);
  virtual void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Name of the file that will be opened
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

protected:
  vtkPDALReader();
  ~vtkPDALReader() override;

  /**
   * Core implementation of the data set reader
   */
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
    vtkInformationVector* outputVector) override;

  /**
   * Read point record data i.e. position and visualisation data
   */
  void ReadPointRecordData(pdal::Stage& reader, vtkPolyData* pointsPolyData);

  char* FileName;
};

#endif // vtkPDALReader_h
