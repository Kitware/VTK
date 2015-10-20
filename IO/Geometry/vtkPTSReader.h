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
// .NAME vtkPTSReader - Read ASCII PTS Files.
// .SECTION Description
// vtkPTSReader reads either a text file of
//  points. The first line is the number of points. Point information is
//  either x y z intensity or x y z intensity r g b

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
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify file name.
  void SetFileName(const char *filename);
  vtkGetStringMacro(FileName);

  // Description:
  // Boolean value indicates whether or not to limit points read to a specified
  // (ReadBounds) region.
  vtkBooleanMacro(LimitReadToBounds, bool);
  vtkSetMacro(LimitReadToBounds, bool);
  vtkGetMacro(LimitReadToBounds, bool);

  // Description:
  // Bounds to use if LimitReadToBounds is On
  vtkSetVector6Macro(ReadBounds, double);
  vtkGetVector6Macro(ReadBounds, double);

  // Description:
  // The output type defaults to float, but can instead be double.
  vtkBooleanMacro(OutputDataTypeIsDouble, bool);
  vtkSetMacro(OutputDataTypeIsDouble, bool);
  vtkGetMacro(OutputDataTypeIsDouble, bool);

  // Description:
  // Boolean value indicates whether or not to limit number of points read
  // based on MaxNumbeOfPoints.
  vtkBooleanMacro(LimitToMaxNumberOfPoints, bool);
  vtkSetMacro(LimitToMaxNumberOfPoints, bool);
  vtkGetMacro(LimitToMaxNumberOfPoints, bool);

  // Description:
  // The maximum number of points to load if LimitToMaxNumberOfPoints is on/true.
  // Sets a temporary onRatio.
  vtkSetClampMacro(MaxNumberOfPoints,vtkIdType,1,VTK_INT_MAX);
  vtkGetMacro(MaxNumberOfPoints,vtkIdType);

protected:
  vtkPTSReader();
  ~vtkPTSReader();

  int RequestInformation(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);

  char *FileName;
  bool OutputDataTypeIsDouble;

  bool LimitReadToBounds;
  double ReadBounds[6];
  vtkBoundingBox ReadBBox;
  bool LimitToMaxNumberOfPoints;
  vtkIdType MaxNumberOfPoints;

private:
  vtkPTSReader(const vtkPTSReader&);  // Not implemented.
  void operator=(const vtkPTSReader&);  // Not implemented.
};

#endif
