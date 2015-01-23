/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkXMLPUniformGridAMRWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkXMLPUniformGridAMRWriter - parallel writer for
// vtkUniformGridAMR and subclasses.
// .SECTION Description
// vtkXMLPCompositeDataWriter writes (in parallel or serially) vtkUniformGridAMR
// and subclasses. When running in parallel all processes are expected to have
// the same meta-data (i.e. amr-boxes, structure, etc.) however they may now
// have the missing data-blocks. This class extends
// vtkXMLUniformGridAMRWriter to communicate information about data blocks
// to the root node so that the root node can write the XML file describing the
// structure correctly.

#ifndef vtkXMLPUniformGridAMRWriter_h
#define vtkXMLPUniformGridAMRWriter_h

#include "vtkIOParallelXMLModule.h" // For export macro
#include "vtkXMLUniformGridAMRWriter.h"

class vtkMultiProcessController;

class VTKIOPARALLELXML_EXPORT vtkXMLPUniformGridAMRWriter : public vtkXMLUniformGridAMRWriter
{
public:
  static vtkXMLPUniformGridAMRWriter* New();
  vtkTypeMacro(vtkXMLPUniformGridAMRWriter, vtkXMLUniformGridAMRWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Controller used to communicate data type of blocks.
  // By default, the global controller is used. If you want another
  // controller to be used, set it with this.
  // If no controller is set, only the local blocks will be written
  // to the meta-file.
  virtual void SetController(vtkMultiProcessController*);
  vtkGetObjectMacro(Controller, vtkMultiProcessController);

  // Description:
  // Set whether this instance will write the meta-file. WriteMetaFile
  // is set to flag only on process 0 and all other processes have
  // WriteMetaFile set to 0 by default.
  virtual void SetWriteMetaFile(int flag);

//BTX
protected:
  vtkXMLPUniformGridAMRWriter();
  ~vtkXMLPUniformGridAMRWriter();

  // Description:
  // Overridden to reduce information about data-types across all processes.
  virtual void FillDataTypes(vtkCompositeDataSet*);

  vtkMultiProcessController* Controller;
private:
  vtkXMLPUniformGridAMRWriter(const vtkXMLPUniformGridAMRWriter&); // Not implemented.
  void operator=(const vtkXMLPUniformGridAMRWriter&); // Not implemented.
//ETX
};

#endif
