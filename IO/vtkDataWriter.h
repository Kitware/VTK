/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataWriter.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkDataWriter - helper class for objects that write vtk data files
// .SECTION Description
// vtkDataWriter is a helper class that opens and writes the vtk header and 
// point data (e.g., scalars, vectors, normals, etc.) from a vtk data file. 
// See text for various formats.

// .SECTION See Also
// vtkDataSetWriter vtkPolyDataWriter vtkStructuredGridWriter
// vtkStructuredPointsWriter vtkUnstructuredGridWriter
// vtkFieldDataWriter vtkRectilinearGridWriter

#ifndef __vtkDataWriter_h
#define __vtkDataWriter_h

#include <stdio.h>
#include "vtkWriter.h"

class vtkDataSet;
class vtkPoints;
class vtkCellArray;
class vtkDataArray;

class VTK_IO_EXPORT vtkDataWriter : public vtkWriter
{
public:
  vtkTypeMacro(vtkDataWriter,vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Created object with default header, ASCII format, and default names for 
  // scalars, vectors, tensors, normals, and texture coordinates.
  static vtkDataWriter *New();

  // Description:
  // Specify file name of vtk polygon data file to write.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Enable writing to an OutputString instead of the default, a file.
  vtkSetMacro(WriteToOutputString,int);
  vtkGetMacro(WriteToOutputString,int);
  vtkBooleanMacro(WriteToOutputString,int);

  // Description:
  // When WriteToOutputString in on, then a string is allocated, written to,
  // and can be retrieved with these methods.  The string is deleted during
  // the next call to write ...
  vtkGetMacro(OutputStringLength, int);  
  vtkGetStringMacro(OutputString);
  unsigned char *GetBinaryOutputString() {
      return (unsigned char *)this->OutputString;};
      
  // Description:
  // This convenience method returns the string, sets the IVAR to NULL,
  // so that the user is responsible for deleting the string.
  // I am not sure what the name should be, so it may change in the future.
  char *RegisterAndGetOutputString();
  
  // Description:
  // Specify the header for the vtk data file.
  vtkSetStringMacro(Header);
  vtkGetStringMacro(Header);

  // Description:
  // Specify file type (ASCII or BINARY) for vtk data file.
  vtkSetClampMacro(FileType,int,VTK_ASCII,VTK_BINARY);
  vtkGetMacro(FileType,int);
  void SetFileTypeToASCII() {this->SetFileType(VTK_ASCII);};
  void SetFileTypeToBinary() {this->SetFileType(VTK_BINARY);};

  // Description:
  // Give a name to the scalar data. If not specified, uses default
  // name "scalars".
  vtkSetStringMacro(ScalarsName);
  vtkGetStringMacro(ScalarsName);

  // Description:
  // Give a name to the vector data. If not specified, uses default
  // name "vectors".
  vtkSetStringMacro(VectorsName);
  vtkGetStringMacro(VectorsName);

  // Description:
  // Give a name to the tensors data. If not specified, uses default
  // name "tensors".
  vtkSetStringMacro(TensorsName);
  vtkGetStringMacro(TensorsName);

  // Description:
  // Give a name to the normals data. If not specified, uses default
  // name "normals".
  vtkSetStringMacro(NormalsName);
  vtkGetStringMacro(NormalsName);

  // Description:
  // Give a name to the texture coordinates data. If not specified, uses 
  // default name "textureCoords".
  vtkSetStringMacro(TCoordsName);
  vtkGetStringMacro(TCoordsName);

  // Description:
  // Give a name to the lookup table. If not specified, uses default
  // name "lookupTable".
  vtkSetStringMacro(LookupTableName);
  vtkGetStringMacro(LookupTableName);

  // Description:
  // Give a name to the field data. If not specified, uses default 
  // name "field".
  vtkSetStringMacro(FieldDataName);
  vtkGetStringMacro(FieldDataName);

  // Description:
  // Open a vtk data file. Returns NULL if error.
  virtual ostream *OpenVTKFile();

  // Description:
  // Write the header of a vtk data file. Returns 0 if error.
  int WriteHeader(ostream *fp);

  // Description:
  // Write out the points of the data set.
  int WritePoints(ostream *fp, vtkPoints *p);

  // Description:
  // Write out coordinates for rectilinear grids.
  int WriteCoordinates(ostream *fp, vtkDataArray *coords, int axes);

  // Description:
  // Write out the cells of the data set.
  int WriteCells(ostream *fp, vtkCellArray *cells, const char *label);

  // Description:
  // Write the cell data (e.g., scalars, vectors, ...) of a vtk dataset.
  // Returns 0 if error.
  int WriteCellData(ostream *fp, vtkDataSet *ds);

  // Description:
  // Write the point data (e.g., scalars, vectors, ...) of a vtk dataset.
  // Returns 0 if error.
  int WritePointData(ostream *fp, vtkDataSet *ds);

  // Description:
  // Write out the field data.
  int WriteFieldData(ostream *fp, vtkFieldData *f);

  // Description:
  // Write out the data associated with the dataset (i.e. field data owned by
  // the dataset itself - distinct from that owned by the cells or points).
  int WriteDataSetData(ostream *fp, vtkDataSet *ds);

  // Description:
  // Close a vtk file.
  void CloseVTKFile(ostream *fp);


protected:
  vtkDataWriter();
  ~vtkDataWriter();

  int WriteToOutputString;
  char *OutputString;
  int OutputStringLength;
  int OutputStringAllocatedLength;
  
  void WriteData(); //dummy method to allow this class to be instantiated and delegated to

  char *FileName;
  char *Header;
  int FileType;

  char *ScalarsName;
  char *VectorsName;
  char *TensorsName;
  char *TCoordsName;
  char *NormalsName;
  char *LookupTableName;
  char *FieldDataName;

  int WriteArray(ostream *fp, int dataType, vtkDataArray *data, const char *format, 
		 int num, int numComp);
  int WriteScalarData(ostream *fp, vtkDataArray *s, int num);
  int WriteVectorData(ostream *fp, vtkDataArray *v, int num);
  int WriteNormalData(ostream *fp, vtkDataArray *n, int num);
  int WriteTCoordData(ostream *fp, vtkDataArray *tc, int num);
  int WriteTensorData(ostream *fp, vtkDataArray *t, int num);

private:
  vtkDataWriter(const vtkDataWriter&);  // Not implemented.
  void operator=(const vtkDataWriter&);  // Not implemented.
};

#endif


