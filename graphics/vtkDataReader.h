/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataReader.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2000 Ken Martin, Will Schroeder, Bill Lorensen 
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
ARE DISCLAIMED. IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
// .NAME vtkDataReader - helper class for objects that read vtk data files
// .SECTION Description
// vtkDataReader is a helper class that reads the vtk data file header,
// dataset type, and attribute data (point and cell attributes such as
// scalars, vectors, normals, etc.) from a vtk data file.  See text for
// format.

#ifndef __vtkDataReader_h
#define __vtkDataReader_h

#include "vtkObject.h"
#include "vtkSource.h"
#include "vtkDataSetAttributes.h"

#define VTK_ASCII 1
#define VTK_BINARY 2

class vtkDataSet;
class vtkPointSet;
class vtkRectilinearGrid;

class VTK_EXPORT vtkDataReader : public vtkObject
{
public:
  static vtkDataReader *New();
  vtkTypeMacro(vtkDataReader,vtkObject);
  void PrintSelf(vtkOstream& os, vtkIndent indent);

  // Description:
  // Specify file name of vtk data file to read.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Is the file a valid vtk file of the passed dataset type ?
  // The dataset type is passed as a lower case string.
  int IsFileValid(const char *dstype);
  int IsFileStructuredPoints() {
    return this->IsFileValid("structured_points");};  
  int IsFilePolyData() {
    return this->IsFileValid("polydata");};  
  int IsFileStructuredGrid() {
    return this->IsFileValid("structured_grid");};  
  int IsFileUnstructuredGrid() {
    return this->IsFileValid("unstructured_grid");};  
  int IsFileRectilinearGrid() {
    return this->IsFileValid("rectilinear_grid");};
  
  // Description:
  // Specify the InputString for use when reading from a character array.
  // Optionally include the length for binary strings.
  void SetInputString(char *in);
  vtkGetStringMacro(InputString);
  void SetInputString(char *in, int len);
  vtkGetMacro(InputStringLength, int);
  
  // Description:
  // Get the header from the vtk data file.
  vtkGetStringMacro(Header);

  // Description:
  // Enable reading from an InputString instead of the default, a file.
  vtkSetMacro(ReadFromInputString,int);
  vtkGetMacro(ReadFromInputString,int);
  vtkBooleanMacro(ReadFromInputString,int);

  // Description:
  // Get the type of file (ASCII or BINARY). Returned value only valid
  // after file has been read.
  vtkGetMacro(FileType,int);

  // Description:
  // Set the name of the scalar data to extract. If not specified, first 
  // scalar data encountered is extracted.
  vtkSetStringMacro(ScalarsName);
  vtkGetStringMacro(ScalarsName);

  // Description:
  // Set the name of the vector data to extract. If not specified, first 
  // vector data encountered is extracted.
  vtkSetStringMacro(VectorsName);
  vtkGetStringMacro(VectorsName);

  // Description:
  // Set the name of the tensor data to extract. If not specified, first 
  // tensor data encountered is extracted.
  vtkSetStringMacro(TensorsName);
  vtkGetStringMacro(TensorsName);

  // Description:
  // Set the name of the normal data to extract. If not specified, first 
  // normal data encountered is extracted.
  vtkSetStringMacro(NormalsName);
  vtkGetStringMacro(NormalsName);

  // Description:
  // Set the name of the texture coordinate data to extract. If not specified,
  // first texture coordinate data encountered is extracted.
  vtkSetStringMacro(TCoordsName);
  vtkGetStringMacro(TCoordsName);

  // Description:
  // Set the name of the lookup table data to extract. If not specified, uses 
  // lookup table named by scalar. Otherwise, this specification supersedes.
  vtkSetStringMacro(LookupTableName);
  vtkGetStringMacro(LookupTableName);

  // Description:
  // Set the name of the field data to extract. If not specified, uses 
  // first field data encountered in file.
  vtkSetStringMacro(FieldDataName);
  vtkGetStringMacro(FieldDataName);

