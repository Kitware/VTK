/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkDataWriter
 * @brief   helper class for objects that write vtk data files
 *
 * vtkDataWriter is a helper class that opens and writes the vtk header and
 * point data (e.g., scalars, vectors, normals, etc.) from a vtk data file.
 * See text for various formats.
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
  vtkTypeMacro(vtkDataWriter,vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Created object with default header, ASCII format, and default names for
   * scalars, vectors, tensors, normals, and texture coordinates.
   */
  static vtkDataWriter *New();

  //@{
  /**
   * Specify file name of vtk polygon data file to write.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Enable writing to an OutputString instead of the default, a file.
   */
  vtkSetMacro(WriteToOutputString,int);
  vtkGetMacro(WriteToOutputString,int);
  vtkBooleanMacro(WriteToOutputString,int);
  //@}

  //@{
  /**
   * When WriteToOutputString in on, then a string is allocated, written to,
   * and can be retrieved with these methods.  The string is deleted during
   * the next call to write ...
   */
  vtkGetMacro(OutputStringLength, int);
  vtkGetStringMacro(OutputString);
  unsigned char *GetBinaryOutputString()
  {
      return reinterpret_cast<unsigned char *>(this->OutputString);
  }
  //@}

  /**
   * When WriteToOutputString is on, this method returns a copy of the
   * output string in a vtkStdString.
   */
  vtkStdString GetOutputStdString();

  /**
   * This convenience method returns the string, sets the IVAR to NULL,
   * so that the user is responsible for deleting the string.
   * I am not sure what the name should be, so it may change in the future.
   */
  char *RegisterAndGetOutputString();

  //@{
  /**
   * Specify the header for the vtk data file.
   */
  vtkSetStringMacro(Header);
  vtkGetStringMacro(Header);
  //@}

  //@{
  /**
   * If true, vtkInformation objects attached to arrays and array component
   * nameswill be written to the output. Default is true.
   */
  vtkSetMacro(WriteArrayMetaData, bool)
  vtkGetMacro(WriteArrayMetaData, bool)
  vtkBooleanMacro(WriteArrayMetaData, bool)
  //@}

  //@{
  /**
   * Specify file type (ASCII or BINARY) for vtk data file.
   */
  vtkSetClampMacro(FileType,int,VTK_ASCII,VTK_BINARY);
  vtkGetMacro(FileType,int);
  void SetFileTypeToASCII() {this->SetFileType(VTK_ASCII);};
  void SetFileTypeToBinary() {this->SetFileType(VTK_BINARY);};
  //@}

  //@{
  /**
   * Give a name to the scalar data. If not specified, uses default
   * name "scalars".
   */
  vtkSetStringMacro(ScalarsName);
  vtkGetStringMacro(ScalarsName);
  //@}

  //@{
  /**
   * Give a name to the vector data. If not specified, uses default
   * name "vectors".
   */
  vtkSetStringMacro(VectorsName);
  vtkGetStringMacro(VectorsName);
  //@}

  //@{
  /**
   * Give a name to the tensors data. If not specified, uses default
   * name "tensors".
   */
  vtkSetStringMacro(TensorsName);
  vtkGetStringMacro(TensorsName);
  //@}

  //@{
  /**
   * Give a name to the normals data. If not specified, uses default
   * name "normals".
   */
  vtkSetStringMacro(NormalsName);
  vtkGetStringMacro(NormalsName);
  //@}

  //@{
  /**
   * Give a name to the texture coordinates data. If not specified, uses
   * default name "textureCoords".
   */
  vtkSetStringMacro(TCoordsName);
  vtkGetStringMacro(TCoordsName);
  //@}

  //@{
  /**
   * Give a name to the global ids data. If not specified, uses
   * default name "global_ids".
   */
  vtkSetStringMacro(GlobalIdsName);
  vtkGetStringMacro(GlobalIdsName);
  //@}

  //@{
  /**
   * Give a name to the pedigree ids data. If not specified, uses
   * default name "pedigree_ids".
   */
  vtkSetStringMacro(PedigreeIdsName);
  vtkGetStringMacro(PedigreeIdsName);
  //@}

  //@{
  /**
   * Give a name to the edge flags data. If not specified, uses
   * default name "edge_flags".
   */
  vtkSetStringMacro(EdgeFlagsName);
  vtkGetStringMacro(EdgeFlagsName);
  //@}

  //@{
  /**
   * Give a name to the lookup table. If not specified, uses default
   * name "lookupTable".
   */
  vtkSetStringMacro(LookupTableName);
  vtkGetStringMacro(LookupTableName);
  //@}

  //@{
  /**
   * Give a name to the field data. If not specified, uses default
   * name "field".
   */
  vtkSetStringMacro(FieldDataName);
  vtkGetStringMacro(FieldDataName);
  //@}

  /**
   * Open a vtk data file. Returns NULL if error.
   */
  virtual ostream *OpenVTKFile();

  /**
   * Write the header of a vtk data file. Returns 0 if error.
   */
  int WriteHeader(ostream *fp);

  /**
   * Write out the points of the data set.
   */
  int WritePoints(ostream *fp, vtkPoints *p);

  /**
   * Write out coordinates for rectilinear grids.
   */
  int WriteCoordinates(ostream *fp, vtkDataArray *coords, int axes);

  /**
   * Write out the cells of the data set.
   */
  int WriteCells(ostream *fp, vtkCellArray *cells, const char *label);

  /**
   * Write the cell data (e.g., scalars, vectors, ...) of a vtk dataset.
   * Returns 0 if error.
   */
  int WriteCellData(ostream *fp, vtkDataSet *ds);

  /**
   * Write the point data (e.g., scalars, vectors, ...) of a vtk dataset.
   * Returns 0 if error.
   */
  int WritePointData(ostream *fp, vtkDataSet *ds);

  /**
   * Write the edge data (e.g., scalars, vectors, ...) of a vtk graph.
   * Returns 0 if error.
   */
  int WriteEdgeData(ostream *fp, vtkGraph *g);

  /**
   * Write the vertex data (e.g., scalars, vectors, ...) of a vtk graph.
   * Returns 0 if error.
   */
  int WriteVertexData(ostream *fp, vtkGraph *g);

  /**
   * Write the row data (e.g., scalars, vectors, ...) of a vtk table.
   * Returns 0 if error.
   */
  int WriteRowData(ostream *fp, vtkTable *g);

  /**
   * Write out the field data.
   */
  int WriteFieldData(ostream *fp, vtkFieldData *f);

  /**
   * Write out the data associated with the dataset (i.e. field data owned by
   * the dataset itself - distinct from that owned by the cells or points).
   */
  int WriteDataSetData(ostream *fp, vtkDataSet *ds);

  /**
   * Close a vtk file.
   */
  void CloseVTKFile(ostream *fp);


protected:
  vtkDataWriter();
  ~vtkDataWriter();

  int WriteToOutputString;
  char *OutputString;
  int OutputStringLength;

  void WriteData(); //dummy method to allow this class to be instantiated and delegated to

  char *FileName;
  char *Header;
  int FileType;

  bool WriteArrayMetaData;

  char *ScalarsName;
  char *VectorsName;
  char *TensorsName;
  char *TCoordsName;
  char *NormalsName;
  char *LookupTableName;
  char *FieldDataName;
  char* GlobalIdsName;
  char* PedigreeIdsName;
  char* EdgeFlagsName;

  int WriteArray(ostream *fp, int dataType, vtkAbstractArray *data, const char *format,
                 int num, int numComp);
  int WriteScalarData(ostream *fp, vtkDataArray *s, int num);
  int WriteVectorData(ostream *fp, vtkDataArray *v, int num);
  int WriteNormalData(ostream *fp, vtkDataArray *n, int num);
  int WriteTCoordData(ostream *fp, vtkDataArray *tc, int num);
  int WriteTensorData(ostream *fp, vtkDataArray *t, int num);
  int WriteGlobalIdData(ostream *fp, vtkDataArray *g, int num);
  int WritePedigreeIdData(ostream *fp, vtkAbstractArray *p, int num);
  int WriteEdgeFlagsData(ostream *fp, vtkDataArray *edgeFlags, int num);

  bool CanWriteInformationKey(vtkInformation *info, vtkInformationKey *key);

  /**
   * Format is detailed \ref IOLegacyInformationFormat "here".
   */
  int WriteInformation(ostream *fp, vtkInformation *info);

private:
  vtkDataWriter(const vtkDataWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDataWriter&) VTK_DELETE_FUNCTION;
};

#endif
