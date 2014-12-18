/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkSTLWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkSTLWriter - write stereo lithography files
// .SECTION Description
// vtkSTLWriter writes stereo lithography (.stl) files in either ASCII or
// binary form. Stereo lithography files only contain triangles. If polygons
// with more than 3 vertices are present, only the first 3 vertices are
// written.  Use vtkTriangleFilter to convert polygons to triangles.

// .SECTION Caveats
// Binary files written on one system may not be readable on other systems.
// vtkSTLWriter uses VAX or PC byte ordering and swaps bytes on other systems.

#ifndef vtkSTLWriter_h
#define vtkSTLWriter_h

#include "vtkIOGeometryModule.h" // For export macro
#include "vtkWriter.h"

class vtkCellArray;
class vtkPoints;
class vtkPolyData;

class VTKIOGEOMETRY_EXPORT vtkSTLWriter : public vtkWriter
{
public:
  static vtkSTLWriter *New();
  vtkTypeMacro(vtkSTLWriter,vtkWriter);
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the input to this writer.
  vtkPolyData* GetInput();
  vtkPolyData* GetInput(int port);

  // Description:
  // Specify file name of vtk polygon data file to write.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Specify file type (ASCII or BINARY) for vtk data file.
  vtkSetClampMacro(FileType,int,VTK_ASCII,VTK_BINARY);
  vtkGetMacro(FileType,int);
  void SetFileTypeToASCII() {this->SetFileType(VTK_ASCII);};
  void SetFileTypeToBinary() {this->SetFileType(VTK_BINARY);};

protected:
  vtkSTLWriter();
  ~vtkSTLWriter()
    {
    delete[] this->FileName;
    delete[] this->Header;
    }

  void WriteData();

  void WriteBinarySTL(vtkPoints *pts, vtkCellArray *polys);
  void WriteAsciiSTL(vtkPoints *pts, vtkCellArray *polys);

  char* FileName;
  char *Header;
  int FileType;

  virtual int FillInputPortInformation(int port, vtkInformation *info);

private:
  vtkSTLWriter(const vtkSTLWriter&);  // Not implemented.
  void operator=(const vtkSTLWriter&);  // Not implemented.
};

#endif

