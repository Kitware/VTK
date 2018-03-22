/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPDFExporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class vtkPDFExporter
 * @brief Exports vtkContext2D scenes to PDF.
 *
 * This exporter draws context2D scenes into a PDF file.
 *
 * If ActiveRenderer is specified then it exports contents of
 * ActiveRenderer. Otherwise it exports contents of all renderers.
 */

#ifndef vtkPDFExporter_h
#define vtkPDFExporter_h

#include "vtkIOExportPDFModule.h" // For export macro
#include "vtkExporter.h"

class vtkContextActor;
class vtkRenderer;

class VTKIOEXPORTPDF_EXPORT vtkPDFExporter: public vtkExporter
{
public:
  static vtkPDFExporter* New();
  vtkTypeMacro(vtkPDFExporter, vtkExporter)
  void PrintSelf(ostream &os, vtkIndent indent) override;

  /** The title of the exported document. @{ */
  vtkSetStringMacro(Title)
  vtkGetStringMacro(Title)
  /** @} */

  /** The name of the exported file. @{ */
  vtkSetStringMacro(FileName)
  vtkGetStringMacro(FileName)
  /** @} */

protected:
  vtkPDFExporter();
  ~vtkPDFExporter() override;

  void WriteData() override;

  void WritePDF();
  void PrepareDocument();
  void RenderContextActors();
  void RenderContextActor(vtkContextActor *actor,
                          vtkRenderer *renderer);

  char *Title;
  char *FileName;

private:
  vtkPDFExporter(const vtkPDFExporter&) = delete;
  void operator=(const vtkPDFExporter&) = delete;

  struct Details;
  Details *Impl;
};

#endif // vtkPDFExporter_h
