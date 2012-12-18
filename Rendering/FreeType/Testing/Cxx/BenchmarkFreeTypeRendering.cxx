/*=========================================================================

  Program:   Visualization Toolkit
  Module:    BenchmarkFreeTypeRendering.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "vtkFreeTypeTools.h"
#include "vtkFreeTypeUtilities.h"
#include "vtkTextRenderer.h"

#include "vtkImageData.h"
#include "vtkNew.h"
#include "vtkStdString.h"
#include "vtkTextProperty.h"

#include <ctime>
#include <cstdio>

//----------------------------------------------------------------------------
int BenchmarkFreeTypeRendering(int , char *[])
{
  clock_t startT;
  clock_t endT;
  double elapsedSecs;
  double minSecs;
  double maxSecs;
  double aveSecs;

  vtkFreeTypeTools *ftTools = vtkFreeTypeTools::GetInstance();
  vtkFreeTypeUtilities *ftUtils = vtkFreeTypeUtilities::GetInstance();
  vtkNew<vtkTextProperty> tprop;
  vtkNew<vtkTextRenderer> textRenderer;

  vtkStdString str;
  for (int i = 0; i < 500; ++i)
    {
    str += "I'm a test string!\n";
    }

  elapsedSecs = 0;
  minSecs = VTK_DOUBLE_MAX;
  maxSecs = 0;
  aveSecs = 0;
  for (int i = 0; i < 100; ++i)
    {
    // Set the font to something different to throw off the FT caches.
    tprop->SetFontFamily(i % VTK_UNKNOWN_FONT);
    tprop->SetFontSize(i % 50 + 10);
    tprop->SetBold(i % 2);
    tprop->SetItalic((i % 51) % 2);
    tprop->SetOrientation(static_cast<double>(i) * (360.0 / 100.0));

    // Reset the image data
    vtkNew<vtkImageData> image;

    // Clock the time needed to render the string.
    startT = clock();
    ftTools->RenderString(tprop.GetPointer(), str, image.GetPointer());
    endT = clock();
    double iterSecs = static_cast<double>(endT - startT) /
        static_cast<double>(CLOCKS_PER_SEC);
    elapsedSecs += iterSecs;
    minSecs = std::min(minSecs, iterSecs);
    maxSecs = std::max(maxSecs, iterSecs);
    }
  aveSecs = elapsedSecs / 100.0;
  printf("FTTools Total: %9.5f Min: %9.5f Max: %9.5f Ave: %9.5f\n",
         elapsedSecs, minSecs, maxSecs, aveSecs);

  elapsedSecs = 0;
  minSecs = VTK_DOUBLE_MAX;
  maxSecs = 0;
  aveSecs = 0;
  for (int i = 0; i < 100; ++i)
    {
    // Set the font to something different to throw off the FT face cache.
    tprop->SetFontFamily(i % VTK_UNKNOWN_FONT);
    tprop->SetFontSize(i % 50 + 10);
    tprop->SetBold(i % 2);
    tprop->SetItalic((i % 51) % 2);
    tprop->SetOrientation(static_cast<double>(i) * (360.0 / 100.0));

    // Reset the image data
    vtkNew<vtkImageData> image;

    // Clock the time needed to render the string.
    startT = clock();
    textRenderer->RenderString(tprop.GetPointer(), str, image.GetPointer());
    endT = clock();
    double iterSecs = static_cast<double>(endT - startT) /
        static_cast<double>(CLOCKS_PER_SEC);
    elapsedSecs += iterSecs;
    minSecs = std::min(minSecs, iterSecs);
    maxSecs = std::max(maxSecs, iterSecs);
    }
  aveSecs = elapsedSecs / 100.0;
  printf("TextRen Total: %9.5f Min: %9.5f Max: %9.5f Ave: %9.5f\n",
         elapsedSecs, minSecs, maxSecs, aveSecs);

  elapsedSecs = 0;
  minSecs = VTK_DOUBLE_MAX;
  maxSecs = 0;
  aveSecs = 0;
  for (int i = 0; i < 100; ++i)
    {
    // Set the font to something different to throw off the FT face cache.
    tprop->SetFontFamily(i % VTK_UNKNOWN_FONT);
    tprop->SetFontSize(i % 50 + 10);
    tprop->SetBold(i % 2);
    tprop->SetItalic((i % 51) % 2);
    tprop->SetOrientation(static_cast<double>(i) * (360.0 / 100.0));

    // Reset the image data
    vtkNew<vtkImageData> image;

    // Clock the time needed to render the string.
    startT = clock();
    ftUtils->RenderString(tprop.GetPointer(), str, image.GetPointer());
    endT = clock();
    double iterSecs = static_cast<double>(endT - startT) /
        static_cast<double>(CLOCKS_PER_SEC);
    elapsedSecs += iterSecs;
    minSecs = std::min(minSecs, iterSecs);
    maxSecs = std::max(maxSecs, iterSecs);
    }
  aveSecs = elapsedSecs / 100.0;
  printf("FTUtils Total: %9.5f Min: %9.5f Max: %9.5f Ave: %9.5f\n",
         elapsedSecs, minSecs, maxSecs, aveSecs);

  return EXIT_SUCCESS;
}
