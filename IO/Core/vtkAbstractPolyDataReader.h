// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkAbstractPolyDataReader
 * @brief   Superclass for algorithms that read
 * models from a file.
 *
 * This class allows to use a single base class to manage AbstractPolyData
 * reader classes in a uniform manner without needing to know the actual
 * type of the reader.
 * i.e. makes it possible to create maps to associate filename extension
 * and vtkAbstractPolyDataReader object.
 *
 * @sa
 * vtkOBJReader vtkOFFReader vtkPLYReader vtkSTLReader
 */

#ifndef vtkAbstractPolyDataReader_h
#define vtkAbstractPolyDataReader_h

#include "vtkIOCoreModule.h" // For export macro
#include "vtkPolyDataAlgorithm.h"
#include "vtkResourceStream.h" // For vtkResourceStream
#include "vtkSmartPointer.h"   // For vtkSmartPointer

VTK_ABI_NAMESPACE_BEGIN
class VTKIOCORE_EXPORT vtkAbstractPolyDataReader : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkAbstractPolyDataReader, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * Specify file name of AbstractPolyData file (obj / off / ply / stl).
   */
  vtkSetFilePathMacro(FileName);
  vtkGetFilePathMacro(FileName);
  ///@}

  ///@{
  /**
   * Specify stream to read from
   * When both `Stream` and `Filename` are set, it's left to the implementation to determine which
   * one is used. If both are null, reader outputs nothing.
   */
  vtkSetSmartPointerMacro(Stream, vtkResourceStream);
  vtkGetSmartPointerMacro(Stream, vtkResourceStream);
  ///@}

protected:
  vtkAbstractPolyDataReader();
  ~vtkAbstractPolyDataReader() override;

  char* FileName;
  vtkSmartPointer<vtkResourceStream> Stream;

private:
  vtkAbstractPolyDataReader(const vtkAbstractPolyDataReader&) = delete;
  void operator=(const vtkAbstractPolyDataReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
