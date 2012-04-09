/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLPDataReader - Superclass for PVTK XML file readers.
// .SECTION Description
// vtkXMLPDataReader provides functionality common to all PVTK XML
// file readers.  Concrete subclasses call upon this functionality
// when needed.

// .SECTION See Also
// vtkXMLDataReader

#ifndef __vtkXMLPDataReader_h
#define __vtkXMLPDataReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLReader.h"

class vtkDataArray;
class vtkDataSet;
class vtkXMLDataReader;

class VTKIOXML_EXPORT vtkXMLPDataReader : public vtkXMLReader
{
public:
  vtkTypeMacro(vtkXMLPDataReader,vtkXMLReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get the number of pieces from the summary file being read.
  vtkGetMacro(NumberOfPieces, int);

  // For the specified port, copy the information this reader sets up in
  // SetupOutputInformation to outInfo
  virtual void CopyOutputInformation(vtkInformation *outInfo, int port);

protected:
  vtkXMLPDataReader();
  ~vtkXMLPDataReader();

  // Pipeline execute information driver.  Called by vtkXMLReader.
  int ReadXMLInformation();
  virtual void SetupOutputInformation(vtkInformation *outInfo);

  int ReadPrimaryElement(vtkXMLDataElement* ePrimary);

  vtkDataSet* GetPieceInputAsDataSet(int piece);
  void SetupOutputData();

  virtual vtkXMLDataReader* CreatePieceReader()=0;
  virtual vtkIdType GetNumberOfPoints()=0;
  virtual vtkIdType GetNumberOfCells()=0;
  virtual void CopyArrayForPoints(vtkDataArray* inArray,
                                  vtkDataArray* outArray)=0;
  virtual void CopyArrayForCells(vtkDataArray* inArray,
                                 vtkDataArray* outArray)=0;

  virtual void SetupPieces(int numPieces);
  virtual void DestroyPieces();
  int ReadPiece(vtkXMLDataElement* ePiece, int index);
  virtual int ReadPiece(vtkXMLDataElement* ePiece);
  int ReadPieceData(int index);
  virtual int ReadPieceData();
  int CanReadPiece(int index);

  char* CreatePieceFileName(const char* fileName);
  void SplitFileName();

  // Callback registered with the PieceProgressObserver.
  static void PieceProgressCallbackFunction(vtkObject*, unsigned long, void*,
                                           void*);
  virtual void PieceProgressCallback();

  // Pieces from the input summary file.
  int NumberOfPieces;

  // The ghost level available on each input piece.
  int GhostLevel;

  // The piece currently being read.
  int Piece;

  // The path to the input file without the file name.
  char* PathName;

  // Information per-piece.
  vtkXMLDataElement** PieceElements;
  vtkXMLDataReader** PieceReaders;
  int* CanReadPieceFlag;

  // The PPointData and PCellData element representations.
  vtkXMLDataElement* PPointDataElement;
  vtkXMLDataElement* PCellDataElement;

  // The observer to report progress from reading serial data in each
  // piece.
  vtkCallbackCommand* PieceProgressObserver;

private:
  vtkXMLPDataReader(const vtkXMLPDataReader&);  // Not implemented.
  void operator=(const vtkXMLPDataReader&);  // Not implemented.
};

#endif
