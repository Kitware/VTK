/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPStructuredGridReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLPStructuredGridReader
 * @brief   Read PVTK XML StructuredGrid files.
 *
 * vtkXMLPStructuredGridReader reads the PVTK XML StructuredGrid file
 * format.  This reads the parallel format's summary file and then
 * uses vtkXMLStructuredGridReader to read data from the individual
 * StructuredGrid piece files.  Streaming is supported.  The standard
 * extension for this reader's file format is "pvts".
 *
 * @sa
 * vtkXMLStructuredGridReader
*/

#ifndef vtkXMLPStructuredGridReader_h
#define vtkXMLPStructuredGridReader_h

#include "vtkIOXMLModule.h" // For export macro
#include "vtkXMLPStructuredDataReader.h"

class vtkStructuredGrid;

class VTKIOXML_EXPORT vtkXMLPStructuredGridReader : public vtkXMLPStructuredDataReader
{
public:
  vtkTypeMacro(vtkXMLPStructuredGridReader,vtkXMLPStructuredDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;
  static vtkXMLPStructuredGridReader *New();

  /**
   * Get the reader's output.
   */
  vtkStructuredGrid *GetOutput();

  /**
   * Needed for ParaView
   */
  vtkStructuredGrid* GetOutput(int idx);

protected:
  vtkXMLPStructuredGridReader();
  ~vtkXMLPStructuredGridReader() VTK_OVERRIDE;

  vtkStructuredGrid* GetPieceInput(int index);

  void SetupEmptyOutput() VTK_OVERRIDE;
  const char* GetDataSetName() VTK_OVERRIDE;
  void SetOutputExtent(int* extent) VTK_OVERRIDE;
  void GetPieceInputExtent(int index, int* extent) VTK_OVERRIDE;
  int ReadPrimaryElement(vtkXMLDataElement* ePrimary) VTK_OVERRIDE;
  void SetupOutputData() VTK_OVERRIDE;
  int ReadPieceData() VTK_OVERRIDE;
  vtkXMLDataReader* CreatePieceReader() VTK_OVERRIDE;
  int FillOutputPortInformation(int, vtkInformation*) VTK_OVERRIDE;

  // The PPoints element with point information.
  vtkXMLDataElement* PPointsElement;

private:
  vtkXMLPStructuredGridReader(const vtkXMLPStructuredGridReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLPStructuredGridReader&) VTK_DELETE_FUNCTION;
};

#endif
