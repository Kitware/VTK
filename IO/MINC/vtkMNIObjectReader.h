/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMNIObjectReader.h

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
/**
 * @class   vtkMNIObjectReader
 * @brief   A reader for MNI surface mesh files.
 *
 * The MNI .obj file format is used to store geometrical data.  This
 * file format was developed at the McConnell Brain Imaging Centre at
 * the Montreal Neurological Institute and is used by their software.
 * Only polygon and line files are supported by this reader, but for
 * those formats, all data elements are read including normals, colors,
 * and surface properties.  ASCII and binary file types are supported.
 * @sa
 * vtkMINCImageReader vtkMNIObjectWriter vtkMNITransformReader
 * @par Thanks:
 * Thanks to David Gobbi for writing this class and Atamai Inc. for
 * contributing it to VTK.
*/

#ifndef vtkMNIObjectReader_h
#define vtkMNIObjectReader_h

#include "vtkIOMINCModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"

class vtkProperty;
class vtkPolyData;
class vtkFloatArray;
class vtkIntArray;
class vtkPoints;
class vtkCellArray;

class VTKIOMINC_EXPORT vtkMNIObjectReader : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkMNIObjectReader,vtkPolyDataAlgorithm);

  static vtkMNIObjectReader *New();
  void PrintSelf(ostream& os, vtkIndent indent) override;

  //@{
  /**
   * Set the file name.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  /**
   * Get the entension for this file format.
   */
  virtual const char* GetFileExtensions() {
    return ".obj"; }

  /**
   * Get the name of this file format.
   */
  virtual const char* GetDescriptiveName() {
    return "MNI object"; }

  /**
   * Test whether the specified file can be read.
   */
  virtual int CanReadFile(const char* name);

  /**
   * Get the property associated with the object.
   */
  virtual vtkProperty *GetProperty() { return this->Property; };

protected:
  vtkMNIObjectReader();
  ~vtkMNIObjectReader() override;

  char *FileName;
  vtkProperty *Property;
  int FileType;

  istream *InputStream;
  int LineNumber;
  char *LineText;
  char *CharPointer;

  int ReadLine(char *text, unsigned int length);
  int SkipWhitespace();
  int ParseValues(vtkDataArray *array, vtkIdType n);
  int ParseIdValue(vtkIdType *value);

  int ReadNumberOfPoints(vtkIdType *numCells);
  int ReadNumberOfCells(vtkIdType *numCells);
  int ReadProperty(vtkProperty *property);
  int ReadLineThickness(vtkProperty *property);
  int ReadPoints(vtkPolyData *polyData, vtkIdType numPoints);
  int ReadNormals(vtkPolyData *polyData, vtkIdType numPoints);
  int ReadColors(vtkProperty *property, vtkPolyData *data,
                 vtkIdType numPoints, vtkIdType numCells);
  int ReadCells(vtkPolyData *data, vtkIdType numCells, int cellType);

  int ReadPolygonObject(vtkPolyData *output);
  int ReadLineObject(vtkPolyData *output);

  virtual int ReadFile(vtkPolyData *output);

  int RequestData(vtkInformation* request,
                          vtkInformationVector** inInfo,
                          vtkInformationVector* outInfo) override;

private:
  vtkMNIObjectReader(const vtkMNIObjectReader&) = delete;
  void operator=(const vtkMNIObjectReader&) = delete;

};

#endif