  //BTX
  // Description:
  // Set/Get the name of the source object that owns this helper instance.
  // It is meant to be used by the source object, and does not reference
  // count the source.
  void SetSource(vtkSource *source);
  vtkGetObjectMacro(Source,vtkSource);
  //ETX
  
  // Description:
  // Open a vtk data file. Returns zero if error.
  int OpenVTKFile();

  // Description:
  // Read the header of a vtk data file. Returns 0 if error.
  int ReadHeader();

  // Description:
  // Read the cell data of a vtk data file. The number of cells (from the 
  // dataset) must match the number of cells defined in cell attributes (unless
  // no geometry was defined).
  int ReadCellData(vtkDataSet *ds, int numCells);

  // Description:
  // Read the point data of a vtk data file. The number of points (from the
  // dataset) must match the number of points defined in point attributes
  // (unless no geometry was defined).
  int ReadPointData(vtkDataSet *ds, int numPts);

  // Description:
  // Read point coordinates. Return 0 if error.
  int ReadPoints(vtkPointSet *ps, int numPts);

  // Description:
  // Read a bunch of "cells". Return 0 if error.
  int ReadCells(int size, int *data);

  // Description:
  // Read a piece of the cells (for streaming complience)
  int ReadCells(int size, int *data, int skip1, int read2, int skip3);

  // Description:
  // Read the coordinates for a rectilinear grid. The axes parameter specifies
  // which coordinate axes (0,1,2) is being read.
  int ReadCoordinates(vtkRectilinearGrid *rg, int axes, int numCoords);

  vtkDataArray *ReadArray(char *dataType, int numTuples, int numComp);
  vtkFieldData *ReadFieldData();

  // Description:
  // Internal function to read in a value.  Returns zero if there was an
  // error.
  int Read(char *);
  int Read(unsigned char *);
  int Read(short *);
  int Read(unsigned short *);
  int Read(int *);
  int Read(unsigned int *);
  int Read(long *);
  int Read(unsigned long *);
  int Read(float *);
  int Read(double *);

  // Description:
  // Close a vtk file.
  void CloseVTKFile();

  // Description:
  // Internal function to read in a line up to 256 characters.
  // Returns zero if there was an error.
  int ReadLine(char result[256]);

  // Description:
  // Internal function to read in a string up to 256 characters.
  // Returns zero if there was an error.
  int ReadString(char result[256]);

  // Description:
  // Helper method for reading in data.
  char *LowerCase(char *);
  
  // Description:
  // Internal function used to consume whitespace when reading in
  // an InputString.
  void EatWhiteSpace();

  // Description:
  // Return the istream being used to read in the data.
  vtkIstream *GetIStream() {return this->IS;};

protected:
  vtkDataReader();
  ~vtkDataReader();
  vtkDataReader(const vtkDataReader&) {};
  void operator=(const vtkDataReader&) {};

  char *FileName;
  int FileType;
  vtkIstream *IS;

  char *ScalarsName;
  char *VectorsName;
  char *TensorsName;
  char *TCoordsName;
  char *NormalsName;
  char *LookupTableName;
  char *FieldDataName;
  char *ScalarLut;

  int ReadFromInputString;
  char *InputString;
  int InputStringLength;
  int InputStringPos;

  vtkSetStringMacro(ScalarLut);
  vtkGetStringMacro(ScalarLut);

  vtkSource *Source;

  int ReadScalarData(vtkDataSetAttributes *a, int num);
  int ReadVectorData(vtkDataSetAttributes *a, int num);
  int ReadNormalData(vtkDataSetAttributes *a, int num);
  int ReadTensorData(vtkDataSetAttributes *a, int num);
  int ReadCoScalarData(vtkDataSetAttributes *a, int num);
  int ReadLutData(vtkDataSetAttributes *a);
  int ReadTCoordsData(vtkDataSetAttributes *a, int num);

  char *Header;

};

#endif


