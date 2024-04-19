// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
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
#include "vtkSimpleReader.h"
#include "vtkStdString.h" // For API using strings

#include <vtkSmartPointer.h> // for smart pointer

#include <locale> // For locale settings

#define VTK_ASCII 1
#define VTK_BINARY 2

VTK_ABI_NAMESPACE_BEGIN
class vtkAbstractArray;
class vtkCharArray;
class vtkCellArray;
class vtkDataSet;
class vtkDataSetAttributes;
class vtkFieldData;
class vtkGraph;
class vtkPointSet;
class vtkRectilinearGrid;
class vtkTable;

class VTKIOLEGACY_EXPORT vtkDataReader : public vtkSimpleReader
{
public:
  enum FieldType
  {
    POINT_DATA,
    CELL_DATA,
    FIELD_DATA
  };

  static vtkDataReader* New();
  vtkTypeMacro(vtkDataReader, vtkSimpleReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify file name of vtk data file to read. This is just
   * a convenience method that calls the superclass' AddFileName
   * method.
   */
  void SetFileName(VTK_FILEPATH const char* fname);
  VTK_FILEPATH const char* GetFileName() const;
  VTK_FILEPATH const char* GetFileName(int i) const
  {
    return this->vtkSimpleReader::GetFileName(i);
  }
  ///@}

  ///@{
  /**
   * Return the version of the file read; for example, VTK legacy readers
   * will return the version of the VTK legacy file. (In the case of VTK
   * legacy files, see vtkDataWriter.h for the enum types returned.) This
   * method only returns useful information after a successful read is
   * performed; and some derived classes may not return relevant
   * information.) Note that for VTK legacy readers, the FileVersion is
   * defined by the compositing the major version digits with the minor
   * version digit. Extremely ancient VTK files (e.g., before version 4.2)
   * will return a FileVersion of 3.0.
   */
  vtkGetMacro(FileVersion, int);
  vtkGetMacro(FileMajorVersion, int);
  vtkGetMacro(FileMinorVersion, int);
  ///@}

  ///@{
  /**
   * Is the file a valid vtk file of the passed dataset type ?
   * The dataset type is passed as a lower case string.
   */
  int IsFileValid(const char* dstype);
  int IsFileStructuredPoints() { return this->IsFileValid("structured_points"); }
  int IsFilePolyData() { return this->IsFileValid("polydata"); }
  int IsFileStructuredGrid() { return this->IsFileValid("structured_grid"); }
  int IsFileUnstructuredGrid() { return this->IsFileValid("unstructured_grid"); }
  int IsFileRectilinearGrid() { return this->IsFileValid("rectilinear_grid"); }
  ///@}

  ///@{
  /**
   * Specify the InputString for use when reading from a character array.
   * Optionally include the length for binary strings. Note that a copy
   * of the string is made and stored. If this causes exceedingly large
   * memory consumption, consider using InputArray instead.
   */
  void SetInputString(const char* in);
  vtkGetStringMacro(InputString);
  void SetInputString(const char* in, int len);
  vtkGetMacro(InputStringLength, int);
  void SetBinaryInputString(const char*, int len);
  void SetInputString(const vtkStdString& input)
  {
    this->SetBinaryInputString(input.c_str(), static_cast<int>(input.length()));
  }
  ///@}

  ///@{
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
  ///@}

  ///@{
  /**
   * Get the header from the vtk data file.
   */
  vtkGetStringMacro(Header);
  ///@}

  ///@{
  /**
   * Enable reading from an InputString or InputArray instead of the default,
   * a file.
   */
  vtkSetMacro(ReadFromInputString, vtkTypeBool);
  vtkGetMacro(ReadFromInputString, vtkTypeBool);
  vtkBooleanMacro(ReadFromInputString, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Get the type of file (ASCII or BINARY). Returned value only valid
   * after file has been read.
   */
  vtkGetMacro(FileType, int);
  ///@}

  /**
   * How many attributes of various types are in this file? This
   * requires reading the file, so the filename must be set prior
   * to invoking this operation. (Note: file characteristics are
   * cached, so only a single read is necessary to return file
   * characteristics.)
   */
  int GetNumberOfScalarsInFile()
  {
    this->CharacterizeFile();
    return this->NumberOfScalarsInFile;
  }
  int GetNumberOfVectorsInFile()
  {
    this->CharacterizeFile();
    return this->NumberOfVectorsInFile;
  }
  int GetNumberOfTensorsInFile()
  {
    this->CharacterizeFile();
    return this->NumberOfTensorsInFile;
  }
  int GetNumberOfNormalsInFile()
  {
    this->CharacterizeFile();
    return this->NumberOfNormalsInFile;
  }
  int GetNumberOfTCoordsInFile()
  {
    this->CharacterizeFile();
    return this->NumberOfTCoordsInFile;
  }
  int GetNumberOfFieldDataInFile()
  {
    this->CharacterizeFile();
    return this->NumberOfFieldDataInFile;
  }

  ///@{
  /**
   * What is the name of the ith attribute of a certain type
   * in this file? This requires reading the file, so the filename
   * must be set prior to invoking this operation.
   */
  const char* GetScalarsNameInFile(int i);
  const char* GetVectorsNameInFile(int i);
  const char* GetTensorsNameInFile(int i);
  const char* GetNormalsNameInFile(int i);
  const char* GetTCoordsNameInFile(int i);
  const char* GetFieldDataNameInFile(int i);
  ///@}

  ///@{
  /**
   * Set the name of the scalar data to extract. If not specified, first
   * scalar data encountered is extracted.
   */
  vtkSetStringMacro(ScalarsName);
  vtkGetStringMacro(ScalarsName);
  ///@}

  ///@{
  /**
   * Set the name of the vector data to extract. If not specified, first
   * vector data encountered is extracted.
   */
  vtkSetStringMacro(VectorsName);
  vtkGetStringMacro(VectorsName);
  ///@}

  ///@{
  /**
   * Set the name of the tensor data to extract. If not specified, first
   * tensor data encountered is extracted.
   */
  vtkSetStringMacro(TensorsName);
  vtkGetStringMacro(TensorsName);
  ///@}

  ///@{
  /**
   * Set the name of the normal data to extract. If not specified, first
   * normal data encountered is extracted.
   */
  vtkSetStringMacro(NormalsName);
  vtkGetStringMacro(NormalsName);
  ///@}

  ///@{
  /**
   * Set the name of the texture coordinate data to extract. If not specified,
   * first texture coordinate data encountered is extracted.
   */
  vtkSetStringMacro(TCoordsName);
  vtkGetStringMacro(TCoordsName);
  ///@}

  ///@{
  /**
   * Set the name of the lookup table data to extract. If not specified, uses
   * lookup table named by scalar. Otherwise, this specification supersedes.
   */
  vtkSetStringMacro(LookupTableName);
  vtkGetStringMacro(LookupTableName);
  ///@}

  ///@{
  /**
   * Set the name of the field data to extract. If not specified, uses
   * first field data encountered in file.
   */
  vtkSetStringMacro(FieldDataName);
  vtkGetStringMacro(FieldDataName);
  ///@}

  ///@{
  /**
   * Enable reading all scalars.
   */
  vtkSetMacro(ReadAllScalars, vtkTypeBool);
  vtkGetMacro(ReadAllScalars, vtkTypeBool);
  vtkBooleanMacro(ReadAllScalars, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Enable reading all vectors.
   */
  vtkSetMacro(ReadAllVectors, vtkTypeBool);
  vtkGetMacro(ReadAllVectors, vtkTypeBool);
  vtkBooleanMacro(ReadAllVectors, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Enable reading all normals.
   */
  vtkSetMacro(ReadAllNormals, vtkTypeBool);
  vtkGetMacro(ReadAllNormals, vtkTypeBool);
  vtkBooleanMacro(ReadAllNormals, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Enable reading all tensors.
   */
  vtkSetMacro(ReadAllTensors, vtkTypeBool);
  vtkGetMacro(ReadAllTensors, vtkTypeBool);
  vtkBooleanMacro(ReadAllTensors, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Enable reading all color scalars.
   */
  vtkSetMacro(ReadAllColorScalars, vtkTypeBool);
  vtkGetMacro(ReadAllColorScalars, vtkTypeBool);
  vtkBooleanMacro(ReadAllColorScalars, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Enable reading all tcoords.
   */
  vtkSetMacro(ReadAllTCoords, vtkTypeBool);
  vtkGetMacro(ReadAllTCoords, vtkTypeBool);
  vtkBooleanMacro(ReadAllTCoords, vtkTypeBool);
  ///@}

  ///@{
  /**
   * Enable reading all fields.
   */
  vtkSetMacro(ReadAllFields, vtkTypeBool);
  vtkGetMacro(ReadAllFields, vtkTypeBool);
  vtkBooleanMacro(ReadAllFields, vtkTypeBool);
  ///@}

  /**
   * Open a vtk data file. Returns zero if error.
   */
  int OpenVTKFile(VTK_FILEPATH const char* fname = nullptr);

  /**
   * Read the header of a vtk data file. Returns 0 if error.
   */
  int ReadHeader(VTK_FILEPATH const char* fname = nullptr);

  /**
   * Read the cell data of a vtk data file. The number of cells (from the
   * dataset) must match the number of cells defined in cell attributes (unless
   * no geometry was defined).
   */
  int ReadCellData(vtkDataSet* ds, vtkIdType numCells);

  /**
   * Read the point data of a vtk data file. The number of points (from the
   * dataset) must match the number of points defined in point attributes
   * (unless no geometry was defined).
   */
  int ReadPointData(vtkDataSet* ds, vtkIdType numPts);

  /**
   * Read point coordinates. Return 0 if error.
   */
  int ReadPointCoordinates(vtkPointSet* ps, vtkIdType numPts);

  /**
   * Read point coordinates. Return 0 if error.
   */
  int ReadPointCoordinates(vtkGraph* g, vtkIdType numPts);

  /**
   * Read the vertex data of a vtk data file. The number of vertices (from the
   * graph) must match the number of vertices defined in vertex attributes
   * (unless no geometry was defined).
   */
  int ReadVertexData(vtkGraph* g, vtkIdType numVertices);

  /**
   * Read the edge data of a vtk data file. The number of edges (from the
   * graph) must match the number of edges defined in edge attributes
   * (unless no geometry was defined).
   */
  int ReadEdgeData(vtkGraph* g, vtkIdType numEdges);

  /**
   * Read the row data of a vtk data file.
   */
  int ReadRowData(vtkTable* t, vtkIdType numEdges);

  /**
   * Read cells in a vtkCellArray, and update the smartpointer reference passed
   * in. If no cells are present in the file, cellArray will be set to nullptr.
   * Returns 0 if error.
   */
  int ReadCells(vtkSmartPointer<vtkCellArray>& cellArray);

  /**
   * Read a bunch of "cells". Return 0 if error.
   * @note Legacy implementation for file versions < 5.0.
   */
  int ReadCellsLegacy(vtkIdType size, int* data);

  /**
   * Read a piece of the cells (for streaming compliance)
   */
  int ReadCellsLegacy(vtkIdType size, int* data, int skip1, int read2, int skip3);

  /**
   * Read the coordinates for a rectilinear grid. The axes parameter specifies
   * which coordinate axes (0,1,2) is being read.
   */
  int ReadCoordinates(vtkRectilinearGrid* rg, int axes, int numCoords);

  ///@{
  /**
   * Helper functions for reading data.
   */
  vtkAbstractArray* ReadArray(const char* dataType, vtkIdType numTuples, vtkIdType numComp);
  vtkFieldData* ReadFieldData(FieldType fieldType = FIELD_DATA);
  ///@}

  ///@{
  /**
   * Internal function to read in a value.  Returns zero if there was an
   * error.
   */
  int Read(char*);
  int Read(unsigned char*);
  int Read(short*);
  int Read(unsigned short*);
  int Read(int*);
  int Read(unsigned int*);
  int Read(long*);
  int Read(unsigned long*);
  int Read(long long* result);
  int Read(unsigned long long* result);
  int Read(float*);
  int Read(double*);
  ///@}

  /**
   * Read @a n character from the stream into @a str, then reset the stream
   * position. Returns the number of characters actually read.
   */
  size_t Peek(char* str, size_t n);

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
  int ReadString(char (&result)[256]);

  /**
   * Helper method for reading in data.
   */
  char* LowerCase(char* str, size_t len = 256);

  /**
   * Return the istream being used to read in the data.
   */
  istream* GetIStream() { return this->IS; }

  ///@{
  /**
   * Overridden to handle reading from a string. The
   * superclass only knows about files.
   */
  int ReadTimeDependentMetaData(int timestep, vtkInformation* metadata) override;
  int ReadMesh(int piece, int npieces, int nghosts, int timestep, vtkDataObject* output) override;
  int ReadPoints(int /*piece*/, int /*npieces*/, int /*nghosts*/, int /*timestep*/,
    vtkDataObject* /*output*/) override
  {
    return 1;
  }
  int ReadArrays(int /*piece*/, int /*npieces*/, int /*nghosts*/, int /*timestep*/,
    vtkDataObject* /*output*/) override
  {
    return 1;
  }
  ///@}

  ///@{
  /**
   * Overridden with default implementation of doing nothing
   * so that subclasses only override what is needed (usually
   * only ReadMesh).
   */
  int ReadMeshSimple(VTK_FILEPATH const std::string& /*fname*/, vtkDataObject* /*output*/) override
  {
    return 1;
  }
  int ReadPointsSimple(
    VTK_FILEPATH const std::string& /*fname*/, vtkDataObject* /*output*/) override
  {
    return 1;
  }
  int ReadArraysSimple(
    VTK_FILEPATH const std::string& /*fname*/, vtkDataObject* /*output*/) override
  {
    return 1;
  }
  ///@}

protected:
  vtkDataReader();
  ~vtkDataReader() override;

  std::string CurrentFileName;
  int FileVersion;
  int FileMajorVersion;
  int FileMinorVersion;
  int FileType;
  istream* IS;

  char* ScalarsName;
  char* VectorsName;
  char* TensorsName;
  char* TCoordsName;
  char* NormalsName;
  char* LookupTableName;
  char* FieldDataName;
  char* ScalarLut;

  vtkTypeBool ReadFromInputString;
  char* InputString;
  int InputStringLength;
  int InputStringPos;

  void SetScalarLut(const char* lut);
  vtkGetStringMacro(ScalarLut);

  char* Header;

  int ReadScalarData(vtkDataSetAttributes* a, vtkIdType num);
  int ReadVectorData(vtkDataSetAttributes* a, vtkIdType num);
  int ReadNormalData(vtkDataSetAttributes* a, vtkIdType num);
  int ReadTensorData(vtkDataSetAttributes* a, vtkIdType num, vtkIdType numComp = 9);
  int ReadCoScalarData(vtkDataSetAttributes* a, vtkIdType num);
  int ReadLutData(vtkDataSetAttributes* a);
  int ReadTCoordsData(vtkDataSetAttributes* a, vtkIdType num);
  int ReadGlobalIds(vtkDataSetAttributes* a, vtkIdType num);
  int ReadPedigreeIds(vtkDataSetAttributes* a, vtkIdType num);
  int ReadEdgeFlags(vtkDataSetAttributes* a, vtkIdType num);

  /**
   * Format is detailed at
   * https://docs.vtk.org/en/latest/design_documents/IOLegacyInformationFormat.html
   */
  int ReadInformation(vtkInformation* info, vtkIdType numKeys);

  int ReadDataSetData(vtkDataSet* ds);

  // This supports getting additional information from vtk files
  int NumberOfScalarsInFile;
  char** ScalarsNameInFile;
  int ScalarsNameAllocSize;
  int NumberOfVectorsInFile;
  char** VectorsNameInFile;
  int VectorsNameAllocSize;
  int NumberOfTensorsInFile;
  char** TensorsNameInFile;
  int TensorsNameAllocSize;
  int NumberOfTCoordsInFile;
  char** TCoordsNameInFile;
  int TCoordsNameAllocSize;
  int NumberOfNormalsInFile;
  char** NormalsNameInFile;
  int NormalsNameAllocSize;
  int NumberOfFieldDataInFile;
  char** FieldDataNameInFile;
  int FieldDataNameAllocSize;
  vtkTimeStamp CharacteristicsTime;

  vtkTypeBool ReadAllScalars;
  vtkTypeBool ReadAllVectors;
  vtkTypeBool ReadAllNormals;
  vtkTypeBool ReadAllTensors;
  vtkTypeBool ReadAllColorScalars;
  vtkTypeBool ReadAllTCoords;
  vtkTypeBool ReadAllFields;

  std::locale CurrentLocale;

  void InitializeCharacteristics();
  int CharacterizeFile(); // read entire file, storing important characteristics
  void CheckFor(const char* name, char* line, int& num, char**& array, int& allocSize);

  vtkCharArray* InputArray;

  /**
   * Decode a string. This method is the inverse of
   * vtkWriter::EncodeString.  Returns the length of the
   * result string.
   */
  int DecodeString(char* resname, const char* name);

private:
  vtkDataReader(const vtkDataReader&) = delete;
  void operator=(const vtkDataReader&) = delete;

  void ConvertGhostLevelsToGhostType(FieldType fieldType, vtkAbstractArray* data) const;
};

VTK_ABI_NAMESPACE_END
#endif
