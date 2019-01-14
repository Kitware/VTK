/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSegYReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef vtkSegYReader_h
#define vtkSegYReader_h

#include "vtkDataSetAlgorithm.h"

#include <vtkIOSegYModule.h> // For export macro

// Forward declarations
class vtkImageData;
class vtkSegYReaderInternal;

/**
 * @class vtkSegYReader
 * @brief Reads SegY data files.
 *
 * vtkSegYReader reads SegY data files. We create a vtkStructuredGrid
 * for 2.5D SegY and 3D data. If we set the StructuredGrid option to 0
 * we create a vtkImageData for 3D data. This saves memory and may
 * speed-up certain algorithms, but the position and the shape of the
 * data may not be correct. The axes for the data are: crossline,
 * inline, depth. For situations where traces are missing values of
 * zero are used to fill in the dataset.
 */
class VTKIOSEGY_EXPORT vtkSegYReader : public vtkDataSetAlgorithm
{
public:
  static vtkSegYReader* New();
  vtkTypeMacro(vtkSegYReader, vtkDataSetAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  vtkSegYReader();
  ~vtkSegYReader() override;

  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  enum VTKSegYCoordinateModes
  {
    VTK_SEGY_SOURCE = 0, // default
    VTK_SEGY_CDP = 1,
    VTK_SEGY_CUSTOM = 2
  };

  //@{
  /**
   * Specify whether to use source x/y coordinates or CDP coordinates or custom
   * byte positions for data position in the SEG-Y trace header. Defaults to
   * source x/y coordinates.
   *
   * As per SEG-Y rev 2.0 specification,
   * Source XY coordinate bytes = (73, 77)
   * CDP XY coordinate bytes = (181, 185)
   */
  vtkSetClampMacro(XYCoordMode, int, VTK_SEGY_SOURCE, VTK_SEGY_CUSTOM);
  vtkGetMacro(XYCoordMode, int);
  void SetXYCoordModeToSource();
  void SetXYCoordModeToCDP();
  void SetXYCoordModeToCustom();
  //@}

  //@{
  /**
   * Specify X and Y byte positions for custom XYCoordinateMode.
   * By default, XCoordByte = 73, YCoordByte = 77 i.e. source xy.
   *
   * \sa SetXYCoordinatesModeToCustom()
   */
  vtkSetMacro(XCoordByte, int);
  vtkGetMacro(XCoordByte, int);
  vtkSetMacro(YCoordByte, int);
  vtkGetMacro(YCoordByte, int);
  //@}

  enum VTKSegYVerticalCRS
  {
    VTK_SEGY_VERTICAL_HEIGHTS = 0, // default
    VTK_SEGY_VERTICAL_DEPTHS
  };

  //@{
  /**
   * Specify whether the vertical coordinates in the SEG-Y file are heights
   * (positive up) or depths (positive down). By default, the vertical
   * coordinates are treated as heights (i.e. positive up). This means that the
   * Z-axis of the dataset goes from 0 (surface) to -ve depth (last sample).
   * \note As per the SEG-Y rev 2.0 specification, this information is defined
   * in the Location Data Stanza of the Extended Textual Header. However, as of
   * this revision, vtkSegY2DReader does not support reading the extended
   * textual header.
   */
  vtkSetMacro(VerticalCRS, int);
  vtkGetMacro(VerticalCRS, int);
  //@}

  //@{
  /**
   * Specify if we create a vtkStructuredGrid even when the data is
   * 3D. Note this consumes more memory but it shows the precise
   * location for each point and the correct shape of the data. The
   * default value is true.  If we set this option to false we
   * create a vtkImageData for the SegY 3D dataset.
   */
  vtkSetMacro(StructuredGrid, int);
  vtkGetMacro(StructuredGrid, int);
  vtkBooleanMacro(StructuredGrid, int);
  //@}

protected:
  int RequestData(vtkInformation* request,
                  vtkInformationVector** inputVector,
                  vtkInformationVector* outputVector) override;

  int RequestInformation(vtkInformation* request,
                         vtkInformationVector** inputVector,
                         vtkInformationVector* outputVector) override;
  int RequestDataObject(vtkInformation* request,
                        vtkInformationVector** inputVector,
                        vtkInformationVector* outputVector) override;

protected:
  vtkSegYReaderInternal* Reader;
  char *FileName;
  bool Is3D;
  double DataOrigin[3];
  double DataSpacing[3][3];
  int DataSpacingSign[3];
  int DataExtent[6];

  int XYCoordMode;
  int StructuredGrid;

  // Custom XY coordinate byte positions
  int XCoordByte;
  int YCoordByte;

  int VerticalCRS;


private:
  vtkSegYReader(const vtkSegYReader&) = delete;
  void operator=(const vtkSegYReader&) = delete;
};

#endif // vtkSegYReader_h
