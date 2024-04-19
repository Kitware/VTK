// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkPostScriptWriter
 * @brief   Writes an image as a PostScript file.
 *
 * vtkPostScriptWriter writes an image as a PostScript file using some
 * reasonable scalings and centered on the page which is assumed to be
 * about 8.5 by 11 inches. This is based loosely off of the code from
 * pnmtops.c. Right now there aren't any real options.
 */

#ifndef vtkPostScriptWriter_h
#define vtkPostScriptWriter_h

#include "vtkIOImageModule.h" // For export macro
#include "vtkImageWriter.h"

VTK_ABI_NAMESPACE_BEGIN
class VTKIOIMAGE_EXPORT vtkPostScriptWriter : public vtkImageWriter
{
public:
  static vtkPostScriptWriter* New();
  vtkTypeMacro(vtkPostScriptWriter, vtkImageWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

protected:
  vtkPostScriptWriter() = default;
  ~vtkPostScriptWriter() override = default;

  void WriteFile(ostream* file, vtkImageData* data, int extent[6], int wExt[6]) override;
  void WriteFileHeader(ostream*, vtkImageData*, int wExt[6]) override;
  void WriteFileTrailer(ostream*, vtkImageData*) override;

private:
  vtkPostScriptWriter(const vtkPostScriptWriter&) = delete;
  void operator=(const vtkPostScriptWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
