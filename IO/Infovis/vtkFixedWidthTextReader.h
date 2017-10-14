/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFixedWidthTextReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

/**
 * @class   vtkFixedWidthTextReader
 * @brief   reader for pulling in text files with fixed-width fields
 *
 *
 *
 * vtkFixedWidthTextReader reads in a table from a text file where
 * each column occupies a certain number of characters.
 *
 * This class emits ProgressEvent for every 100 lines it reads.
 *
 *
 * @warning
 * This first version of the reader will assume that all fields have
 * the same width.  It also assumes that the first line in the file
 * has at least as many fields (i.e. at least as many characters) as
 * any other line in the file.
 *
 * @par Thanks:
 * Thanks to Andy Wilson from Sandia National Laboratories for
 * implementing this class.
*/

#ifndef vtkFixedWidthTextReader_h
#define vtkFixedWidthTextReader_h

#include "vtkIOInfovisModule.h" // For export macro
#include "vtkTableAlgorithm.h"

class vtkCommand;
class vtkTable;

class VTKIOINFOVIS_EXPORT vtkFixedWidthTextReader : public vtkTableAlgorithm
{
public:
  static vtkFixedWidthTextReader* New();
  vtkTypeMacro(vtkFixedWidthTextReader,vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  vtkGetStringMacro(FileName);
  vtkSetStringMacro(FileName);

  //@{
  /**
   * Set/get the field width
   */
  vtkSetMacro(FieldWidth, int);
  vtkGetMacro(FieldWidth, int);
  //@}

  //@{
  /**
   * If set, this flag will cause the reader to strip whitespace from
   * the beginning and ending of each field.  Defaults to off.
   */
  vtkSetMacro(StripWhiteSpace, bool);
  vtkGetMacro(StripWhiteSpace, bool);
  vtkBooleanMacro(StripWhiteSpace, bool);
  //@}

  //@{
  /**
   * Set/get whether to treat the first line of the file as headers.
   */
  vtkGetMacro(HaveHeaders,bool);
  vtkSetMacro(HaveHeaders,bool);
  vtkBooleanMacro(HaveHeaders, bool);
  //@}

  //@{
  /**
   * Set/get the ErrorObserver for the internal vtkTable
   * This is useful for applications that want to catch error messages.
   */
  void SetTableErrorObserver(vtkCommand *);
  vtkGetObjectMacro(TableErrorObserver,vtkCommand);
  //@}

 protected:
  vtkFixedWidthTextReader();
  ~vtkFixedWidthTextReader() override;

  int RequestData(
    vtkInformation*,
    vtkInformationVector**,
    vtkInformationVector*) override;

  void OpenFile();

  char* FileName;
  bool HaveHeaders;
  bool StripWhiteSpace;
  int FieldWidth;

private:
  vtkFixedWidthTextReader(const vtkFixedWidthTextReader&) = delete;
  void operator=(const vtkFixedWidthTextReader&) = delete;
  vtkCommand *TableErrorObserver;
};

#endif

