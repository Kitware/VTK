/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDataReader
 * @brief   helper superclass for objects that read vtk data files
 *
 * vtkDataReader is a helper superclass that reads the vtk data file header,
 * dataset type, and attribute data (point and cell attributes such as
 * scalars, vectors, normals, etc.) from a vtk data file.  See text for
 * the format of the various vtk file types.
 *
 * @sa
 * vtkPolyDataReader vtkStructuredPointsReader vtkStructuredGridReader
 * vtkUnstructuredGridReader vtkRectilinearGridReader
*/

#ifndef vtkDataReader_h
#define vtkDataReader_h

#include "vtkIOLegacyModule.h" // For export macro
#include "vtkAlgorithm.h"
#include "vtkStdString.h" // For API using strings

#define VTK_ASCII 1
#define VTK_BINARY 2

class vtkAbstractArray;
class vtkCharArray;
class vtkDataSet;
class vtkDataSetAttributes;
class vtkFieldData;
class vtkGraph;
class vtkPointSet;
class vtkRectilinearGrid;
class vtkTable;

class VTKIOLEGACY_EXPORT vtkDataReader : public vtkAlgorithm
{
public:
  enum FieldType
  {
    POINT_DATA,
    CELL_DATA,
    FIELD_DATA
  };

  static vtkDataReader *New();
  vtkTypeMacro(vtkDataReader,vtkAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Specify file name of vtk data file to read.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Is the file a valid vtk file of the passed dataset type ?
   * The dataset type is passed as a lower case string.
   */
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
  //@}

  //@{
  /**
   * Specify the InputString for use when reading from a character array.
   * Optionally include the length for binary strings. Note that a copy
   * of the string is made and stored. If this causes exceedingly large
   * memory consumption, consider using InputArray instead.
   */
  void SetInputString(const char *in);
  vtkGetStringMacro(InputString);
  void SetInputString(const char *in, int len);
  vtkGetMacro(InputStringLength, int);
  void SetBinaryInputString(const char *, int len);
  void SetInputString(const vtkStdString& input)
    { this->SetBinaryInputString(input.c_str(), static_cast<int>(input.length())); }
  //@}

  //@{
  /**
   * Specify the vtkCharArray to be used  when reading from a string.
   * If set, this array has precedence over InputString.
   * Use this instead of InputString to avoid the extra memory copy.
   * It should be noted that if the underlying char* is owned by the
   * user ( vtkCharArray::SetArray(array, 1); ) and is deleted before
   * the reader, bad things will happen during a pipeline update.
   */
  virtual void SetInputArray(vtkCharArray*);
  vtkGetObjectMacro(InputArray, vtkCharArray);
  //@}

  //@{
  /**
   * Get the header from the vtk data file.
   */
  vtkGetStringMacro(Header);
  //@}

  //@{
  /**
   * Enable reading from an InputString or InputArray instead of the default,
   * a file.
   */
  vtkSetMacro(ReadFromInputString,int);
  vtkGetMacro(ReadFromInputString,int);
  vtkBooleanMacro(ReadFromInputString,int);
  //@}

  //@{
  /**
   * Get the type of file (ASCII or BINARY). Returned value only valid
   * after file has been read.
   */
  vtkGetMacro(FileType,int);
  //@}

  /**
   * How many attributes of various types are in this file? This
   * requires reading the file, so the filename must be set prior
   * to invoking this operation. (Note: file characteristics are
   * cached, so only a single read is necessary to return file
   * characteristics.)
   */
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

  //@{
  /**
   * What is the name of the ith attribute of a certain type
   * in this file? This requires reading the file, so the filename
   * must be set prior to invoking this operation.
   */
  const char *GetScalarsNameInFile(int i);
  const char *GetVectorsNameInFile(int i);
  const char *GetTensorsNameInFile(int i);
  const char *GetNormalsNameInFile(int i);
  const char *GetTCoordsNameInFile(int i);
  const char *GetFieldDataNameInFile(int i);
  //@}

  //@{
  /**
   * Set the name of the scalar data to extract. If not specified, first
   * scalar data encountered is extracted.
   */
  vtkSetStringMacro(ScalarsName);
  vtkGetStringMacro(ScalarsName);
  //@}

  //@{
  /**
   * Set the name of the vector data to extract. If not specified, first
   * vector data encountered is extracted.
   */
  vtkSetStringMacro(VectorsName);
  vtkGetStringMacro(VectorsName);
  //@}

  //@{
  /**
   * Set the name of the tensor data to extract. If not specified, first
   * tensor data encountered is extracted.
   */
  vtkSetStringMacro(TensorsName);
  vtkGetStringMacro(TensorsName);
  //@}

  //@{
  /**
   * Set the name of the normal data to extract. If not specified, first
   * normal data encountered is extracted.
   */
  vtkSetStringMacro(NormalsName);
  vtkGetStringMacro(NormalsName);
  //@}

  //@{
  /**
   * Set the name of the texture coordinate data to extract. If not specified,
   * first texture coordinate data encountered is extracted.
   */
  vtkSetStringMacro(TCoordsName);
  vtkGetStringMacro(TCoordsName);
  //@}

  //@{
  /**
   * Set the name of the lookup table data to extract. If not specified, uses
   * lookup table named by scalar. Otherwise, this specification supersedes.
   */
  vtkSetStringMacro(LookupTableName);
  vtkGetStringMacro(LookupTableName);
  //@}

  //@{
  /**
   * Set the name of the field data to extract. If not specified, uses
   * first field data encountered in file.
   */
  vtkSetStringMacro(FieldDataName);
  vtkGetStringMacro(FieldDataName);
  //@}

  //@{
  /**
   * Enable reading all scalars.
   */
  vtkSetMacro(ReadAllScalars,int);
  vtkGetMacro(ReadAllScalars,int);
  vtkBooleanMacro(ReadAllScalars,int);
  //@}

  //@{
  /**
   * Enable reading all vectors.
   */
  vtkSetMacro(ReadAllVectors,int);
  vtkGetMacro(ReadAllVectors,int);
  vtkBooleanMacro(ReadAllVectors,int);
  //@}

  //@{
  /**
   * Enable reading all normals.
   */
  vtkSetMacro(ReadAllNormals,int);
  vtkGetMacro(ReadAllNormals,int);
  vtkBooleanMacro(ReadAllNormals,int);
  //@}

  //@{
  /**
   * Enable reading all tensors.
   */
  vtkSetMacro(ReadAllTensors,int);
  vtkGetMacro(ReadAllTensors,int);
  vtkBooleanMacro(ReadAllTensors,int);
  //@}

  //@{
  /**
   * Enable reading all color scalars.
   */
  vtkSetMacro(ReadAllColorScalars,int);
  vtkGetMacro(ReadAllColorScalars,int);
  vtkBooleanMacro(ReadAllColorScalars,int);
  //@}

  //@{
  /**
   * Enable reading all tcoords.
   */
  vtkSetMacro(ReadAllTCoords,int);
  vtkGetMacro(ReadAllTCoords,int);
  vtkBooleanMacro(ReadAllTCoords,int);
  //@}

  //@{
  /**
   * Enable reading all fields.
   */
  vtkSetMacro(ReadAllFields,int);
  vtkGetMacro(ReadAllFields,int);
  vtkBooleanMacro(ReadAllFields,int);
  //@}

  /**
   * Open a vtk data file. Returns zero if error.
   */
  int OpenVTKFile();

  /**
   * Read the header of a vtk data file. Returns 0 if error.
   */
  int ReadHeader();

  /**
   * Read the cell data of a vtk data file. The number of cells (from the
   * dataset) must match the number of cells defined in cell attributes (unless
   * no geometry was defined).
   */
  int ReadCellData(vtkDataSet *ds, vtkIdType numCells);

  /**
   * Read the point data of a vtk data file. The number of points (from the
   * dataset) must match the number of points defined in point attributes
   * (unless no geometry was defined).
   */
  int ReadPointData(vtkDataSet *ds, vtkIdType numPts);

  /**
   * Read point coordinates. Return 0 if error.
   */
  int ReadPoints(vtkPointSet *ps, vtkIdType numPts);

  /**
   * Read point coordinates. Return 0 if error.
   */
  int ReadPoints(vtkGraph *g, vtkIdType numPts);

  /**
   * Read the vertex data of a vtk data file. The number of vertices (from the
   * graph) must match the number of vertices defined in vertex attributes
   * (unless no geometry was defined).
   */
  int ReadVertexData(vtkGraph *g, vtkIdType numVertices);

  /**
   * Read the edge data of a vtk data file. The number of edges (from the
   * graph) must match the number of edges defined in edge attributes
   * (unless no geometry was defined).
   */
  int ReadEdgeData(vtkGraph *g, vtkIdType numEdges);

  /**
   * Read the row data of a vtk data file.
   */
  int ReadRowData(vtkTable *t, vtkIdType numEdges);

  /**
   * Read a bunch of "cells". Return 0 if error.
   */
  int ReadCells(vtkIdType size, int *data);

  /**
   * Read a piece of the cells (for streaming compliance)
   */
  int ReadCells(vtkIdType size, int *data, int skip1, int read2, int skip3);

  /**
   * Read the coordinates for a rectilinear grid. The axes parameter specifies
   * which coordinate axes (0,1,2) is being read.
   */
  int ReadCoordinates(vtkRectilinearGrid *rg, int axes, int numCoords);

  //@{
  /**
   * Helper functions for reading data.
   */
  vtkAbstractArray *ReadArray(const char *dataType, vtkIdType numTuples, vtkIdType numComp);
  vtkFieldData *ReadFieldData(FieldType fieldType = FIELD_DATA);
  //@}

  //@{
  /**
   * Return major and minor version of the file.
   * Returns version 3.0 if the version cannot be read from file.
   */
  vtkGetMacro(FileMajorVersion, int);
  vtkGetMacro(FileMinorVersion, int);
  //@}

  //@{
  /**
   * Internal function to read in a value.  Returns zero if there was an
   * error.
   */
  int Read(char *);
  int Read(unsigned char *);
  int Read(short *);
  int Read(unsigned short *);
  int Read(int *);
  int Read(unsigned int *);
  int Read(long *);
  int Read(unsigned long *);
  int Read(long long *result);
  int Read(unsigned long long *result);
  int Read(float *);
  int Read(double *);
  //@}

  /**
   * Read @a n character from the stream into @a str, then reset the stream
   * position. Returns the number of characters actually read.
   */
  size_t Peek(char *str, size_t n);


  /**
   * Close the vtk file.
   */
  void CloseVTKFile();

  /**
   * Internal function to read in a line up to 256 characters.
   * Returns zero if there was an error.
   */
  int ReadLine(char result[256]);

  /**
   * Internal function to read in a string up to 256 characters.
   * Returns zero if there was an error.
   */
  int ReadString(char result[256]);

  /**
   * Helper method for reading in data.
   */
  char *LowerCase(char *str, const size_t len=256);

  /**
   * Return the istream being used to read in the data.
   */
  istream *GetIStream() {return this->IS;};

  /**
   * Read the meta information from the file.  This needs to be public to it
   * can be accessed by vtkDataSetReader.
   */
  virtual int ReadMetaData(vtkInformation *) { return 1; }

protected:
  vtkDataReader();
  ~vtkDataReader() VTK_OVERRIDE;

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

  void SetScalarLut(const char* lut);
  vtkGetStringMacro(ScalarLut);

  char *Header;

  int ReadScalarData(vtkDataSetAttributes *a, vtkIdType num);
  int ReadVectorData(vtkDataSetAttributes *a, vtkIdType num);
  int ReadNormalData(vtkDataSetAttributes *a, vtkIdType num);
  int ReadTensorData(vtkDataSetAttributes *a, vtkIdType num, vtkIdType numComp = 9);
  int ReadCoScalarData(vtkDataSetAttributes *a, vtkIdType num);
  int ReadLutData(vtkDataSetAttributes *a);
  int ReadTCoordsData(vtkDataSetAttributes *a, vtkIdType num);
  int ReadGlobalIds(vtkDataSetAttributes *a, vtkIdType num);
  int ReadPedigreeIds(vtkDataSetAttributes *a, vtkIdType num);
  int ReadEdgeFlags(vtkDataSetAttributes *a, vtkIdType num);

  /**
   * Format is detailed \ref IOLegacyInformationFormat "here".
   */
  int ReadInformation(vtkInformation *info, vtkIdType numKeys);

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

  int ReadAllScalars;
  int ReadAllVectors;
  int ReadAllNormals;
  int ReadAllTensors;
  int ReadAllColorScalars;
  int ReadAllTCoords;
  int ReadAllFields;
  int FileMajorVersion;
  int FileMinorVersion;

  void InitializeCharacteristics();
  int CharacterizeFile(); //read entire file, storing important characteristics
  void CheckFor(const char* name, char *line, int &num, char** &array,
                int& allocSize);

  vtkCharArray* InputArray;

  /**
   * Decode a string. This method is the inverse of
   * vtkWriter::EncodeString.  Returns the length of the
   * result string.
   */
  int DecodeString(char *resname, const char* name);

  int ProcessRequest(vtkInformation *, vtkInformationVector **,
                             vtkInformationVector *) VTK_OVERRIDE;
  virtual int RequestData(vtkInformation *, vtkInformationVector **,
                          vtkInformationVector *)
    { return 1; }
  virtual int RequestUpdateExtent(vtkInformation *, vtkInformationVector **,
                                  vtkInformationVector *)
    { return 1; }
  virtual int RequestInformation(vtkInformation *, vtkInformationVector **,
                                 vtkInformationVector *)
    { return 1; }

private:
  vtkDataReader(const vtkDataReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDataReader&) VTK_DELETE_FUNCTION;

  void ConvertGhostLevelsToGhostType(
    FieldType fieldType, vtkAbstractArray *data) const;
};

#endif
