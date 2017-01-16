/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPStructuredDataReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLPStructuredDataReader
 * @brief   Superclass for parallel structured data XML readers.
 *
 * vtkXMLPStructuredDataReader provides functionality common to all
 * parallel structured data format readers.
 *
 * @sa
 * vtkXMLPImageDataReader vtkXMLPStructuredGridReader
 * vtkXMLPRectilinearGridReader
*/

#ifndef vtkXMLPStructuredDataReader_h
#define vtkXMLPStructuredDataReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLPDataReader.h"

class vtkExtentSplitter;
class vtkXMLStructuredDataReader;

class VTKIOXML_EXPORT vtkXMLPStructuredDataReader : public vtkXMLPDataReader
{
public:
  vtkTypeMacro(vtkXMLPStructuredDataReader,vtkXMLPDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  // For the specified port, copy the information this reader sets up in
  // SetupOutputInformation to outInfo
  void CopyOutputInformation(vtkInformation *outInfo, int port) VTK_OVERRIDE;
protected:
  vtkXMLPStructuredDataReader();
  ~vtkXMLPStructuredDataReader() VTK_OVERRIDE;

  vtkIdType GetNumberOfPoints() VTK_OVERRIDE;
  vtkIdType GetNumberOfCells() VTK_OVERRIDE;
  void CopyArrayForPoints(vtkDataArray* inArray, vtkDataArray* outArray) VTK_OVERRIDE;
  void CopyArrayForCells(vtkDataArray* inArray, vtkDataArray* outArray) VTK_OVERRIDE;

  virtual void SetOutputExtent(int* extent)=0;
  virtual void GetPieceInputExtent(int index, int* extent)=0;

  // Pipeline execute data driver.  Called by vtkXMLReader.
  void ReadXMLData() VTK_OVERRIDE;
  int ReadPrimaryElement(vtkXMLDataElement* ePrimary) VTK_OVERRIDE;

  void SetupOutputData() VTK_OVERRIDE;

  void SetupPieces(int numPieces) VTK_OVERRIDE;
  void DestroyPieces() VTK_OVERRIDE;
  int ReadPiece(vtkXMLDataElement* ePiece) VTK_OVERRIDE;
  int ReadPieceData() VTK_OVERRIDE;
  void CopySubExtent(int* inExtent, int* inDimensions, vtkIdType* inIncrements,
                     int* outExtent,int* outDimensions,vtkIdType* outIncrements,
                     int* subExtent, int* subDimensions,
                     vtkDataArray* inArray, vtkDataArray* outArray);
  int ComputePieceSubExtents();

  vtkExtentSplitter* ExtentSplitter;

  // The extent to be updated in the output.
  int UpdateExtent[6];
  int PointDimensions[3];
  vtkIdType PointIncrements[3];
  int CellDimensions[3];
  vtkIdType CellIncrements[3];

  // The extent currently being read from a piece.
  int SubExtent[6];
  int SubPointDimensions[3];
  int SubCellDimensions[3];
  int SubPieceExtent[6];
  int SubPiecePointDimensions[3];
  vtkIdType SubPiecePointIncrements[3];
  int SubPieceCellDimensions[3];
  vtkIdType SubPieceCellIncrements[3];

  // Information per-piece.
  int* PieceExtents;

  int RequestInformation(vtkInformation *request,
                                 vtkInformationVector **inputVector,
                                 vtkInformationVector *outputVector) VTK_OVERRIDE;

private:
  vtkXMLPStructuredDataReader(const vtkXMLPStructuredDataReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLPStructuredDataReader&) VTK_DELETE_FUNCTION;
};

#endif
