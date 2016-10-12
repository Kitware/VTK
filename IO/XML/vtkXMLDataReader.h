/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLDataReader
 * @brief   Superclass for VTK XML file readers.
 *
 * vtkXMLDataReader provides functionality common to all file readers for
 * <a href="http://www.vtk.org/Wiki/VTK_XML_Formats">VTK XML formats</a>.
 * Concrete subclasses call upon this functionality when needed.
 *
 * @sa
 * vtkXMLPDataReader
*/

#ifndef vtkXMLDataReader_h
#define vtkXMLDataReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLReader.h"

class VTKIOXML_EXPORT vtkXMLDataReader : public vtkXMLReader
{
public:
  enum FieldType
  {
    POINT_DATA,
    CELL_DATA,
    OTHER
  };


  vtkTypeMacro(vtkXMLDataReader,vtkXMLReader);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Get the number of points in the output.
   */
  virtual vtkIdType GetNumberOfPoints()=0;

  /**
   * Get the number of cells in the output.
   */
  virtual vtkIdType GetNumberOfCells()=0;

  // For the specified port, copy the information this reader sets up in
  // SetupOutputInformation to outInfo
  virtual void CopyOutputInformation(vtkInformation *outInfo, int port);

protected:
  vtkXMLDataReader();
  ~vtkXMLDataReader();

  // Add functionality to methods from superclass.
  virtual void CreateXMLParser();
  virtual void DestroyXMLParser();
  virtual void SetupOutputInformation(vtkInformation *outInfo);

  int ReadPrimaryElement(vtkXMLDataElement* ePrimary);
  void SetupOutputData();

  // Setup the reader for a given number of pieces.
  virtual void SetupPieces(int numPieces);
  virtual void DestroyPieces();

  // Read information from the file for the given piece.
  int ReadPiece(vtkXMLDataElement* ePiece, int piece);
  virtual int ReadPiece(vtkXMLDataElement* ePiece);

  // Read data from the file for the given piece.
  int ReadPieceData(int piece);
  virtual int ReadPieceData();

  virtual void ReadXMLData();

  // Read a data array whose tuples coorrespond to points or cells.
  virtual int ReadArrayForPoints(vtkXMLDataElement* da,
                                 vtkAbstractArray* outArray);
  virtual int ReadArrayForCells(vtkXMLDataElement* da,
                                vtkAbstractArray* outArray);

  // Read an Array values starting at the given index and up to numValues.
  // This method assumes that the array is of correct size to
  // accommodate all numValues values. arrayIndex is the value index at which the read
  // values will be put in the array.
  int ReadArrayValues(
    vtkXMLDataElement* da, vtkIdType arrayIndex, vtkAbstractArray* array,
    vtkIdType startIndex, vtkIdType numValues, FieldType type = OTHER);



  // Callback registered with the DataProgressObserver.
  static void DataProgressCallbackFunction(vtkObject*, unsigned long, void*,
                                           void*);
  // Progress callback from XMLParser.
  virtual void DataProgressCallback();

  // The number of Pieces of data found in the file.
  int NumberOfPieces;

  // The PointData and CellData element representations for each piece.
  vtkXMLDataElement** PointDataElements;
  vtkXMLDataElement** CellDataElements;

  // The piece currently being read.
  int Piece;

  // The number of point/cell data arrays in the output.  Valid after
  // SetupOutputData has been called.
  int NumberOfPointArrays;
  int NumberOfCellArrays;

  // Flag for whether DataProgressCallback should actually update
  // progress.
  int InReadData;

  // The observer to report progress from reading data from XMLParser.
  vtkCallbackCommand* DataProgressObserver;

  // Specify the last time step read, useful to know if we need to rearead data
  // //PointData
  int *PointDataTimeStep;
  vtkTypeInt64 *PointDataOffset;
  int PointDataNeedToReadTimeStep(vtkXMLDataElement *eNested);

  //CellData
  int *CellDataTimeStep;
  vtkTypeInt64 *CellDataOffset;
  int CellDataNeedToReadTimeStep(vtkXMLDataElement *eNested);

private:
  vtkXMLDataReader(const vtkXMLDataReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLDataReader&) VTK_DELETE_FUNCTION;

  void ConvertGhostLevelsToGhostType(
    FieldType type, vtkAbstractArray* data, vtkIdType startIndex,
    vtkIdType numValues);

};

#endif
