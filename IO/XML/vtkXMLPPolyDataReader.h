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
/**
 * @class   vtkXMLPPolyDataReader
 * @brief   Read PVTK XML PolyData files.
 *
 * vtkXMLPPolyDataReader reads the PVTK XML PolyData file format.
 * This reads the parallel format's summary file and then uses
 * vtkXMLPolyDataReader to read data from the individual PolyData
 * piece files.  Streaming is supported.  The standard extension for
 * this reader's file format is "pvtp".
 *
 * @sa
 * vtkXMLPolyDataReader
*/

#ifndef vtkXMLPPolyDataReader_h
#define vtkXMLPPolyDataReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLPUnstructuredDataReader.h"

class vtkPolyData;

class VTKIOXML_EXPORT vtkXMLPPolyDataReader : public vtkXMLPUnstructuredDataReader
{
public:
  vtkTypeMacro(vtkXMLPPolyDataReader,vtkXMLPUnstructuredDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkXMLPPolyDataReader *New();

  //@{
  /**
   * Get the reader's output.
   */
  vtkPolyData *GetOutput();
  vtkPolyData *GetOutput(int idx);
  //@}

protected:
  vtkXMLPPolyDataReader();
  ~vtkXMLPPolyDataReader() VTK_OVERRIDE;

  const char* GetDataSetName() VTK_OVERRIDE;
  void GetOutputUpdateExtent(int& piece, int& numberOfPieces, int& ghostLevel) VTK_OVERRIDE;
  vtkIdType GetNumberOfCellsInPiece(int piece) VTK_OVERRIDE;
  vtkIdType GetNumberOfVertsInPiece(int piece);
  vtkIdType GetNumberOfLinesInPiece(int piece);
  vtkIdType GetNumberOfStripsInPiece(int piece);
  vtkIdType GetNumberOfPolysInPiece(int piece);
  void SetupOutputTotals() VTK_OVERRIDE;

  void SetupOutputData() VTK_OVERRIDE;
  void SetupNextPiece() VTK_OVERRIDE;
  int ReadPieceData() VTK_OVERRIDE;

  void CopyArrayForCells(vtkDataArray* inArray, vtkDataArray* outArray) VTK_OVERRIDE;
  vtkXMLDataReader* CreatePieceReader() VTK_OVERRIDE;
  int FillOutputPortInformation(int, vtkInformation*) VTK_OVERRIDE;

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
  vtkXMLPPolyDataReader(const vtkXMLPPolyDataReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLPPolyDataReader&) VTK_DELETE_FUNCTION;
};

#endif
