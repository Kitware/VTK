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
// .NAME vtkXMLPUnstructuredDataWriter - Superclass for PVTK XML unstructured data writers.
// .SECTION Description
// vtkXMLPUnstructuredDataWriter provides PVTK XML writing
// functionality that is common among all the parallel unstructured
// data formats.

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
  void PrintSelf(ostream& os, vtkIndent indent);

  // See the vtkAlgorithm for a desciption of what these do
  int ProcessRequest(vtkInformation*,
                     vtkInformationVector**,
                     vtkInformationVector*);

protected:
  vtkXMLPUnstructuredDataWriter();
  ~vtkXMLPUnstructuredDataWriter();

  vtkPointSet* GetInputAsPointSet();

  virtual vtkXMLUnstructuredDataWriter* CreateUnstructuredPieceWriter()=0;
  vtkXMLWriter* CreatePieceWriter(int index);
  void WritePData(vtkIndent indent);

private:
  vtkXMLPUnstructuredDataWriter(const vtkXMLPUnstructuredDataWriter&);  // Not implemented.
  void operator=(const vtkXMLPUnstructuredDataWriter&);  // Not implemented.
};

#endif
