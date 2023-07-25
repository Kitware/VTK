// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkXMLPolyDataReader
 * @brief   Read VTK XML PolyData files.
 *
 * vtkXMLPolyDataReader reads the VTK XML PolyData file format.  One
 * polygonal data file can be read to produce one output.  Streaming
 * is supported.  The standard extension for this reader's file format
 * is "vtp".  This reader is also used to read a single piece of the
 * parallel file format.
 *
 * @sa
 * vtkXMLPPolyDataReader
 */

#ifndef vtkXMLPolyDataReader_h
#define vtkXMLPolyDataReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLUnstructuredDataReader.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkPolyData;

class VTKIOXML_EXPORT vtkXMLPolyDataReader : public vtkXMLUnstructuredDataReader
{
public:
  vtkTypeMacro(vtkXMLPolyDataReader, vtkXMLUnstructuredDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkXMLPolyDataReader* New();

  ///@{
  /**
   * Get the reader's output.
   */
  vtkPolyData* GetOutput();
  vtkPolyData* GetOutput(int idx);
  ///@}

  ///@{
  /**
   * Get the number of verts/lines/strips/polys in the output.
   */
  virtual vtkIdType GetNumberOfVerts();
  virtual vtkIdType GetNumberOfLines();
  virtual vtkIdType GetNumberOfStrips();
  virtual vtkIdType GetNumberOfPolys();
  ///@}

protected:
  vtkXMLPolyDataReader();
  ~vtkXMLPolyDataReader() override;

  const char* GetDataSetName() override;
  void GetOutputUpdateExtent(int& piece, int& numberOfPieces, int& ghostLevel) override;
  void SetupOutputTotals() override;
  void SetupNextPiece() override;
  void SetupPieces(int numPieces) override;
  void DestroyPieces() override;

  void SetupOutputData() override;
  int ReadPiece(vtkXMLDataElement* ePiece) override;
  int ReadPieceData() override;

  // Read a data array whose tuples coorrespond to cells.
  int ReadArrayForCells(vtkXMLDataElement* da, vtkAbstractArray* outArray) override;

  // Get the number of cells in the given piece.  Valid after
  // UpdateInformation.
  vtkIdType GetNumberOfCellsInPiece(int piece) override;

  int FillOutputPortInformation(int, vtkInformation*) override;

  // The size of the UpdatePiece.
  int TotalNumberOfVerts;
  int TotalNumberOfLines;
  int TotalNumberOfStrips;
  int TotalNumberOfPolys;
  vtkIdType StartVert;
  vtkIdType StartLine;
  vtkIdType StartStrip;
  vtkIdType StartPoly;

  // The cell elements for each piece.
  vtkXMLDataElement** VertElements;
  vtkXMLDataElement** LineElements;
  vtkXMLDataElement** StripElements;
  vtkXMLDataElement** PolyElements;
  vtkIdType* NumberOfVerts;
  vtkIdType* NumberOfLines;
  vtkIdType* NumberOfStrips;
  vtkIdType* NumberOfPolys;

  // For TimeStep support
  int VertsTimeStep;
  unsigned long VertsOffset;
  int LinesTimeStep;
  unsigned long LinesOffset;
  int StripsTimeStep;
  unsigned long StripsOffset;
  int PolysTimeStep;
  unsigned long PolysOffset;

private:
  vtkXMLPolyDataReader(const vtkXMLPolyDataReader&) = delete;
  void operator=(const vtkXMLPolyDataReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
