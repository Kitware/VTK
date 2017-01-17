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
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
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
  ~vtkXMLPUnstructuredGridReader() VTK_OVERRIDE;

  const char* GetDataSetName() VTK_OVERRIDE;
  void GetOutputUpdateExtent(int& piece, int& numberOfPieces, int& ghostLevel) VTK_OVERRIDE;
  void SetupOutputTotals() VTK_OVERRIDE;

  void SetupOutputData() VTK_OVERRIDE;
  void SetupNextPiece() VTK_OVERRIDE;
  int ReadPieceData() VTK_OVERRIDE;

  void CopyArrayForCells(vtkDataArray* inArray, vtkDataArray* outArray) VTK_OVERRIDE;
  vtkXMLDataReader* CreatePieceReader() VTK_OVERRIDE;
  int FillOutputPortInformation(int, vtkInformation*) VTK_OVERRIDE;

  void SqueezeOutputArrays(vtkDataObject*) VTK_OVERRIDE;

  // The index of the cell in the output where the current piece
  // begins.
  vtkIdType StartCell;

private:
  vtkXMLPUnstructuredGridReader(const vtkXMLPUnstructuredGridReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLPUnstructuredGridReader&) VTK_DELETE_FUNCTION;
};

#endif
