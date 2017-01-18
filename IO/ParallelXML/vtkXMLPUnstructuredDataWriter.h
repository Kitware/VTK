/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPUnstructuredDataWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkXMLPUnstructuredDataWriter
 * @brief   Superclass for PVTK XML unstructured data writers.
 *
 * vtkXMLPUnstructuredDataWriter provides PVTK XML writing
 * functionality that is common among all the parallel unstructured
 * data formats.
*/

#ifndef vtkXMLPUnstructuredDataWriter_h
#define vtkXMLPUnstructuredDataWriter_h

#include "vtkIOParallelXMLModule.h" // For export macro
#include "vtkXMLPDataWriter.h"

class vtkPointSet;
class vtkXMLUnstructuredDataWriter;

class VTKIOPARALLELXML_EXPORT vtkXMLPUnstructuredDataWriter : public vtkXMLPDataWriter
{
public:
  vtkTypeMacro(vtkXMLPUnstructuredDataWriter,vtkXMLPDataWriter);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

protected:
  vtkXMLPUnstructuredDataWriter();
  ~vtkXMLPUnstructuredDataWriter() VTK_OVERRIDE;

  vtkPointSet* GetInputAsPointSet();
  virtual vtkXMLUnstructuredDataWriter* CreateUnstructuredPieceWriter()=0;
  vtkXMLWriter* CreatePieceWriter(int index) VTK_OVERRIDE;
  void WritePData(vtkIndent indent) VTK_OVERRIDE;
private:
  vtkXMLPUnstructuredDataWriter(const vtkXMLPUnstructuredDataWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkXMLPUnstructuredDataWriter&) VTK_DELETE_FUNCTION;
};

#endif
