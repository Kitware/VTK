/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLUnstructuredDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
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

#ifndef vtkXMLUnstructuredDataWriter_h
#define vtkXMLUnstructuredDataWriter_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLWriter.h"

class vtkPointSet;
class vtkCellArray;
class vtkCellIterator;
class vtkDataArray;
class vtkIdTypeArray;
class vtkUnstructuredGrid;

class VTKIOXML_EXPORT vtkXMLUnstructuredDataWriter : public vtkXMLWriter
{
public:
  vtkTypeMacro(vtkXMLUnstructuredDataWriter,vtkXMLWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the number of pieces used to stream the image through the
  // pipeline while writing to the file.
  vtkSetMacro(NumberOfPieces, int);
  vtkGetMacro(NumberOfPieces, int);

  // Description:
  // Get/Set the piece to write to the file.  If this is
  // negative or equal to the NumberOfPieces, all pieces will be written.
  vtkSetMacro(WritePiece, int);
  vtkGetMacro(WritePiece, int);

  // Description:
  // Get/Set the ghost level used to pad each piece.
  vtkSetMacro(GhostLevel, int);
  vtkGetMacro(GhostLevel, int);

  // See the vtkAlgorithm for a desciption of what these do
  int ProcessRequest(vtkInformation*,
                     vtkInformationVector**,
                     vtkInformationVector*);

protected:
  vtkXMLUnstructuredDataWriter();
  ~vtkXMLUnstructuredDataWriter();

  vtkPointSet* GetInputAsPointSet();
  virtual const char* GetDataSetName()=0;
  virtual void SetInputUpdateExtent(int piece, int numPieces,
                                    int ghostLevel);

  virtual int WriteHeader();
  virtual int WriteAPiece();
  virtual int WriteFooter();

  virtual void AllocatePositionArrays();
  virtual void DeletePositionArrays();

  virtual int WriteInlineMode(vtkIndent indent);
  virtual void WriteInlinePieceAttributes();
  virtual void WriteInlinePiece(vtkIndent indent);

  virtual void WriteAppendedPieceAttributes(int index);
  virtual void WriteAppendedPiece(int index, vtkIndent indent);
  virtual void WriteAppendedPieceData(int index);

  void WriteCellsInline(const char* name, vtkCellIterator *cellIter,
                        vtkIdType numCells, vtkIdType cellSizeEstimate,
                        vtkIndent indent);

  void WriteCellsInline(const char* name, vtkCellArray* cells,
                        vtkDataArray* types, vtkIndent indent);

  // New API with face infomration for polyhedron cell support.
  void WriteCellsInline(const char* name, vtkCellArray* cells,
                        vtkDataArray* types, vtkIdTypeArray* faces,
                        vtkIdTypeArray* faceOffsets, vtkIndent indent);

  void WriteCellsInlineWorker(const char* name, vtkDataArray *types,
                              vtkIndent indent);

  void WriteCellsAppended(const char* name, vtkDataArray* types,
                          vtkIndent indent, OffsetsManagerGroup *cellsManager);

  void WriteCellsAppended(const char* name, vtkCellIterator *cellIter,
                          vtkIdType numCells, vtkIndent indent,
                          OffsetsManagerGroup *cellsManager);

  void WriteCellsAppendedData(vtkCellArray* cells, vtkDataArray* types,
                              int timestep, OffsetsManagerGroup *cellsManager);

  void WriteCellsAppendedData(vtkCellIterator* cellIter, vtkIdType numCells,
                              vtkIdType cellSizeEstimate, int timestep,
                              OffsetsManagerGroup *cellsManager);

  // New API with face infomration for polyhedron cell support.
  void WriteCellsAppendedData(vtkCellArray* cells, vtkDataArray* types,
                              vtkIdTypeArray* faces,vtkIdTypeArray* faceOffsets,
                              int timestep, OffsetsManagerGroup *cellsManager);

  void WriteCellsAppendedDataWorker(vtkDataArray* types, int timestep,
                                    OffsetsManagerGroup *cellsManager);

  void ConvertCells(vtkCellIterator* cellIter, vtkIdType numCells,
                    vtkIdType cellSizeEstimate);

  void ConvertCells(vtkCellArray* cells);

  // For polyhedron support, conversion results are stored in Faces and FaceOffsets
  void ConvertFaces(vtkIdTypeArray* faces, vtkIdTypeArray* faceOffsets);

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
  vtkTypeInt64* NumberOfPointsPositions;

  // For TimeStep support
  OffsetsManagerGroup *PointsOM;
  OffsetsManagerArray *PointDataOM;
  OffsetsManagerArray *CellDataOM;

  // Hold the new cell representation arrays while writing a piece.
  vtkIdTypeArray* CellPoints;
  vtkIdTypeArray* CellOffsets;

  int CurrentPiece;

  // Hold the face arrays for polyhedron cells.
  vtkIdTypeArray* Faces;
  vtkIdTypeArray* FaceOffsets;

private:
  vtkXMLUnstructuredDataWriter(const vtkXMLUnstructuredDataWriter&);  // Not implemented.
  void operator=(const vtkXMLUnstructuredDataWriter&);  // Not implemented.
};

#endif
