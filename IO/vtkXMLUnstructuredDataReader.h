/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLUnstructuredDataReader.h
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
// .NAME vtkXMLUnstructuredDataReader - Superclass for unstructured data XML readers.
// .SECTION Description
// vtkXMLUnstructuredDataReader provides functionality common to all
// unstructured data format readers.

// .SECTION See Also
// vtkXMLPolyDataReader vtkXMLUnstructuredGridReader

#ifndef __vtkXMLUnstructuredDataReader_h
#define __vtkXMLUnstructuredDataReader_h

#include "vtkXMLDataReader.h"

class vtkCellArray;
class vtkUnsignedCharArray;
class vtkPointSet;

class VTK_IO_EXPORT vtkXMLUnstructuredDataReader : public vtkXMLDataReader
{
public:
  vtkTypeRevisionMacro(vtkXMLUnstructuredDataReader,vtkXMLDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);  
  
  // Description:
  // Get the number of points in the output.
  virtual vtkIdType GetNumberOfPoints();
  
  // Description:
  // Get the number of cells in the output.
  virtual vtkIdType GetNumberOfCells();
  
  // Description:
  // Setup the reader as if the given update extent were requested by
  // its output.  This can be used after an UpdateInformation to
  // validate GetNumberOfPoints() and GetNumberOfCells() without
  // actually reading data.
  void SetupUpdateExtent(int piece, int numberOfPieces, int ghostLevel);
  
protected:
  vtkXMLUnstructuredDataReader();
  ~vtkXMLUnstructuredDataReader();
  
  vtkPointSet* GetOutputAsPointSet();
  vtkXMLDataElement* FindDataArrayWithName(vtkXMLDataElement* eParent,
                                           const char* name);
  vtkDataArray* CreateDataArrayWithName(vtkXMLDataElement* eParent,
                                        const char* name);
  
  // Pipeline execute data driver.  Called by vtkXMLReader.
  void ReadXMLData();
  
  virtual void SetupEmptyOutput();
  virtual void GetOutputUpdateExtent(int& piece, int& numberOfPieces,
                                     int& ghostLevel)=0;
  virtual void SetupOutputTotals();
  virtual void SetupNextPiece();
  void SetupPieces(int numPieces);
  void DestroyPieces();
  
  void SetupOutputInformation();
  void SetupOutputData();
  int ReadPiece(vtkXMLDataElement* ePiece);
  int ReadPieceData();
  int ReadCellArray(vtkIdType numberOfCells, vtkIdType totalNumberOfCells,
                    vtkXMLDataElement* eCells, vtkCellArray* outCells);
  
  // Read a data array whose tuples coorrespond to points.
  int ReadArrayForPoints(vtkXMLDataElement* da, vtkDataArray* outArray);
  
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
  
  // The Points element for each piece.
  vtkXMLDataElement** PointElements;
  vtkIdType* NumberOfPoints;
  
private:
  vtkXMLUnstructuredDataReader(const vtkXMLUnstructuredDataReader&);  // Not implemented.
  void operator=(const vtkXMLUnstructuredDataReader&);  // Not implemented.
};

#endif
