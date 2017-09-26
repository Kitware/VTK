/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPDataSetWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLPDataSetWriter
 * @brief   Write any type of PVTK XML file.
 *
 * vtkXMLPDataSetWriter is a wrapper around the PVTK XML file format
 * writers.  Given an input vtkDataSet, the correct writer is
 * automatically selected based on the type of input.
 *
 * @sa
 * vtkXMLPImageDataWriter vtkXMLPStructuredGridWriter
 * vtkXMLPRectilinearGridWriter vtkXMLPPolyDataWriter
 * vtkXMLPUnstructuredGridWriter
*/

#ifndef vtkXMLPDataSetWriter_h
#define vtkXMLPDataSetWriter_h

#include "vtkIOParallelXMLModule.h" // For export macro
#include "vtkXMLPDataWriter.h"

class VTKIOPARALLELXML_EXPORT vtkXMLPDataSetWriter : public vtkXMLPDataWriter
{
public:
  vtkTypeMacro(vtkXMLPDataSetWriter,vtkXMLPDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;
  static vtkXMLPDataSetWriter* New();

  /**
   * Get/Set the writer's input.
   */
  vtkDataSet* GetInput();

protected:
  vtkXMLPDataSetWriter();
  ~vtkXMLPDataSetWriter() override;

  // see algorithm for more info
  int FillInputPortInformation(int port, vtkInformation* info) override;

  // Override writing method from superclass.
  int WriteInternal() override;

  // Dummies to satisfy pure virtuals from superclass.
  const char* GetDataSetName() override;
  const char* GetDefaultFileExtension() override;
  vtkXMLWriter* CreatePieceWriter(int index) override;

private:
  vtkXMLPDataSetWriter(const vtkXMLPDataSetWriter&) = delete;
  void operator=(const vtkXMLPDataSetWriter&) = delete;
};

#endif
