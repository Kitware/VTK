/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPUnstructuredDataReader.h
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
// .NAME vtkXMLPUnstructuredDataReader - Superclass for parallel unstructured data XML readers.
// .SECTION Description
// vtkXMLPUnstructuredDataReader provides functionality common to all
// parallel unstructured data format readers.

// .SECTION See Also
// vtkXMLPPolyDataReader vtkXMLPUnstructuredGridReader

#ifndef __vtkXMLPUnstructuredDataReader_h
#define __vtkXMLPUnstructuredDataReader_h

#include "vtkXMLPDataReader.h"

class vtkPointSet;
class vtkCellArray;
class vtkXMLUnstructuredDataReader;

class VTK_IO_EXPORT vtkXMLPUnstructuredDataReader : public vtkXMLPDataReader
{
public:
  vtkTypeRevisionMacro(vtkXMLPUnstructuredDataReader,vtkXMLPDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);
  
protected:
  vtkXMLPUnstructuredDataReader();
  ~vtkXMLPUnstructuredDataReader();
  
  vtkPointSet* GetOutputAsPointSet();
  vtkPointSet* GetPieceInputAsPointSet(int piece);
  virtual void SetupOutputTotals();
  virtual void SetupNextPiece();
  vtkIdType GetNumberOfPoints();
  vtkIdType GetNumberOfCells();
  void CopyArrayForPoints(vtkDataArray* inArray, vtkDataArray* outArray);
  
  void SetupEmptyOutput();
  void SetupOutputInformation();
  void SetupOutputData();
  virtual void GetOutputUpdateExtent(int& piece, int& numberOfPieces,
                                     int& ghostLevel)=0;
  
  // Pipeline execute data driver.  Called by vtkXMLReader.
  void ReadXMLData();  
  int ReadPrimaryElement(vtkXMLDataElement* ePrimary);
  void SetupUpdateExtent(int piece, int numberOfPieces, int ghostLevel);
  
  int ReadPieceData();
  void CopyCellArray(vtkIdType totalNumberOfCells, vtkCellArray* inCells,
                     vtkCellArray* outCells);
  
  // The update request.
  int UpdatePiece;
  int UpdateNumberOfPieces;
  int UpdateGhostLevel;
  
  // The range of pieces from the file that will form the UpdatePiece.
  int StartPiece;
  int EndPiece;
  vtkIdType TotalNumberOfPoints;
  vtkIdType TotalNumberOfCells;
  vtkIdType StartPoint;
  
  // The PPoints element with point information.
  vtkXMLDataElement* PPointsElement;
  
private:
  vtkXMLPUnstructuredDataReader(const vtkXMLPUnstructuredDataReader&);  // Not implemented.
  void operator=(const vtkXMLPUnstructuredDataReader&);  // Not implemented.
};

#endif
