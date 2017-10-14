/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPRectilinearGridWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLPRectilinearGridWriter
 * @brief   Write PVTK XML RectilinearGrid files.
 *
 * vtkXMLPRectilinearGridWriter writes the PVTK XML RectilinearGrid
 * file format.  One rectilinear grid input can be written into a
 * parallel file format with any number of pieces spread across files.
 * The standard extension for this writer's file format is "pvtr".
 * This writer uses vtkXMLRectilinearGridWriter to write the
 * individual piece files.
 *
 * @sa
 * vtkXMLRectilinearGridWriter
*/

#ifndef vtkXMLPRectilinearGridWriter_h
#define vtkXMLPRectilinearGridWriter_h

#include "vtkIOParallelXMLModule.h" // For export macro
#include "vtkXMLPStructuredDataWriter.h"

class vtkRectilinearGrid;

class VTKIOPARALLELXML_EXPORT vtkXMLPRectilinearGridWriter : public vtkXMLPStructuredDataWriter
{
public:
  static vtkXMLPRectilinearGridWriter* New();
  vtkTypeMacro(vtkXMLPRectilinearGridWriter,vtkXMLPStructuredDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get/Set the writer's input.
   */
  vtkRectilinearGrid* GetInput();

  /**
   * Get the default file extension for files written by this writer.
   */
  const char* GetDefaultFileExtension() override;

protected:
  vtkXMLPRectilinearGridWriter();
  ~vtkXMLPRectilinearGridWriter() override;

  // see algorithm for more info
  int FillInputPortInformation(int port, vtkInformation* info) override;

  const char* GetDataSetName() override;
  vtkXMLStructuredDataWriter* CreateStructuredPieceWriter() override;
  void WritePData(vtkIndent indent) override;

private:
  vtkXMLPRectilinearGridWriter(const vtkXMLPRectilinearGridWriter&) = delete;
  void operator=(const vtkXMLPRectilinearGridWriter&) = delete;
};

#endif
