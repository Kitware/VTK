/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkArrayReader.h

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
 * @class   vtkArrayReader
 * @brief    Reads sparse and dense vtkArray data written by vtkArrayWriter.
 *
 *
 * Reads sparse and dense vtkArray data written with vtkArrayWriter.
 *
 * Outputs:
 *   Output port 0: vtkArrayData containing a dense or sparse array.
 *
 * @sa
 * vtkArrayWriter
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
*/

#ifndef vtkArrayReader_h
#define vtkArrayReader_h

#include "vtkIOCoreModule.h" // For export macro
#include "vtkArrayDataAlgorithm.h"

class VTKIOCORE_EXPORT vtkArrayReader :
  public vtkArrayDataAlgorithm
{
public:
  static vtkArrayReader* New();
  vtkTypeMacro(vtkArrayReader, vtkArrayDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Set the filesystem location from which data will be read.
   */
  vtkGetStringMacro(FileName);
  vtkSetStringMacro(FileName);
  //@}

  //@{
  /**
   * The input string to parse. If you set the input string, you must also set
   * the ReadFromInputString flag to parse the string instead of a file.
   */
  virtual void SetInputString(const vtkStdString& string);
  virtual vtkStdString GetInputString();
  //@}

  //@{
  /**
   * Whether to read from an input string as opposed to a file, which is the default.
   */
  vtkSetMacro(ReadFromInputString, bool);
  vtkGetMacro(ReadFromInputString, bool);
  vtkBooleanMacro(ReadFromInputString, bool);
  //@}

  /**
   * Read an arbitrary array from a stream.  Note: you MUST always
   * open streams in binary mode to prevent problems reading files
   * on Windows.
   */
  static vtkArray* Read(istream& stream);

  /**
   * Read an arbitrary array from a string.
   */
  static vtkArray* Read(vtkStdString str);

protected:
  vtkArrayReader();
  ~vtkArrayReader();

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*);

  char* FileName;
  vtkStdString InputString;
  bool ReadFromInputString;

private:
  vtkArrayReader(const vtkArrayReader&) VTK_DELETE_FUNCTION;
  void operator=(const vtkArrayReader&) VTK_DELETE_FUNCTION;
};

#endif

