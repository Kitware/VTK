/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMNIObjectWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*=========================================================================

Copyright (c) 2006 Atamai, Inc.

Use, modification and redistribution of the software, in source or
binary forms, are permitted provided that the following terms and
conditions are met:

1) Redistribution of the source code, in verbatim or modified
   form, must retain the above copyright notice, this license,
   the following disclaimer, and any notices that refer to this
   license and/or the following disclaimer.

2) Redistribution in binary form must include the above copyright
   notice, a copy of this license and the following disclaimer
   in the documentation or with other materials provided with the
   distribution.

3) Modified copies of the source code must be clearly marked as such,
   and must not be misrepresented as verbatim copies of the source code.

THE COPYRIGHT HOLDERS AND/OR OTHER PARTIES PROVIDE THE SOFTWARE "AS IS"
WITHOUT EXPRESSED OR IMPLIED WARRANTY INCLUDING, BUT NOT LIMITED TO,
THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
PURPOSE.  IN NO EVENT SHALL ANY COPYRIGHT HOLDER OR OTHER PARTY WHO MAY
MODIFY AND/OR REDISTRIBUTE THE SOFTWARE UNDER THE TERMS OF THIS LICENSE
BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, LOSS OF DATA OR DATA BECOMING INACCURATE
OR LOSS OF PROFIT OR BUSINESS INTERRUPTION) ARISING IN ANY WAY OUT OF
THE USE OR INABILITY TO USE THE SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGES.

=========================================================================*/
// .NAME vtkMNIObjectWriter - A writer for MNI surface mesh files.
// .SECTION Description
// The MNI .obj file format is used to store geometrical data.  This
// file format was developed at the McConnell Brain Imaging Centre at
// the Montreal Neurological Institute and is used by their software.
// Only polygon and line files are supported by this writer.  For these
// formats, all data elements are written including normals, colors,
// and surface properties.  ASCII and binary file types are supported.
// .SECTION See Also
// vtkMINCImageReader vtkMNIObjectReader vtkMNITransformReader
// .SECTION Thanks
// Thanks to David Gobbi for writing this class and Atamai Inc. for
// contributing it to VTK.

#ifndef vtkMNIObjectWriter_h
#define vtkMNIObjectWriter_h

#include "vtkIOMINCModule.h" // For export macro
#include "vtkWriter.h"

class vtkMapper;
class vtkProperty;
class vtkLookupTable;
class vtkPolyData;
class vtkFloatArray;
class vtkIntArray;
class vtkPoints;

class VTKIOMINC_EXPORT vtkMNIObjectWriter : public vtkWriter
{
public:
  vtkTypeMacro(vtkMNIObjectWriter,vtkWriter);

  static vtkMNIObjectWriter *New();
  virtual void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the entension for this file format.
  virtual const char* GetFileExtensions() {
    return ".obj"; }

  // Description:
  // Get the name of this file format.
  virtual const char* GetDescriptiveName() {
    return "MNI object"; }

  // Description:
  // Set the property associated with the object.  Optional.
  // This is useful for exporting an actor.
  virtual void SetProperty(vtkProperty *property);
  virtual vtkProperty *GetProperty() { return this->Property; };

  // Description:
  // Set the mapper associated with the object.  Optional.
  // This is useful for exporting an actor with the same colors
  // that are used to display the actor within VTK.
  virtual void SetMapper(vtkMapper *mapper);
  virtual vtkMapper *GetMapper() { return this->Mapper; };

  // Description:
  // Set the lookup table associated with the object.  This will be
  // used to convert scalar values to colors, if a mapper is not set.
  virtual void SetLookupTable(vtkLookupTable *table);
  virtual vtkLookupTable *GetLookupTable() { return this->LookupTable; };

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
  vtkMNIObjectWriter();
  ~vtkMNIObjectWriter();

  vtkProperty *Property;
  vtkMapper *Mapper;
  vtkLookupTable *LookupTable;

  ostream *OutputStream;

  int WriteObjectType(int objType);
  int WriteValues(vtkDataArray *array);
  int WriteIdValue(vtkIdType value);
  int WriteNewline();

  int WriteProperty(vtkProperty *property);
  int WriteLineThickness(vtkProperty *property);
  int WritePoints(vtkPolyData *polyData);
  int WriteNormals(vtkPolyData *polyData);
  int WriteColors(vtkProperty *property, vtkMapper *mapper, vtkPolyData *data);
  int WriteCells(vtkPolyData *data, int cellType);

  int WritePolygonObject(vtkPolyData *output);
  int WriteLineObject(vtkPolyData *output);

  void WriteData();

  char* FileName;

  int FileType;

  virtual int FillInputPortInformation(int port, vtkInformation *info);

  ostream *OpenFile();
  void CloseFile(ostream *fp);

private:
  vtkMNIObjectWriter(const vtkMNIObjectWriter&); // Not implemented
  void operator=(const vtkMNIObjectWriter&);  // Not implemented

};

#endif
