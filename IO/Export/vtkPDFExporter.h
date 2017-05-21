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
 */

#ifndef vtkPDFExporter_h
#define vtkPDFExporter_h

#include "vtkIOExportModule.h" // For export macro
#include "vtkExporter.h"

class vtkContextActor;
class vtkRenderer;

class VTKIOEXPORT_EXPORT vtkPDFExporter: public vtkExporter
{
public:
  static vtkPDFExporter* New();
  vtkTypeMacro(vtkPDFExporter, vtkExporter)
  void PrintSelf(ostream &os, vtkIndent indent) VTK_OVERRIDE;

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
  ~vtkPDFExporter() VTK_OVERRIDE;

  void WriteData() VTK_OVERRIDE;

  void WritePDF();
  void PrepareDocument();
  void RenderContextActors();
  void RenderContextActor(vtkContextActor *actor,
                          vtkRenderer *renderer);

  char *Title;
  char *FileName;

private:
  vtkPDFExporter(const vtkPDFExporter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkPDFExporter&) VTK_DELETE_FUNCTION;

  struct Details;
  Details *Impl;
};

#endif // vtkPDFExporter_h
