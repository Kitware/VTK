/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLUnstructuredDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLUnstructuredDataReader
 * @brief   Superclass for unstructured data XML readers.
 *
 * vtkXMLUnstructuredDataReader provides functionality common to all
 * unstructured data format readers.
 *
 * @sa
 * vtkXMLPolyDataReader vtkXMLUnstructuredGridReader
*/

#ifndef vtkXMLUnstructuredDataReader_h
#define vtkXMLUnstructuredDataReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLDataReader.h"

class vtkCellArray;
class vtkIdTypeArray;
class vtkPointSet;
class vtkUnsignedCharArray;

class VTKIOXML_EXPORT vtkXMLUnstructuredDataReader : public vtkXMLDataReader
{
public:
  vtkTypeMacro(vtkXMLUnstructuredDataReader,vtkXMLDataReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Get the number of points in the output.
   */
  virtual vtkIdType GetNumberOfPoints();

  /**
   * Get the number of cells in the output.
   */
  virtual vtkIdType GetNumberOfCells();

  /**
   * Get the number of pieces in the file
   */
  virtual vtkIdType GetNumberOfPieces();

  /**
   * Setup the reader as if the given update extent were requested by
   * its output.  This can be used after an UpdateInformation to
   * validate GetNumberOfPoints() and GetNumberOfCells() without
   * actually reading data.
   */
  void SetupUpdateExtent(int piece, int numberOfPieces, int ghostLevel);

  // For the specified port, copy the information this reader sets up in
  // SetupOutputInformation to outInfo
  virtual void CopyOutputInformation(vtkInformation *outInfo, int port);


protected:
  vtkXMLUnstructuredDataReader();
  ~vtkXMLUnstructuredDataReader();

  vtkPointSet* GetOutputAsPointSet();
  vtkXMLDataElement* FindDataArrayWithName(vtkXMLDataElement* eParent,
                                           const char* name);
  vtkIdTypeArray* ConvertToIdTypeArray(vtkDataArray* a);
  vtkUnsignedCharArray* ConvertToUnsignedCharArray(vtkDataArray* a);

  // Pipeline execute data driver.  Called by vtkXMLReader.
  void ReadXMLData();

  virtual void SetupEmptyOutput();
  virtual void GetOutputUpdateExtent(int& piece, int& numberOfPieces,
                                     int& ghostLevel)=0;
  virtual void SetupOutputTotals();
  virtual void SetupNextPiece();
  void SetupPieces(int numPieces);
  void DestroyPieces();

  // Setup the output's information.
  void SetupOutputInformation(vtkInformation *outInfo);

  void SetupOutputData();
  int ReadPiece(vtkXMLDataElement* ePiece);
  int ReadPieceData();
  int ReadCellArray(vtkIdType numberOfCells, vtkIdType totalNumberOfCells,
                    vtkXMLDataElement* eCells, vtkCellArray* outCells);

  // Read faces and faceoffsets arrays for unstructured grid with polyhedon cells
  int ReadFaceArray(vtkIdType numberOfCells, vtkXMLDataElement* eCells,
                    vtkIdTypeArray* outFaces, vtkIdTypeArray* outFaceOffsets);

  // Read a data array whose tuples coorrespond to points.
  virtual int ReadArrayForPoints(vtkXMLDataElement* da, vtkAbstractArray* outArray);

  // Get the number of points/cells in the given piece.  Valid after
  // UpdateInformation.
  virtual vtkIdType GetNumberOfPointsInPiece(int piece);
  virtual vtkIdType GetNumberOfCellsInPiece(int piece)=0;

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

  int PointsTimeStep;
  unsigned long PointsOffset;
  int PointsNeedToReadTimeStep(vtkXMLDataElement *eNested);
  int CellsNeedToReadTimeStep(vtkXMLDataElement *eNested, int &cellstimestep,
    unsigned long &cellsoffset);


private:
  vtkXMLUnstructuredDataReader(const vtkXMLUnstructuredDataReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLUnstructuredDataReader&) VTK_DELETE_FUNCTION;
};

#endif
