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

class VTKIOLAS_EXPORT vtkLASReader: public vtkPolyDataAlgorithm
{
public:
  vtkLASReader(const vtkLASReader&) = delete;
  void operator=(const vtkLASReader&) = delete;
  static vtkLASReader* New();
  vtkTypeMacro(vtkLASReader,vtkPolyDataAlgorithm);
  virtual void PrintSelf(ostream &os, vtkIndent indent) override;

  /**
   * All the Visualization Types have been listed here
   */
  enum VisualizationTypeConstants {
    None = 0,
    Color,
    Classification
  };

  /**
   * All the Classification Types according to LAS spec are listed here
   */
  enum ClassificationType {
    Created_NotClassified = 0,
    Unclassified,     // 1
    Ground,           // 2
    LowVegetation,    // 3
    MediumVegetation, // 4
    HighVegetation,   // 5
    Building,         // 6
    LowPoint,         // 7
    ModelKeyPoint,    // 8
    Water             // 9
  };

  /**
   * Accessor for name of the file that will be opened
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  /**
   * Accessor for Visualization Type
   */
  vtkSetMacro(VisualizationType, VisualizationTypeConstants);
  vtkGetMacro(VisualizationType, VisualizationTypeConstants);

  /**
   * Accessor for the LAS Header file
   */
  vtkGetMacro(Header, liblas::Header *);

  /**
   * Set User specified color values in the Classification Color Map instead of the default values
   */
  void SetClassificationColor(ClassificationType type, unsigned char color[3]);
  void SetClassificationColor(ClassificationType type, unsigned char red, unsigned char green, unsigned char blue);


protected:
  vtkLASReader();
  virtual ~vtkLASReader();

  /**
   * Core implementation of the data set reader
   */
  int RequestData(vtkInformation* request, vtkInformationVector** inputVector,
                  vtkInformationVector* outputVector);

  /**
   * Read point record data i.e. position and visualisation data
   */
  void ReadPointRecordData(liblas::Reader &reader, vtkPolyData* pointsPolyData);

  /**
   * Map from Class Number to Corresponding Color
   */
  unsigned char ClassificationColorMap[10][3];

  int PointRecordsCount;
  VisualizationTypeConstants VisualizationType;
  liblas::Header* Header;
  char* FileName;
};

#endif // vtkLASReader_h
