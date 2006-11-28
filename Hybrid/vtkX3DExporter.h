/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkX3DExporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtX3DExporter - create an x3d file
// .SECTION Description

#ifndef __vtkX3DExporter_h
#define __vtkX3DExporter_h

#include "vtkExporter.h"

class vtkLight;
class vtkActor;
class vtkActor2D;
class vtkPoints;
class vtkDataArray;
class vtkUnsignedCharArray;
class vtkX3DExporterWriter;

class  VTK_HYBRID_EXPORT vtkX3DExporter : public vtkExporter
{
public:
  static vtkX3DExporter *New();
  vtkTypeRevisionMacro(vtkX3DExporter,vtkExporter);
  void PrintSelf(ostream& os, vtkIndent indent);


  // Description:
  // Set/Get the output file name.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Specify the Speed of navigation. Default is 4.
  vtkSetMacro(Speed,double);
  vtkGetMacro(Speed,double);

  // Description:
  // Turn on binary mode
  vtkSetClampMacro(Binary, int, 0, 1);
  vtkBooleanMacro(Binary, int);
  vtkGetMacro(Binary, int);

protected:
  vtkX3DExporter();
  ~vtkX3DExporter();

  // Description:
  // Write data to output.
  void WriteData();

  void WriteALight(vtkLight *aLight, vtkX3DExporterWriter* writer);
  void WriteAnActor(vtkActor *anActor, vtkX3DExporterWriter* writer,
    int index);
  void WritePointData(vtkPoints *points, vtkDataArray *normals,
    vtkDataArray *tcoords, vtkUnsignedCharArray *colors,
    vtkX3DExporterWriter* writer, int index);
  void WriteanTextActor2D(vtkActor2D *anTextActor2D,
    vtkX3DExporterWriter* writer);

  char *FileName;
  double Speed;
  int Binary;

private:
  vtkX3DExporter(const vtkX3DExporter&); // Not implemented.
  void operator=(const vtkX3DExporter&); // Not implemented.
};


#endif
