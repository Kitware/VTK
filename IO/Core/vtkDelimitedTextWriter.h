/*=========================================================================

  Program:   ParaView
  Module:    vtkDelimitedTextWriter.h

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

/**
 * @class   vtkDelimitedTextWriter
 * @brief   Delimited text writer for vtkTable
 * Writes a vtkTable as a delimited text file (such as CSV).
*/

#ifndef vtkDelimitedTextWriter_h
#define vtkDelimitedTextWriter_h

#include "vtkIOCoreModule.h" // For export macro
#include "vtkWriter.h"

class vtkStdString;
class vtkTable;

class VTKIOCORE_EXPORT vtkDelimitedTextWriter : public vtkWriter
{
public:
  static vtkDelimitedTextWriter* New();
  vtkTypeMacro(vtkDelimitedTextWriter, vtkWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Get/Set the delimiter use to separate fields ("," by default.)
   */
  vtkSetStringMacro(FieldDelimiter);
  vtkGetStringMacro(FieldDelimiter);
  //@}

  //@{
  /**
   * Get/Set the delimiter used for string data, if any
   * eg. double quotes(").
   */
  vtkSetStringMacro(StringDelimiter);
  vtkGetStringMacro(StringDelimiter);
  //@}

  //@{
  /**
   * Get/Set the filename for the file.
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  //@{
  /**
   * Get/Set if StringDelimiter must be used for string data.
   * True by default.
   */
  vtkSetMacro(UseStringDelimiter, bool);
  vtkGetMacro(UseStringDelimiter, bool);
  //@}

  //@{
  /**
   * Enable writing to an OutputString instead of the default, a file.
   */
  vtkSetMacro(WriteToOutputString,bool);
  vtkGetMacro(WriteToOutputString,bool);
  vtkBooleanMacro(WriteToOutputString,bool);
  //@}

  /**
   * This convenience method returns the string, sets the IVAR to NULL,
   * so that the user is responsible for deleting the string.
   */
  char *RegisterAndGetOutputString();

  /**
   * Internal method: Returns the "string" with the "StringDelimiter" if
   * UseStringDelimiter is true.
   */
  vtkStdString GetString(vtkStdString string);

protected:
  vtkDelimitedTextWriter();
  ~vtkDelimitedTextWriter();

  bool WriteToOutputString;
  char* OutputString;

  bool OpenStream();

  virtual void WriteData();
  virtual void WriteTable(vtkTable* rectilinearGrid);

  // see algorithm for more info.
  // This writer takes in vtkTable.
  virtual int FillInputPortInformation(int port, vtkInformation* info);

  char* FileName;
  char* FieldDelimiter;
  char* StringDelimiter;
  bool UseStringDelimiter;

  ostream* Stream;

private:
  vtkDelimitedTextWriter(const vtkDelimitedTextWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkDelimitedTextWriter&) VTK_DELETE_FUNCTION;
};



#endif

