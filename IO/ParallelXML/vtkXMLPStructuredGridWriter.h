/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPStructuredGridWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLPStructuredGridWriter
 * @brief   Write PVTK XML StructuredGrid files.
 *
 * vtkXMLPStructuredGridWriter writes the PVTK XML StructuredGrid
 * file format.  One structured grid input can be written into a
 * parallel file format with any number of pieces spread across files.
 * The standard extension for this writer's file format is "pvts".
 * This writer uses vtkXMLStructuredGridWriter to write the individual
 * piece files.
 *
 * @sa
 * vtkXMLStructuredGridWriter
*/

#ifndef vtkXMLPStructuredGridWriter_h
#define vtkXMLPStructuredGridWriter_h

#include "vtkIOParallelXMLModule.h" // For export macro
#include "vtkXMLPStructuredDataWriter.h"

class vtkStructuredGrid;

class VTKIOPARALLELXML_EXPORT vtkXMLPStructuredGridWriter : public vtkXMLPStructuredDataWriter
{
public:
  static vtkXMLPStructuredGridWriter* New();
  vtkTypeMacro(vtkXMLPStructuredGridWriter,vtkXMLPStructuredDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Get/Set the writer's input.
   */
  vtkStructuredGrid* GetInput();

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension();

protected:
  vtkXMLPStructuredGridWriter();
  ~vtkXMLPStructuredGridWriter();

  // see algorithm for more info
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  const char* GetDataSetName();
  vtkXMLStructuredDataWriter* CreateStructuredPieceWriter();
  void WritePData(vtkIndent indent);

private:
  vtkXMLPStructuredGridWriter(const vtkXMLPStructuredGridWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLPStructuredGridWriter&) VTK_DELETE_FUNCTION;
};

#endif
