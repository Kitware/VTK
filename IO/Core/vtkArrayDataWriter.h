/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayDataWriter.h

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
 * @class   vtkArrayDataWriter
 * @brief   Serialize vtkArrayData to a file or stream.
 *
 *
 * vtkArrayDataWriter serializes vtkArrayData using a text-based
 * format that is human-readable and easily parsed (default option).  The
 * WriteBinary array option can be used to serialize the vtkArrayData
 * using a binary format that is optimized for rapid throughput.
 *
 * vtkArrayDataWriter can be used in two distinct ways: first, it can be used as a
 * normal pipeline filter, which writes its inputs to a file.  Alternatively, static
 * methods are provided for writing vtkArrayData instances to files or arbitrary c++
 * streams.
 *
 * Inputs:
 *   Input port 0: (required) vtkArrayData object.
 *
 * Output Format:
 *   See http://www.kitware.com/InfovisWiki/index.php/N-Way_Array_File_Formats for
 *   details on how vtkArrayDataWriter encodes data.
 *
 * @sa
 * vtkArrayDataReader
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
*/

#ifndef vtkArrayDataWriter_h
#define vtkArrayDataWriter_h

#include "vtkIOCoreModule.h" // For export macro
#include "vtkWriter.h"
#include "vtkStdString.h" // For string API

class vtkArrayData;

class VTKIOCORE_EXPORT vtkArrayDataWriter :
  public vtkWriter
{
public:
  static vtkArrayDataWriter *New();
  vtkTypeMacro(vtkArrayDataWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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
  vtkSetMacro(Binary, vtkTypeBool);
  vtkGetMacro(Binary, vtkTypeBool);
  vtkBooleanMacro(Binary, vtkTypeBool);
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

  int Write() override; // This is necessary to get Write() wrapped for scripting languages.

  /**
   * Writes input port 0 data to a file, using an arbitrary filename and binary flag.
   */
  bool Write(const vtkStdString& FileName, bool WriteBinary = false);

  /**
   * Write an arbitrary array to a file, without using the pipeline.
   */
  static bool Write(vtkArrayData* array, const vtkStdString& file_name, bool WriteBinary = false);

  /**
   * Write input port 0 data to an arbitrary stream.  Note: streams should always be opened in
   * binary mode, to prevent problems reading files on Windows.
   */
  bool Write(ostream& stream, bool WriteBinary = false);

  /**
   * Write arbitrary data to a stream without using the pipeline.  Note: streams should always
   * be opened in binary mode, to prevent problems reading files on Windows.
   */
  static bool Write(vtkArrayData* array, ostream& stream, bool WriteBinary = false);

  /**
   * Write input port 0 data to a string. Note that the WriteBinary argument is not
   * optional in order to not clash with the inherited Write() method.
   */
  vtkStdString Write(bool WriteBinary);

  /**
   * Write arbitrary data to a string without using the pipeline.
   */
  static vtkStdString Write(vtkArrayData* array, bool WriteBinary = false);

protected:
  vtkArrayDataWriter();
  ~vtkArrayDataWriter() override;

  int FillInputPortInformation(int port, vtkInformation* info) override;
  void WriteData() override;

  char* FileName;
  vtkTypeBool Binary;
  bool WriteToOutputString;
  vtkStdString OutputString;

private:
  vtkArrayDataWriter(const vtkArrayDataWriter&) = delete;
  void operator=(const vtkArrayDataWriter&) = delete;
};

#endif
