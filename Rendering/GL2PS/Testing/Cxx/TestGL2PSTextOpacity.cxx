/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestGL2PSAddPolyPrimitive.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// Tests bad PDF generation for opacity=0.5 and TextAsPath=1

#include "vtk_gl2ps.h"

#include "vtkTestingInteractor.h"

#include <iostream>
#include <string>
#include <vector>
#include <fstream>

#include <vtkOpenGLGL2PSExporter.h>
#include <vtkTextActor.h>
#include <vtkRenderer.h>
#include <vtkRenderWindow.h>
#include <vtkTextActor.h>
#include <vtkTextProperty.h>

#include <vtkPropCollection.h>

int TestGL2PSTextOpacity(int , char * [])
{
  vtkNew<vtkOpenGLGL2PSExporter> exporter;

  vtkNew<vtkTextActor> ta;
  ta->SetPosition(0, 0);
  ta->SetInput("Hello");
  ta->GetTextProperty()->SetColor(0.0, 1.0, 0.0);
  ta->GetTextProperty()->SetOpacity(0.5);
  exporter->SetCompress(0);
  exporter->DrawBackgroundOff();

  vtkNew<vtkRenderer> ren;
  vtkNew<vtkRenderWindow> renWin;

  ren->AddActor(ta.GetPointer());
  renWin->AddRenderer(ren.GetPointer());


  exporter->SetTextAsPath(1);
  exporter->SetInput(renWin.GetPointer());
  exporter->SetFileFormatToPDF();
  exporter->SetFilePrefix("text");
  exporter->Write();

  // write to stdout and test for "ca" at the end of a line
  std::ifstream ifs;
  char line[256];
  ifs.open("text.pdf");
  while (ifs.getline(line, sizeof(line)))
  {
    std::streamsize count = ifs.gcount();
    // bad PDF instruction is a 'ca' string at the end of the line
    // count includes the 0 at the end of the string
    if (count >= 3 && line[count - 3] == 'c' &&
        line[count - 2] == 'a')
      return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
