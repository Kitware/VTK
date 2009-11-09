/*=========================================================================

  Program:   ParaView
  Module:    vtkJavaScriptDataWriter.h

  Copyright (c) Kitware, Inc.
  All rights reserved.
  See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2009 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

// .NAME vtkJavaScriptDataWriter - A Javascript data writer for vtkTable
// Writes a vtkTable into a Javascript data format. 
#ifndef __vtkJavaScriptDataWriter_h
#define __vtkJavaScriptDataWriter_h

#include "vtkWriter.h"

class vtkStdString;
class vtkTable;

class VTK_IO_EXPORT vtkJavaScriptDataWriter : public vtkWriter
{
public:
  static vtkJavaScriptDataWriter* New();
  vtkTypeRevisionMacro(vtkJavaScriptDataWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get/Set the filename for the file.
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

protected:
  vtkJavaScriptDataWriter();
  ~vtkJavaScriptDataWriter();

  bool OpenFile();

  virtual void WriteData();
  virtual void WriteTable(vtkTable* table);

  // see algorithm for more info.
  // This writer takes in vtkTable.
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  char* FileName;
  ofstream* Stream;
private:
  vtkJavaScriptDataWriter(const vtkJavaScriptDataWriter&); // Not implemented.
  void operator=(const vtkJavaScriptDataWriter&); // Not implemented.
//ETX
};



#endif

