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

/**
 * @class   vtkArrayWriter
 * @brief   Serialize sparse and dense arrays to a file or stream.
 *
 *
 * vtkArrayWriter serializes sparse and dense array data using a text-based
 * format that is human-readable and easily parsed (default option).  The
 * WriteBinary array option can be used to serialize the sparse and dense array data
 * using a binary format that is optimized for rapid throughput.
 *
 * vtkArrayWriter can be used in two distinct ways: first, it can be used as a
 * normal pipeline filter, which writes its inputs to a file.  Alternatively, static
 * methods are provided for writing vtkArray instances to files or arbitrary c++
 * streams.
 *
 * Inputs:
 *   Input port 0: (required) vtkArrayData object containing a single sparse or dense
 *                            array.
 *
 * Output Format:
 *   See http://www.kitware.com/InfovisWiki/index.php/N-Way_Array_File_Formats for
 *   details on how vtkArrayWriter encodes data.
 *
 * @sa
 * vtkArrayReader
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
*/

#ifndef vtkArrayWriter_h
#define vtkArrayWriter_h

#include "vtkIOCoreModule.h" // For export macro
#include "vtkWriter.h"
#include "vtkStdString.h" // For string API

class vtkArray;

class VTKIOCORE_EXPORT vtkArrayWriter :
  public vtkWriter
{
public:
  static vtkArrayWriter *New();
  vtkTypeMacro(vtkArrayWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent) VTK_OVERRIDE;

  //@{
  /**
   * Get / set the filename where data will be stored (when used as a filter).
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Get / set whether data will be written in binary format (when used as a filter).
   */
  vtkSetMacro(Binary, int);
  vtkGetMacro(Binary, int);
  vtkBooleanMacro(Binary, int);
  //@}

  /**
   * The output string. This is only set when WriteToOutputString is set.
   */
  virtual vtkStdString GetOutputString()
    { return this->OutputString; }

  //@{
  /**
   * Whether to output to a string instead of to a file, which is the default.
   */
  vtkSetMacro(WriteToOutputString, bool);
  vtkGetMacro(WriteToOutputString, bool);
  vtkBooleanMacro(WriteToOutputString, bool);
  //@}

  int Write() VTK_OVERRIDE; // This is necessary to get Write() wrapped for scripting languages.

  /**
   * Writes input port 0 data to a file, using an arbitrary filename and binary flag.
   */
  bool Write(const vtkStdString& FileName, bool WriteBinary = false);

  /**
   * Write an arbitrary array to a file, without using the pipeline.
   */
  static bool Write(vtkArray* array, const vtkStdString& file_name, bool WriteBinary = false);

  /**
   * Write input port 0 data to an arbitrary stream.  Note: streams should always be opened in
   * binary mode, to prevent problems reading files on Windows.
   */
  bool Write(ostream& stream, bool WriteBinary = false);

  /**
   * Write arbitrary data to a stream without using the pipeline.  Note: streams should always
   * be opened in binary mode, to prevent problems reading files on Windows.
   */
  static bool Write(vtkArray* array, ostream& stream, bool WriteBinary = false);

  /**
   * Write input port 0 data to a string. Note that the WriteBinary argument is not
   * optional in order to not clash with the inherited Write() method.
   */
  vtkStdString Write(bool WriteBinary);

  /**
   * Write arbitrary data to a string without using the pipeline.
   */
  static vtkStdString Write(vtkArray* array, bool WriteBinary = false);

protected:
  vtkArrayWriter();
  ~vtkArrayWriter() VTK_OVERRIDE;

  int FillInputPortInformation(int port, vtkInformation* info) VTK_OVERRIDE;
  void WriteData() VTK_OVERRIDE;

  char* FileName;
  int Binary;
  bool WriteToOutputString;
  vtkStdString OutputString;

private:
  vtkArrayWriter(const vtkArrayWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkArrayWriter&) VTK_DELETE_FUNCTION;
};

#endif

