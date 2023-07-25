// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOBJReader
 * @brief   read Wavefront .obj files
 *
 * vtkOBJReader is a source object that reads Wavefront .obj
 * files. The output of this source object is polygonal data.
 * @sa
 * vtkOBJImporter
 */

#ifndef vtkOBJReader_h
#define vtkOBJReader_h

#include "vtkAbstractPolyDataReader.h"
#include "vtkIOGeometryModule.h" // For export macro
#include "vtkResourceStream.h"   // For vtkResourceStream

VTK_ABI_NAMESPACE_BEGIN
class VTKIOGEOMETRY_EXPORT vtkOBJReader : public vtkAbstractPolyDataReader
{
public:
  static vtkOBJReader* New();
  vtkTypeMacro(vtkOBJReader, vtkAbstractPolyDataReader);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  /**
   * Get first comment in the file.
   * Comment may be multiple lines. # and leading spaces are removed.
   */
  vtkGetStringMacro(Comment);

  ///@{
  /**
   * Specify stream to read from
   * When selecting input method, `Stream` has an higher priority than `Filename`.
   * If both are null, reader outputs nothing.
   */
  vtkSetSmartPointerMacro(Stream, vtkResourceStream);
  vtkGetSmartPointerMacro(Stream, vtkResourceStream);
  ///@}

protected:
  vtkOBJReader();
  ~vtkOBJReader() override;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

  /**
   * Set comment string. Internal use only.
   */
  vtkSetStringMacro(Comment);

  char* Comment;
  vtkSmartPointer<vtkResourceStream> Stream;

private:
  vtkSmartPointer<vtkResourceStream> Open();

  vtkOBJReader(const vtkOBJReader&) = delete;
  void operator=(const vtkOBJReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
