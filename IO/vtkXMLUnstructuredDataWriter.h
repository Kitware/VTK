/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLUnstructuredDataWriter.h
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
// .NAME vtkXMLUnstructuredDataWriter - Superclass for VTK XML unstructured data writers.
// .SECTION Description
// vtkXMLUnstructuredDataWriter provides VTK XML writing functionality
// that is common among all the unstructured data formats.

#ifndef __vtkXMLUnstructuredDataWriter_h
#define __vtkXMLUnstructuredDataWriter_h

#include "vtkXMLWriter.h"

class vtkPointSet;
class vtkCellArray;
class vtkDataArray;
class vtkIdTypeArray;

class VTK_IO_EXPORT vtkXMLUnstructuredDataWriter : public vtkXMLWriter
{
public:
  vtkTypeRevisionMacro(vtkXMLUnstructuredDataWriter,vtkXMLWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // Get/Set the number of pieces used to stream the image through the
  // pipeline while writing to the file.
  vtkSetMacro(NumberOfPieces, int);
  vtkGetMacro(NumberOfPieces, int);
  
  // Description: Get/Set the piece to write to the file.  If this
  // negative or equal to the NumberOfPieces, all pieces will be
  // written.
  vtkSetMacro(WritePiece, int);
  vtkGetMacro(WritePiece, int);

  // Description:
  // Get/Set the ghost level used to pad each piece.
  vtkSetMacro(GhostLevel, int);
  vtkGetMacro(GhostLevel, int);
  
protected:
  vtkXMLUnstructuredDataWriter();
  ~vtkXMLUnstructuredDataWriter();  
  
  vtkPointSet* GetInputAsPointSet();
  virtual const char* GetDataSetName()=0;
  virtual void SetInputUpdateExtent(int piece, int numPieces,
                                    int ghostLevel)=0;
  
  // The actual writing driver required by vtkXMLWriter.
  int WriteData();
  
  virtual int WriteInlineMode(vtkIndent indent);
  virtual void WriteInlinePieceAttributes();
  virtual void WriteInlinePiece(vtkIndent indent);
  
  virtual int WriteAppendedMode(vtkIndent indent);
  virtual void WriteAppendedPieceAttributes(int index);
  virtual void WriteAppendedPiece(int index, vtkIndent indent);
  virtual void WriteAppendedPieceData(int index);  
  
  void WriteCellsInline(const char* name, vtkCellArray* cells,
                        vtkDataArray* types, vtkIndent indent);
  unsigned long* WriteCellsAppended(const char* name, vtkDataArray* types,
                                    vtkIndent indent);
  void WriteCellsAppendedData(vtkCellArray* cells, vtkDataArray* types,
                              unsigned long* positions);
  void ConvertCells(vtkCellArray* cells);
  
  // Get the number of points/cells.  Valid after Update has been
  // invoked on the input.
  virtual vtkIdType GetNumberOfInputPoints();
  virtual vtkIdType GetNumberOfInputCells()=0;
  void CalculateDataFractions(float* fractions);
  void CalculateCellFractions(float* fractions, vtkIdType typesSize);
  
  // Number of pieces used for streaming.
  int NumberOfPieces;
  
  // Which piece to write, if not all.
  int WritePiece;
  
  // The ghost level on each piece.
  int GhostLevel;
  
  // Positions of attributes for each piece.
  unsigned long* PointsPositions;
  unsigned long* NumberOfPointsPositions;
  unsigned long** PointDataPositions;
  unsigned long** CellDataPositions;
  
  // Hold the new cell representation arrays while writing a piece.
  vtkIdTypeArray* CellPoints;
  vtkIdTypeArray* CellOffsets;
  
private:
  vtkXMLUnstructuredDataWriter(const vtkXMLUnstructuredDataWriter&);  // Not implemented.
  void operator=(const vtkXMLUnstructuredDataWriter&);  // Not implemented.
};

#endif
