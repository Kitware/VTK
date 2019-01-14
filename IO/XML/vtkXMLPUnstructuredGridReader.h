/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPUnstructuredGridReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLPUnstructuredGridReader
 * @brief   Read PVTK XML UnstructuredGrid files.
 *
 * vtkXMLPUnstructuredGridReader reads the PVTK XML UnstructuredGrid
 * file format.  This reads the parallel format's summary file and
 * then uses vtkXMLUnstructuredGridReader to read data from the
 * individual UnstructuredGrid piece files.  Streaming is supported.
 * The standard extension for this reader's file format is "pvtu".
 *
 * @sa
 * vtkXMLUnstructuredGridReader
*/

#ifndef vtkXMLPUnstructuredGridReader_h
#define vtkXMLPUnstructuredGridReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLPUnstructuredDataReader.h"

class vtkUnstructuredGrid;

class VTKIOXML_EXPORT vtkXMLPUnstructuredGridReader : public vtkXMLPUnstructuredDataReader
{
public:
  vtkTypeMacro(vtkXMLPUnstructuredGridReader,vtkXMLPUnstructuredDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkXMLPUnstructuredGridReader *New();

  //@{
  /**
   * Get the reader's output.
   */
  vtkUnstructuredGrid *GetOutput();
  vtkUnstructuredGrid *GetOutput(int idx);
  //@}

protected:
  vtkXMLPUnstructuredGridReader();
  ~vtkXMLPUnstructuredGridReader() override;

  const char* GetDataSetName() override;
  void GetOutputUpdateExtent(int& piece, int& numberOfPieces, int& ghostLevel) override;
  void SetupOutputTotals() override;

  void SetupOutputData() override;
  void SetupNextPiece() override;
  int ReadPieceData() override;

  void CopyArrayForCells(vtkDataArray* inArray, vtkDataArray* outArray) override;
  vtkXMLDataReader* CreatePieceReader() override;
  int FillOutputPortInformation(int, vtkInformation*) override;

  void SqueezeOutputArrays(vtkDataObject*) override;

  // The index of the cell in the output where the current piece
  // begins.
  vtkIdType StartCell;

private:
  vtkXMLPUnstructuredGridReader(const vtkXMLPUnstructuredGridReader&) = delete;
  void operator=(const vtkXMLPUnstructuredGridReader&) = delete;
};

#endif
