/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayWriter.h

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

// .NAME vtkArrayWriter - Serialize sparse and dense arrays to a file or stream.
//
// .SECTION Description
// vtkArrayWriter serializes sparse and dense array data using a text-based
// format that is human-readable and easily parsed (default option).  The
// WriteBinary array option can be used to serialize the sparse and dense array data
// using a binary format that is optimized for rapid throughput.
//
// vtkArrayWriter can be used in two distinct ways: first, it can be used as a
// normal pipeline filter, which writes its inputs to a file.  Alternatively, static
// methods are provided for writing vtkArray instances to files or arbitrary c++
// streams.
//
// Inputs:
//   Input port 0: (required) vtkArrayData object containing a single sparse or dense
//                            array.
//
// Output Format:
//   See http://www.kitware.com/InfovisWiki/index.php/N-Way_Array_File_Formats for
//   details on how vtkArrayWriter encodes data.
//
// .SECTION See Also
// vtkArrayReader
//
// .SECTION Thanks
// Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.


#ifndef __vtkArrayWriter_h
#define __vtkArrayWriter_h

#include <vtkWriter.h>

class vtkArray;
class vtkStdString;

class VTK_IO_EXPORT vtkArrayWriter :
  public vtkWriter
{
public:
  static vtkArrayWriter *New();
  vtkTypeMacro(vtkArrayWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Get / set the filename where data will be stored (when used as a filter).
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);

  // Description:
  // Get / set whether data will be written in binary format (when used as a filter).
  vtkSetMacro(Binary, int);
  vtkGetMacro(Binary, int);
  vtkBooleanMacro(Binary, int);

  virtual int Write(); // This is necessary to get Write() wrapped for scripting languages.

  // Description:
  // Writes input port 0 data to a file, using an arbitrary filename and binary flag.
  bool Write(const vtkStdString& FileName, bool WriteBinary = false);

  // Description:
  // Write an arbitrary array to a file, without using the pipeline.
  static bool Write(vtkArray* array, const vtkStdString& file_name, bool WriteBinary = false);

//BTX
  // Description:
  // Write input port 0 data to an arbitrary stream.
  bool Write(ostream& stream, bool WriteBinary = false);

  // Description:
  // Write arbitrary data to a stream without using the pipeline.
  static bool Write(vtkArray* array, ostream& stream, bool WriteBinary = false);
//ETX

protected:
  vtkArrayWriter();
  ~vtkArrayWriter();

  virtual int FillInputPortInformation(int port, vtkInformation* info);
  virtual void WriteData();

  char* FileName;
  int Binary;

private:
  vtkArrayWriter(const vtkArrayWriter&);  // Not implemented.
  void operator=(const vtkArrayWriter&);  // Not implemented.
};

#endif

