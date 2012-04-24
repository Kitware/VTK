/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPPolyDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLPPolyDataReader - Read PVTK XML PolyData files.
// .SECTION Description
// vtkXMLPPolyDataReader reads the PVTK XML PolyData file format.
// This reads the parallel format's summary file and then uses
// vtkXMLPolyDataReader to read data from the individual PolyData
// piece files.  Streaming is supported.  The standard extension for
// this reader's file format is "pvtp".

// .SECTION See Also
// vtkXMLPolyDataReader

#ifndef __vtkXMLPPolyDataReader_h
#define __vtkXMLPPolyDataReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLPUnstructuredDataReader.h"

class vtkPolyData;

class VTKIOXML_EXPORT vtkXMLPPolyDataReader : public vtkXMLPUnstructuredDataReader
{
public:
  vtkTypeMacro(vtkXMLPPolyDataReader,vtkXMLPUnstructuredDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkXMLPPolyDataReader *New();

  // Description:
  // Get the reader's output.
  vtkPolyData *GetOutput();
  vtkPolyData *GetOutput(int idx);

protected:
  vtkXMLPPolyDataReader();
  ~vtkXMLPPolyDataReader();

  const char* GetDataSetName();
  void GetOutputUpdateExtent(int& piece, int& numberOfPieces, int& ghostLevel);
  vtkIdType GetNumberOfCellsInPiece(int piece);
  vtkIdType GetNumberOfVertsInPiece(int piece);
  vtkIdType GetNumberOfLinesInPiece(int piece);
  vtkIdType GetNumberOfStripsInPiece(int piece);
  vtkIdType GetNumberOfPolysInPiece(int piece);
  void SetupOutputTotals();

  void SetupOutputData();
  void SetupNextPiece();
  int ReadPieceData();

  void CopyArrayForCells(vtkDataArray* inArray, vtkDataArray* outArray);
  vtkXMLDataReader* CreatePieceReader();
  virtual int FillOutputPortInformation(int, vtkInformation*);

  // The size of the UpdatePiece.
  vtkIdType TotalNumberOfVerts;
  vtkIdType TotalNumberOfLines;
  vtkIdType TotalNumberOfStrips;
  vtkIdType TotalNumberOfPolys;
  vtkIdType StartVert;
  vtkIdType StartLine;
  vtkIdType StartStrip;
  vtkIdType StartPoly;

private:
  vtkXMLPPolyDataReader(const vtkXMLPPolyDataReader&);  // Not implemented.
  void operator=(const vtkXMLPPolyDataReader&);  // Not implemented.
};

#endif
