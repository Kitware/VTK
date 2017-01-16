/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPTSReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkPTSReader
 * @brief   Read ASCII PTS Files.
 *
 * vtkPTSReader reads either a text file of
 *  points. The first line is the number of points. Point information is
 *  either x y z intensity or x y z intensity r g b
*/

#ifndef vtkPTSReader_h
#define vtkPTSReader_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"
#include "vtkBoundingBox.h" // For Bounding Box Data Member


class VTKIOGEOMETRY_EXPORT vtkPTSReader : public vtkPolyDataAlgorithm
{
public:
  static vtkPTSReader *New();
  vtkTypeMacro(vtkPTSReader,vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Specify file name.
   */
  void SetFileName(const char *filename);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Boolean value indicates whether or not to limit points read to a specified
   * (ReadBounds) region.
   */
  vtkBooleanMacro(LimitReadToBounds, bool);
  vtkSetMacro(LimitReadToBounds, bool);
  vtkGetMacro(LimitReadToBounds, bool);
  //@}

  //@{
  /**
   * Bounds to use if LimitReadToBounds is On
   */
  vtkSetVector6Macro(ReadBounds, double);
  vtkGetVector6Macro(ReadBounds, double);
  //@}

  //@{
  /**
   * The output type defaults to float, but can instead be double.
   */
  vtkBooleanMacro(OutputDataTypeIsDouble, bool);
  vtkSetMacro(OutputDataTypeIsDouble, bool);
  vtkGetMacro(OutputDataTypeIsDouble, bool);
  //@}

  //@{
  /**
   * Boolean value indicates whether or not to limit number of points read
   * based on MaxNumbeOfPoints.
   */
  vtkBooleanMacro(LimitToMaxNumberOfPoints, bool);
  vtkSetMacro(LimitToMaxNumberOfPoints, bool);
  vtkGetMacro(LimitToMaxNumberOfPoints, bool);
  //@}

  //@{
  /**
   * The maximum number of points to load if LimitToMaxNumberOfPoints is on/true.
   * Sets a temporary onRatio.
   */
  vtkSetClampMacro(MaxNumberOfPoints,vtkIdType,1,VTK_INT_MAX);
  vtkGetMacro(MaxNumberOfPoints,vtkIdType);
  //@}

  //@{
  /**
   * Boolean value indicates whether or not to create cells
   * for this dataset. Otherwise only points and scalars
   * are created. Defaults to true.
   */
  vtkBooleanMacro(CreateCells, bool);
  vtkSetMacro(CreateCells, bool);
  vtkGetMacro(CreateCells, bool);
  //@}

  //@{
  /**
   * Boolean value indicates when color values are present
   * if luminance should be read in as well
   * Defaults to true.
   */
  vtkBooleanMacro(IncludeColorAndLuminance, bool);
  vtkSetMacro(IncludeColorAndLuminance, bool);
  vtkGetMacro(IncludeColorAndLuminance, bool);
  //@}

protected:
  vtkPTSReader();
  ~vtkPTSReader() VTK_OVERRIDE;

  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *) VTK_OVERRIDE;

  char *FileName;
  bool OutputDataTypeIsDouble;

  bool LimitReadToBounds;
  double ReadBounds[6];
  vtkBoundingBox ReadBBox;
  bool LimitToMaxNumberOfPoints;
  vtkIdType MaxNumberOfPoints;
  bool CreateCells;
  bool IncludeColorAndLuminance;

private:
  vtkPTSReader(const vtkPTSReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPTSReader&) VTK_DELETE_FUNCTION;
};

#endif
