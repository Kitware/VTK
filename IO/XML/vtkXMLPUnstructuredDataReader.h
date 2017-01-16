/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPUnstructuredDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLPUnstructuredDataReader
 * @brief   Superclass for parallel unstructured data XML readers.
 *
 * vtkXMLPUnstructuredDataReader provides functionality common to all
 * parallel unstructured data format readers.
 *
 * @sa
 * vtkXMLPPolyDataReader vtkXMLPUnstructuredGridReader
*/

#ifndef vtkXMLPUnstructuredDataReader_h
#define vtkXMLPUnstructuredDataReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLPDataReader.h"

class vtkPointSet;
class vtkCellArray;
class vtkXMLUnstructuredDataReader;

class VTKIOXML_EXPORT vtkXMLPUnstructuredDataReader : public vtkXMLPDataReader
{
public:
  vtkTypeMacro(vtkXMLPUnstructuredDataReader,vtkXMLPDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  // For the specified port, copy the information this reader sets up in
  // SetupOutputInformation to outInfo
  void CopyOutputInformation(vtkInformation *outInfo, int port) VTK_OVERRIDE;

protected:
  vtkXMLPUnstructuredDataReader();
  ~vtkXMLPUnstructuredDataReader() VTK_OVERRIDE;

  int RequestInformation(vtkInformation *request,
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector) VTK_OVERRIDE;


  vtkPointSet* GetOutputAsPointSet();
  vtkPointSet* GetPieceInputAsPointSet(int piece);
  virtual void SetupOutputTotals();
  virtual void SetupNextPiece();
  vtkIdType GetNumberOfPoints() VTK_OVERRIDE;
  vtkIdType GetNumberOfCells() VTK_OVERRIDE;
  void CopyArrayForPoints(vtkDataArray* inArray, vtkDataArray* outArray) VTK_OVERRIDE;

  void SetupEmptyOutput() VTK_OVERRIDE;

  // Setup the output's information.
  void SetupOutputInformation(vtkInformation *outInfo) VTK_OVERRIDE;

  void SetupOutputData() VTK_OVERRIDE;
  virtual void GetOutputUpdateExtent(int& piece, int& numberOfPieces,
                                     int& ghostLevel)=0;

  // Pipeline execute data driver.  Called by vtkXMLReader.
  void ReadXMLData() VTK_OVERRIDE;
  int ReadPrimaryElement(vtkXMLDataElement* ePrimary) VTK_OVERRIDE;
  void SetupUpdateExtent(int piece, int numberOfPieces, int ghostLevel);

  int ReadPieceData() VTK_OVERRIDE;
  void CopyCellArray(vtkIdType totalNumberOfCells, vtkCellArray* inCells,
                     vtkCellArray* outCells);

  // Get the number of points/cells in the given piece.  Valid after
  // UpdateInformation.
  virtual vtkIdType GetNumberOfPointsInPiece(int piece);
  virtual vtkIdType GetNumberOfCellsInPiece(int piece);

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
  vtkXMLPUnstructuredDataReader(const vtkXMLPUnstructuredDataReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLPUnstructuredDataReader&) VTK_DELETE_FUNCTION;
};

#endif
