// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright 2008 Sandia Corporation
// SPDX-License-Identifier: LicenseRef-BSD-3-Clause-Sandia-USGov
/**
 * @class   vtkRISReader
 * @brief   reader for RIS files
 *
 *
 * RIS is a tagged format for expressing bibliographic citations.  Data is
 * structured as a collection of records with each record composed of
 * one-to-many fields.  See
 *
 * http://en.wikipedia.org/wiki/RIS_(file_format)
 * http://www.refman.com/support/risformat_intro.asp
 * http://www.adeptscience.co.uk/kb/article/A626
 *
 * for details.  vtkRISReader will convert an RIS file into a vtkTable, with
 * the set of table columns determined dynamically from the contents of the
 * file.
 */

#ifndef vtkRISReader_h
#define vtkRISReader_h

#include "vtkIOInfovisModule.h" // For export macro
#include "vtkTableAlgorithm.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkTable;

class VTKIOINFOVIS_EXPORT vtkRISReader : public vtkTableAlgorithm
{
public:
  static vtkRISReader* New();
  vtkTypeMacro(vtkRISReader, vtkTableAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Set/get the file to load
   */
  vtkGetFilePathMacro(FileName);
  vtkSetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * Set/get the delimiter to be used for concatenating field data (default: ";")
   */
  vtkGetStringMacro(Delimiter);
  vtkSetStringMacro(Delimiter);
  ///@}

  ///@{
  /**
   * Set/get the maximum number of records to read from the file (zero = unlimited)
   */
  vtkGetMacro(MaxRecords, int);
  vtkSetMacro(MaxRecords, int);
  ///@}

protected:
  vtkRISReader();
  ~vtkRISReader() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  char* FileName;
  char* Delimiter;
  int MaxRecords;

private:
  vtkRISReader(const vtkRISReader&) = delete;
  void operator=(const vtkRISReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
