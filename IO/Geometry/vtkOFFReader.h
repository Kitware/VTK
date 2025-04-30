// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkOFFReader
 * @brief   read Geomview .off files
 *
 * vtkOFFReader is a source object that reads Object File Format .off
 * files. The output of this source object is polygonal data.
 *
 * Please note that this is a very simple reader class that only
 * supports the standard 'OFF' format with 3 vertex coordinates.
 * The maximum number of vertices per face has been limited to 100
 * to simplify error handling. The optional color specification of
 * the polygons is ignored by this reader.
 *
 * This reader supports streaming.
 * When selecting input method, `Stream` has an higher priority than `Filename`.
 * If both are null, reader outputs nothing.
 *
 * The original documentation of the OFF file format can be found here:
 * @sa http://www.geomview.org/docs/html/OFF.html
 *
 * \author Peter Zajac (TU Dortmund)
 */

#ifndef vtkOFFReader_h
#define vtkOFFReader_h

#include "vtkAbstractPolyDataReader.h"
#include "vtkIOGeometryModule.h" // For export macro
#include "vtkResourceStream.h"   // For vtkResourceStream

VTK_ABI_NAMESPACE_BEGIN
class VTKIOGEOMETRY_EXPORT vtkOFFReader : public vtkAbstractPolyDataReader
{
public:
  static vtkOFFReader* New();
  vtkTypeMacro(vtkOFFReader, vtkAbstractPolyDataReader);

protected:
  vtkOFFReader() = default;
  ~vtkOFFReader() override = default;

  int RequestData(vtkInformation*, vtkInformationVector**, vtkInformationVector*) override;

private:
  /**
   * @brief Returns the resource stream to read from
   *
   * If this->Stream is set, then that Stream object will be returned,
   * otherwise a file stream will be created for the file at this->FileName
   * and that file stream will be returned instead.
   */
  vtkSmartPointer<vtkResourceStream> Open();

  vtkOFFReader(const vtkOFFReader&) = delete;
  void operator=(const vtkOFFReader&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
