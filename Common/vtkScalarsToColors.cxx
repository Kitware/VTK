/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkScalarsToColors.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkScalarsToColors.h"

#include <math.h>

vtkCxxRevisionMacro(vtkScalarsToColors, "1.17");

// do not use SetMacro() because we do not the table to rebuild.
void vtkScalarsToColors::SetAlpha(float alpha)
{
  this->Alpha = (alpha < 0.0 ? 0.0 : (alpha > 1.0 ? 1.0 : alpha));
}

vtkUnsignedCharArray *vtkScalarsToColors::MapScalars(vtkDataArray *scalars,
                                                     int colorMode, int comp)
{
  vtkUnsignedCharArray *newColors;
  vtkUnsignedCharArray *colors;

  // map scalars through lookup table only if needed
  if ( colorMode == VTK_COLOR_MODE_DEFAULT && 
       (colors=vtkUnsignedCharArray::SafeDownCast(scalars)) != NULL )
    {
    newColors = this->
      ConvertUnsignedCharToRGBA(colors, colors->GetNumberOfComponents(),
                                scalars->GetNumberOfTuples());
    }
  else
    {
    newColors = vtkUnsignedCharArray::New();
    newColors->SetNumberOfComponents(4);
    newColors->SetNumberOfTuples(scalars->GetNumberOfTuples());
    this->
      MapScalarsThroughTable2(scalars->GetVoidPointer(comp), 
                              newColors->GetPointer(0),
                              scalars->GetDataType(),
                              scalars->GetNumberOfTuples(),
                              scalars->GetNumberOfComponents(), 
                              VTK_RGBA);
    }//need to map

  return newColors;
}

// Map a set of scalar values through the table
void vtkScalarsToColors::MapScalarsThroughTable(vtkDataArray *scalars, 
                                                unsigned char *output,
                                                int outputFormat)
{
  switch (outputFormat)
    {
    case VTK_RGBA:
    case VTK_RGB:
    case VTK_LUMINANCE_ALPHA:
    case VTK_LUMINANCE:
      break;
    default:
      vtkErrorMacro(<< "MapScalarsThroughTable: unrecognized color format");
      break;
    }

  this->MapScalarsThroughTable2(scalars->GetVoidPointer(0),
                                output,
                                scalars->GetDataType(),
                                scalars->GetNumberOfTuples(),
                                scalars->GetNumberOfComponents(),
                                outputFormat);
}

vtkUnsignedCharArray *vtkScalarsToColors::ConvertUnsignedCharToRGBA(
  vtkUnsignedCharArray *colors, int numComp, int numTuples)
{
  if ( numComp == 4 && this->Alpha >= 1.0 )
    {
    colors->Register(this);
    return colors;
    }
    
  unsigned char *cptr = colors->GetPointer(0);
  vtkUnsignedCharArray *newColors = vtkUnsignedCharArray::New();
  newColors->SetNumberOfComponents(4);
  newColors->SetNumberOfTuples(numTuples);
  unsigned char *nptr = newColors->GetPointer(0);
  int i;

  if ( this->Alpha >= 1.0 )
    {
    switch (numComp)
      {
      case 1:
        for (i=0; i<numTuples; i++)
          {
          *nptr++ = *cptr;
          *nptr++ = *cptr;
          *nptr++ = *cptr++;
          *nptr++ = 255;
          }
        break;

      case 2:
        for (i=0; i<numTuples; i++)
          {
          *nptr++ = *cptr;
          *nptr++ = *cptr;
          *nptr++ = *cptr++;
          *nptr++ = *cptr++;
          }
        break;

      case 3:
        for (i=0; i<numTuples; i++)
          {
          *nptr++ = *cptr++;
          *nptr++ = *cptr++;
          *nptr++ = *cptr++;
          *nptr++ = 255;
          }
        break;

      default:
        vtkErrorMacro(<<"Cannot convert colors");
        return NULL;
      }
    }
  else //blending required
    {
    unsigned char alpha;
    switch (numComp)
      {
      case 1:
        alpha = static_cast<unsigned char>(this->Alpha*255);
        for (i=0; i<numTuples; i++)
          {
          *nptr++ = *cptr;
          *nptr++ = *cptr;
          *nptr++ = *cptr++;
          *nptr++ = alpha;
          }
        break;

      case 2:
        for (i=0; i<numTuples; i++)
          {
          *nptr++ = *cptr;
          *nptr++ = *cptr;
          *nptr++ = *cptr++;
          *nptr++ = static_cast<unsigned char>((*cptr)*this->Alpha); cptr++;
          }
        break;

      case 3:
        alpha = static_cast<unsigned char>(this->Alpha*255);
        for (i=0; i<numTuples; i++)
          {
          *nptr++ = *cptr++;
          *nptr++ = *cptr++;
          *nptr++ = *cptr++;
          *nptr++ = alpha;
          }
        break;

      case 4:
        for (i=0; i<numTuples; i++)
          {
          *nptr++ = *cptr++;
          *nptr++ = *cptr++;
          *nptr++ = *cptr++;
          *nptr++ = static_cast<unsigned char>((*cptr)*this->Alpha); cptr++;
          }
        break;

      default:
        vtkErrorMacro(<<"Cannot convert colors");
        return NULL;
      }
    }
  
  return newColors;
}


void vtkScalarsToColors::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "Alpha: " << this->Alpha << endl;
}
