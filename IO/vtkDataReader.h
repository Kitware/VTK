/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataReader.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkDataReader - helper superclass for objects that read vtk data files
// .SECTION Description
// vtkDataReader is a helper superclass that reads the vtk data file header,
// dataset type, and attribute data (point and cell attributes such as
// scalars, vectors, normals, etc.) from a vtk data file.  See text for
// the format of the various vtk file types.
//
// .SECTION See Also
// vtkPolyDataReader vtkStructuredPointsReader vtkStructuredGridReader
// vtkUnsutructuredGridReader vtkRectilinearGridReader

#ifndef __vtkDataReader_h
#define __vtkDataReader_h

#include "vtkSource.h"

#define VTK_ASCII 1
#define VTK_BINARY 2

class vtkCharArray;
class vtkDataSet;
class vtkDataSetAttributes;
class vtkPointSet;
class vtkRectilinearGrid;

class VTK_IO_EXPORT vtkDataReader : public vtkSource
{
public:
  static vtkDataReader *New();
  vtkTypeRevisionMacro(vtkDataReader,vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

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
  // Optionally include the length for binary strings. Note that a copy
  // of the string is made and stored. If this causes exceedingly large
  // memory consumption, consider using InputArray instead.
  void SetInputString(const char *in);
  vtkGetStringMacro(InputString);
  void SetInputString(const char *in, int len);
  vtkGetMacro(InputStringLength, int);
  void SetBinaryInputString(const char *, int len);

  // Description:
  // Specify the vtkCharArray to be used  when reading from a string.
  // If set, this array has precendence over InputString.
  // Use this instead of InputString to avoid the extra memory copy.
  // It should be noted that if the underlying char* is owned by the
  // user ( vtkCharArray::SetArray(array, 1); ) and is deleted before
  // the reader, bad things will happen during a pipeline update.
  virtual void SetInputArray(vtkCharArray*);
  vtkGetObjectMacro(InputArray, vtkCharArray);
    
  // Description:
  // Get the header from the vtk data file.
  vtkGetStringMacro(Header);

  // Description:
  // Enable reading from an InputString or InputArray instead of the default, 
  // a file.
  vtkSetMacro(ReadFromInputString,int);
  vtkGetMacro(ReadFromInputString,int);
  vtkBooleanMacro(ReadFromInputString,int);

  // Description:
  // Get the type of file (ASCII or BINARY). Returned value only valid
  // after file has been read.
  vtkGetMacro(FileType,int);

  // Description:
  // How many attributes of various types are in this file? This 
  // requires reading the file, so the filename must be set prior 
  // to invoking this operation. (Note: file characteristics are
  // cached, so only a single read is necessary to return file
  // characteristics.)
  int GetNumberOfScalarsInFile()
    {this->CharacterizeFile(); return this->NumberOfScalarsInFile;}
  int GetNumberOfVectorsInFile()
    {this->CharacterizeFile(); return this->NumberOfVectorsInFile;}
  int GetNumberOfTensorsInFile()
    {this->CharacterizeFile(); return this->NumberOfTensorsInFile;}
  int GetNumberOfNormalsInFile()
    {this->CharacterizeFile(); return this->NumberOfNormalsInFile;}
  int GetNumberOfTCoordsInFile()
    {this->CharacterizeFile(); return this->NumberOfTCoordsInFile;}
  int GetNumberOfFieldDataInFile()
    {this->CharacterizeFile(); return this->NumberOfFieldDataInFile;}
  
  // Description:
  // What is the name of the ith attribute of a certain type
  // in this file? This requires reading the file, so the filename 
  // must be set prior to invoking this operation.
  const char *GetScalarsNameInFile(int i);
  const char *GetVectorsNameInFile(int i);
  const char *GetTensorsNameInFile(int i);
  const char *GetNormalsNameInFile(int i);
  const char *GetTCoordsNameInFile(int i);
  const char *GetFieldDataNameInFile(int i);
  
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
  // Read a piece of the cells (for streaming compliance)
  int ReadCells(int size, int *data, int skip1, int read2, int skip3);

  // Description:
  // Read the coordinates for a rectilinear grid. The axes parameter specifies
  // which coordinate axes (0,1,2) is being read.
  int ReadCoordinates(vtkRectilinearGrid *rg, int axes, int numCoords);

  // Description:
  // Helper functions for reading data.
  vtkDataArray *ReadArray(const char *dataType, int numTuples, int numComp);
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
  // Close the vtk file.
  void CloseVTKFile();

//BTX
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
  char *LowerCase(char *str, const size_t len=256);
  
  // Description:
  // Return the istream being used to read in the data.
  istream *GetIStream() {return this->IS;};
//ETX

protected:
  vtkDataReader();
  ~vtkDataReader();

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
  int InputStringLength;
  int InputStringPos;

  vtkSetStringMacro(ScalarLut);
  vtkGetStringMacro(ScalarLut);

  char *Header;

  int ReadScalarData(vtkDataSetAttributes *a, int num);
  int ReadVectorData(vtkDataSetAttributes *a, int num);
  int ReadNormalData(vtkDataSetAttributes *a, int num);
  int ReadTensorData(vtkDataSetAttributes *a, int num);
  int ReadCoScalarData(vtkDataSetAttributes *a, int num);
  int ReadLutData(vtkDataSetAttributes *a);
  int ReadTCoordsData(vtkDataSetAttributes *a, int num);

  int ReadDataSetData(vtkDataSet *ds);

  // This supports getting additional information from vtk files
  int  NumberOfScalarsInFile;
  char **ScalarsNameInFile;
  int ScalarsNameAllocSize;
  int  NumberOfVectorsInFile;
  char **VectorsNameInFile;
  int VectorsNameAllocSize;
  int  NumberOfTensorsInFile;
  char **TensorsNameInFile;
  int TensorsNameAllocSize;
  int  NumberOfTCoordsInFile;
  char **TCoordsNameInFile;
  int TCoordsNameAllocSize;
  int  NumberOfNormalsInFile;
  char **NormalsNameInFile;
  int NormalsNameAllocSize;
  int  NumberOfFieldDataInFile;
  char **FieldDataNameInFile;
  int FieldDataNameAllocSize;
  vtkTimeStamp CharacteristicsTime;

  void InitializeCharacteristics();
  int CharacterizeFile(); //read entire file, storing important characteristics
  void CheckFor(const char* name, char *line, int &num, char** &array, 
                int& allocSize);

  vtkCharArray* InputArray;

  // Description:
  // Decode the name of array. This method is the inverse of 
  // vtkWriter::EncodeName.
  void DecodeArrayName(char *resname, const char* name);

private:
  vtkDataReader(const vtkDataReader&);  // Not implemented.
  void operator=(const vtkDataReader&);  // Not implemented.
};

#endif


