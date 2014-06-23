/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestColorString.cxx

  Copyright (c) Marco Cecchetti
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/


#include "vtkColorString.h"
#include "vtkSmartPointer.h"
#include "vtkTestDriver.h"

#include <iostream>


struct Data
{
  const char* inputString;
  unsigned char expectedOutput[4];
};


int TestColorString(int vtkNotUsed(argc), char* vtkNotUsed(argv)[])
{

  bool testResult = true;

  static const Data dataList[] = {
      // Valid hexadecimal string.
      { "#70faCC", {0x70, 0xFA, 0xCC, 0xFF} },
      { " #70faCC ", {0x70, 0xFA, 0xCC, 0xFF} },
      { "#70faCCF0", {0x70, 0xFA, 0xCC, 0xF0} },
      { " #70faCCF0 ", {0x70, 0xFA, 0xCC, 0xF0} },

      // Non-valid hexadecimal string.
      { "#", {0, 0, 0, 0} },
      { "#70f", {0, 0, 0, 0} },
      { "#70faCCF088", {0, 0, 0, 0} },
      { "# 70faCCF0", {0, 0, 0, 0} },
      { "#70 faCCF0", {0, 0, 0, 0} },
      { "#70f aCCF0", {0, 0, 0, 0} },
      { "#70faCC w", {0, 0, 0, 0} },

      // Valid rgb() string.
      { "rgb ( 020, 0 , 255 ) ", {20, 0, 255, 255} },
      { "rgb(20,0,255)", {20, 0, 255, 255} },

      // Non-valid rgb() string.
      { "rgb (20, 0 , 2558)", {0, 0, 0, 0} },
      { "rgb (20, 0 , 25, 58)", {0, 0, 0, 0} },
      { "rgb (  ", {0, 0, 0, 0} },
      { "rgb(0, 0 , 256)", {0, 0, 0, 0} },

      // Valid rgba() string.
      { "rgba ( 020, 0 , 255, 3 )", {20, 0, 255, 3} },

      // Non-valid rgba() string.
      { "rgba(20, 0 , 255)", {0, 0, 0, 0} },

      // Valid named color string.
      { "steelblue", {70, 130, 180, 255} },

      // Non-valid color string.
      { "xcnvvb", {0, 0, 0, 0} },
      { "", {0, 0, 0, 0} },

      // End element.
      { "\n",  {0, 0, 0, 0} },
  };

  vtkSmartPointer<vtkColorString> color = vtkSmartPointer<vtkColorString>::New();

  const char* inputString = "";
  const unsigned char* expectedOutput;
  vtkColor4ub outputColor;
  unsigned int i = 0;
  while (inputString[0] != '\n' )
    {
    inputString = dataList[i].inputString;
    expectedOutput = dataList[i].expectedOutput;
    color->SetColor(inputString);
    color->GetColor(outputColor);
    if (outputColor != vtkColor4ub(expectedOutput))
      {
      vtkGenericWarningMacro(
        << "Fail: TestColorString()"
        << ", input `" <<  inputString << "`"
        << ", found " << outputColor
        << ", expected " << vtkColor4ub(expectedOutput) << " instead."
        );
      testResult &= false;
      }
    ++i;
    }

  if ( !testResult )
    {
    return EXIT_FAILURE;
    }
  return EXIT_SUCCESS;

}
