/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataReader.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
// .NAME vtkDataReader - helper class for objects that read vtk data files
// .SECTION Description
// vtkDataReader is a helper class that reads the vtk data file header,
// dataset type, and attribute data (point and cell attributes such as
// scalars, vectors, normals, etc.) from a vtk data file.  See text for
// format.

#ifndef __vtkDataReader_h
#define __vtkDataReader_h

#include <stdio.h>
#include <fstream.h>
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
  vtkDataReader();
  ~vtkDataReader();
  static vtkDataReader *New() {return new vtkDataReader;};
  const char *GetClassName() {return "vtkDataReader";};
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Specify file name of vtk data file to read.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Specify the InputString for use when reading from a character array.
  // Optionally include the length for binary strings.
  vtkSetStringMacro(InputString);
  vtkGetStringMacro(InputString);
  void SetInputString(char *in, int len);
  
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

  // Description:
  // Set/Get the name of the source object that owns this helper instance.
  vtkSetObjectMacro(Source,vtkSource);
  vtkGetObjectMacro(Source,vtkSource);

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
  // Read lookup table. Return 0 if error.
  int ReadCells(int size, int *data);

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
  istream *GetIStream() {return this->IS;};

protected:
  char *FileName;
  int FileType;
  istream *IS;

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

};

#endif


