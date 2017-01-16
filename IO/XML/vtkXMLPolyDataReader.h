/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPolyDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
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

class vtkPolyData;

class VTKIOXML_EXPORT vtkXMLPolyDataReader : public vtkXMLUnstructuredDataReader
{
public:
  vtkTypeMacro(vtkXMLPolyDataReader,vtkXMLUnstructuredDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkXMLPolyDataReader *New();

  //@{
  /**
   * Get the reader's output.
   */
  vtkPolyData *GetOutput();
  vtkPolyData *GetOutput(int idx);
  //@}

  //@{
  /**
   * Get the number of verts/lines/strips/polys in the output.
   */
  virtual vtkIdType GetNumberOfVerts();
  virtual vtkIdType GetNumberOfLines();
  virtual vtkIdType GetNumberOfStrips();
  virtual vtkIdType GetNumberOfPolys();
  //@}

protected:
  vtkXMLPolyDataReader();
  ~vtkXMLPolyDataReader() VTK_OVERRIDE;

  const char* GetDataSetName() VTK_OVERRIDE;
  void GetOutputUpdateExtent(int& piece, int& numberOfPieces, int& ghostLevel) VTK_OVERRIDE;
  void SetupOutputTotals() VTK_OVERRIDE;
  void SetupNextPiece() VTK_OVERRIDE;
  void SetupPieces(int numPieces) VTK_OVERRIDE;
  void DestroyPieces() VTK_OVERRIDE;

  void SetupOutputData() VTK_OVERRIDE;
  int ReadPiece(vtkXMLDataElement* ePiece) VTK_OVERRIDE;
  int ReadPieceData() VTK_OVERRIDE;

  // Read a data array whose tuples coorrespond to cells.
  int ReadArrayForCells(vtkXMLDataElement* da,
    vtkAbstractArray* outArray) VTK_OVERRIDE;

  // Get the number of cells in the given piece.  Valid after
  // UpdateInformation.
  vtkIdType GetNumberOfCellsInPiece(int piece) VTK_OVERRIDE;

  int FillOutputPortInformation(int, vtkInformation*) VTK_OVERRIDE;

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
  vtkXMLPolyDataReader(const vtkXMLPolyDataReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLPolyDataReader&) VTK_DELETE_FUNCTION;
};

#endif
