/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPPolyDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLPPolyDataWriter
 * @brief   Write PVTK XML PolyData files.
 *
 * vtkXMLPPolyDataWriter writes the PVTK XML PolyData file format.
 * One poly data input can be written into a parallel file format with
 * any number of pieces spread across files.  The standard extension
 * for this writer's file format is "pvtp".  This writer uses
 * vtkXMLPolyDataWriter to write the individual piece files.
 *
 * @sa
 * vtkXMLPolyDataWriter
*/

#ifndef vtkXMLPPolyDataWriter_h
#define vtkXMLPPolyDataWriter_h

#include "vtkIOParallelXMLModule.h" // For export macro
#include "vtkXMLPUnstructuredDataWriter.h"

class vtkPolyData;

class VTKIOPARALLELXML_EXPORT vtkXMLPPolyDataWriter : public vtkXMLPUnstructuredDataWriter
{
public:
  static vtkXMLPPolyDataWriter* New();
  vtkTypeMacro(vtkXMLPPolyDataWriter,vtkXMLPUnstructuredDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  /**
   * Get/Set the writer's input.
   */
  vtkPolyData* GetInput();

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension();

protected:
  vtkXMLPPolyDataWriter();
  ~vtkXMLPPolyDataWriter();

  // see algorithm for more info
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  const char* GetDataSetName();
  vtkXMLUnstructuredDataWriter* CreateUnstructuredPieceWriter();

private:
  vtkXMLPPolyDataWriter(const vtkXMLPPolyDataWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLPPolyDataWriter&) VTK_DELETE_FUNCTION;
};

#endif
