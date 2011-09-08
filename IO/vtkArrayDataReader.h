/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayDataReader.h
  
-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// .NAME vtkArrayDataReader -  Reads vtkArrayData written by vtkArrayDataWriter.
//
// .SECTION Description
// Reads vtkArrayData data written with vtkArrayDataWriter.
//
// Outputs:
//   Output port 0: vtkArrayData containing a collection of vtkArrays.
//
// .SECTION See Also
// vtkArrayDataWriter

#ifndef __vtkArrayDataReader_h
#define __vtkArrayDataReader_h

#include "vtkArrayDataAlgorithm.h"

class VTK_IO_EXPORT vtkArrayDataReader :
  public vtkArrayDataAlgorithm
{
public:
  static vtkArrayDataReader* New();
  vtkTypeMacro(vtkArrayDataReader, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Set the filesystem location from which data will be read.
  vtkGetStringMacro(FileName);
  vtkSetStringMacro(FileName);
  
  // Description:
  // The input string to parse. If you set the input string, you must also set
  // the ReadFromInputString flag to parse the string instead of a file.
  virtual void SetInputString(const vtkStdString& string);
  virtual vtkStdString GetInputString();
  
  // Description:
  // Whether to read from an input string as opposed to a file, which is the default.
  vtkSetMacro(ReadFromInputString, bool);
  vtkGetMacro(ReadFromInputString, bool);
  vtkBooleanMacro(ReadFromInputString, bool);

  // Description:
  // Read an arbitrary array from a stream.  Note: you MUST always
  // open streams in binary mode to prevent problems reading files
  // on Windows.
  static vtkArrayData* Read(istream& stream);

  // Description:
  // Read an arbitrary array from a string.
  static vtkArrayData* Read(vtkStdString str);

protected:
  vtkArrayDataReader();
  ~vtkArrayDataReader();

  int RequestData(
    vtkInformation*, 
    vtkInformationVector**, 
    vtkInformationVector*);

  char* FileName;
  vtkStdString InputString;
  bool ReadFromInputString;

private:
  vtkArrayDataReader(const vtkArrayDataReader&); // Not implemented
  void operator=(const vtkArrayDataReader&);   // Not implemented
};

#endif
