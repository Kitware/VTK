// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkDataWriter
 * @brief   helper class for objects that write VTK data files
 *
 * vtkDataWriter is a helper class that opens and writes the VTK header and
 * point data (e.g., scalars, vectors, normals, etc.) from a vtk data file.
 * See the VTK textbook and online resources for various formats.
 *
 * @sa
 * vtkDataSetWriter vtkPolyDataWriter vtkStructuredGridWriter
 * vtkStructuredPointsWriter vtkUnstructuredGridWriter
 * vtkFieldDataWriter vtkRectilinearGridWriter
 */

#ifndef vtkDataWriter_h
#define vtkDataWriter_h

#include "vtkIOLegacyModule.h" // For export macro
#include "vtkWriter.h"

#include <locale> // For locale settings

VTK_ABI_NAMESPACE_BEGIN
class vtkCellArray;
class vtkDataArray;
class vtkDataSet;
class vtkFieldData;
class vtkGraph;
class vtkInformation;
class vtkInformationKey;
class vtkPoints;
class vtkTable;

class VTKIOLEGACY_EXPORT vtkDataWriter : public vtkWriter
{
public:
  ///@{
  /**
   * Standard methods for type information and printing.
   */
  vtkTypeMacro(vtkDataWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  ///@}

  /**
   * Create object with default header, ASCII format, and default names for
   * scalars, vectors, tensors, normals, and texture coordinates.
   */
  static vtkDataWriter* New();

  ///@{
  /**
   * Specify the file name of VTK data file to write.
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  // Currently VTK can write out two different versions of file format: files
  // of VTK reader version 4.2 and previous; and VTK reader version 5.1 and
  // later. This will likely change in the future. (Note: the major
  // difference in the two formats is the way cell arrays are written out.)
  // By default, Version 5.1 files are written out.
  enum VTKFileVersion
  {
    VTK_LEGACY_READER_VERSION_4_2 = 42,
    VTK_LEGACY_READER_VERSION_5_1 = 51
  };

  ///@{
  /**
   * Specify the VTK file version to write. See the enum documentation above
   * (VTKFileVersion) for additional information about supported versions.
   * It is possible to get the file major and minor versions separately.  See
   * also vtkDataReader for related methods. (Note, the parsing of the
   * FileVersion into major and minor version is as follows: the least
   * significant digit is the minor version; the remaining digits are the
   * major version.
   */
  void SetFileVersion(int);
  vtkGetMacro(FileVersion, int);
  vtkGetMacro(FileMajorVersion, int);
  vtkGetMacro(FileMinorVersion, int);
  ///@}

  ///@{
  /**
   * Enable writing to an OutputString instead of the default, a file.
   */
  vtkSetMacro(WriteToOutputString, vtkTypeBool);
  vtkGetMacro(WriteToOutputString, vtkTypeBool);
  vtkBooleanMacro(WriteToOutputString, vtkTypeBool);
  ///@}

  ///@{
  /**
   * When WriteToOutputString in on, then a string is allocated, written to,
   * and can be retrieved with these methods.  The string is deleted during
   * the next call to write ...
   */
  vtkGetMacro(OutputStringLength, vtkIdType);
  vtkGetStringMacro(OutputString);
  unsigned char* GetBinaryOutputString()
  {
    return reinterpret_cast<unsigned char*>(this->OutputString);
  }
  ///@}

  /**
   * When WriteToOutputString is on, this method returns a copy of the
   * output string in a vtkStdString.
   */
  vtkStdString GetOutputStdString();

  /**
   * This convenience method returns the string, sets the IVAR to nullptr,
   * so that the user is responsible for deleting the string.
   * I am not sure what the name should be, so it may change in the future.
   */
  char* RegisterAndGetOutputString();

  ///@{
  /**
   * Specify the header for the VTK data file.
   */
  vtkSetStringMacro(Header);
  vtkGetStringMacro(Header);
  ///@}

  ///@{
  /**
   * If true, vtkInformation objects attached to arrays and array component
   * nameswill be written to the output. The default is true.
   */
  vtkSetMacro(WriteArrayMetaData, bool);
  vtkGetMacro(WriteArrayMetaData, bool);
  vtkBooleanMacro(WriteArrayMetaData, bool);
  ///@}

  ///@{
  /**
   * Specify the file type (ASCII or BINARY) of the VTK data file.
   */
  vtkSetClampMacro(FileType, int, VTK_ASCII, VTK_BINARY);
  vtkGetMacro(FileType, int);
  void SetFileTypeToASCII() { this->SetFileType(VTK_ASCII); }
  void SetFileTypeToBinary() { this->SetFileType(VTK_BINARY); }
  ///@}

  ///@{
  /**
   * Give a name to the scalar data. If not specified, uses default
   * name "scalars".
   */
  vtkSetStringMacro(ScalarsName);
  vtkGetStringMacro(ScalarsName);
  ///@}

  ///@{
  /**
   * Give a name to the vector data. If not specified, uses default
   * name "vectors".
   */
  vtkSetStringMacro(VectorsName);
  vtkGetStringMacro(VectorsName);
  ///@}

  ///@{
  /**
   * Give a name to the tensors data. If not specified, uses default
   * name "tensors".
   */
  vtkSetStringMacro(TensorsName);
  vtkGetStringMacro(TensorsName);
  ///@}

  ///@{
  /**
   * Give a name to the normals data. If not specified, uses default
   * name "normals".
   */
  vtkSetStringMacro(NormalsName);
  vtkGetStringMacro(NormalsName);
  ///@}

  ///@{
  /**
   * Give a name to the texture coordinates data. If not specified, uses
   * default name "textureCoords".
   */
  vtkSetStringMacro(TCoordsName);
  vtkGetStringMacro(TCoordsName);
  ///@}

  ///@{
  /**
   * Give a name to the global ids data. If not specified, uses
   * default name "global_ids".
   */
  vtkSetStringMacro(GlobalIdsName);
  vtkGetStringMacro(GlobalIdsName);
  ///@}

  ///@{
  /**
   * Give a name to the pedigree ids data. If not specified, uses
   * default name "pedigree_ids".
   */
  vtkSetStringMacro(PedigreeIdsName);
  vtkGetStringMacro(PedigreeIdsName);
  ///@}

  ///@{
  /**
   * Give a name to the edge flags data. If not specified, uses
   * default name "edge_flags".
   */
  vtkSetStringMacro(EdgeFlagsName);
  vtkGetStringMacro(EdgeFlagsName);
  ///@}

  ///@{
  /**
   * Give a name to the lookup table. If not specified, uses default
   * name "lookupTable".
   */
  vtkSetStringMacro(LookupTableName);
  vtkGetStringMacro(LookupTableName);
  ///@}

  ///@{
  /**
   * Give a name to the field data. If not specified, uses default
   * name "field".
   */
  vtkSetStringMacro(FieldDataName);
  vtkGetStringMacro(FieldDataName);
  ///@}

  /**
   * Open a vtk data file. Returns nullptr if error.
   */
  virtual ostream* OpenVTKFile();

  /**
   * Write the header of a vtk data file. Returns 0 if error.
   */
  int WriteHeader(ostream* fp);

  /**
   * Write out the points of the data set.
   */
  int WritePoints(ostream* fp, vtkPoints* p);

  /**
   * Write out coordinates for rectilinear grids.
   */
  int WriteCoordinates(ostream* fp, vtkDataArray* coords, int axes);

  /**
   * Write out the cells of the data set.
   */
  int WriteCells(ostream* fp, vtkCellArray* cells, const char* label);

  /**
   * Write out the cells of the data set.
   * @note Legacy implementation for file version < 5.0.
   */
  int WriteCellsLegacy(ostream* fp, vtkCellArray* cells, const char* label);

  /**
   * Write the cell data (e.g., scalars, vectors, ...) of a vtk dataset.
   * Returns 0 if error.
   */
  int WriteCellData(ostream* fp, vtkDataSet* ds);

  /**
   * Write the point data (e.g., scalars, vectors, ...) of a vtk dataset.
   * Returns 0 if error.
   */
  int WritePointData(ostream* fp, vtkDataSet* ds);

  /**
   * Write the edge data (e.g., scalars, vectors, ...) of a vtk graph.
   * Returns 0 if error.
   */
  int WriteEdgeData(ostream* fp, vtkGraph* g);

  /**
   * Write the vertex data (e.g., scalars, vectors, ...) of a vtk graph.
   * Returns 0 if error.
   */
  int WriteVertexData(ostream* fp, vtkGraph* g);

  /**
   * Write the row data (e.g., scalars, vectors, ...) of a vtk table.
   * Returns 0 if error.
   */
  int WriteRowData(ostream* fp, vtkTable* g);

  /**
   * Write out the field data.
   */
  int WriteFieldData(ostream* fp, vtkFieldData* f);

  /**
   * Write out the data associated with the dataset (i.e. field data owned by
   * the dataset itself - distinct from that owned by the cells or points).
   */
  int WriteDataSetData(ostream* fp, vtkDataSet* ds);

  /**
   * Close a vtk file.
   */
  void CloseVTKFile(ostream* fp);

protected:
  vtkDataWriter();
  ~vtkDataWriter() override;

  vtkTypeBool WriteToOutputString;
  char* OutputString;
  vtkIdType OutputStringLength;

  void WriteData() override; // dummy method to allow this class to be instantiated and delegated to

  char* FileName;
  int FileVersion;
  int FileMajorVersion;
  int FileMinorVersion;
  char* Header;
  int FileType;

  bool WriteArrayMetaData;

  char* ScalarsName;
  char* VectorsName;
  char* TensorsName;
  char* TCoordsName;
  char* NormalsName;
  char* LookupTableName;
  char* FieldDataName;
  char* GlobalIdsName;
  char* PedigreeIdsName;
  char* EdgeFlagsName;

  std::locale CurrentLocale;

  int WriteArray(ostream* fp, int dataType, vtkAbstractArray* data, const char* format,
    vtkIdType num, vtkIdType numComp);
  int WriteScalarData(ostream* fp, vtkDataArray* s, vtkIdType num);
  int WriteVectorData(ostream* fp, vtkDataArray* v, vtkIdType num);
  int WriteNormalData(ostream* fp, vtkDataArray* n, vtkIdType num);
  int WriteTCoordData(ostream* fp, vtkDataArray* tc, vtkIdType num);
  int WriteTensorData(ostream* fp, vtkDataArray* t, vtkIdType num);
  int WriteGlobalIdData(ostream* fp, vtkDataArray* g, vtkIdType num);
  int WritePedigreeIdData(ostream* fp, vtkAbstractArray* p, vtkIdType num);
  int WriteEdgeFlagsData(ostream* fp, vtkDataArray* edgeFlags, vtkIdType num);

  bool CanWriteInformationKey(vtkInformation* info, vtkInformationKey* key);

  /**
   * Format is detailed at
   * https://docs.vtk.org/en/latest/design_documents/IOLegacyInformationFormat.html
   */
  int WriteInformation(ostream* fp, vtkInformation* info);

private:
  vtkDataWriter(const vtkDataWriter&) = delete;
  void operator=(const vtkDataWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
