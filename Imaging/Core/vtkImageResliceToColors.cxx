/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageResliceToColors.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkImageResliceToColors.h"

#include "vtkImageData.h"
#include "vtkInformation.h"
#include "vtkInformationVector.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkStreamingDemandDrivenPipeline.h"
#include "vtkTypeTraits.h"
#include "vtkLookupTable.h"

#include "vtkTemplateAliasMacro.h"
// turn off 64-bit ints when templating over all types
# undef VTK_USE_INT64
# define VTK_USE_INT64 0
# undef VTK_USE_UINT64
# define VTK_USE_UINT64 0

#include <climits>
#include <float.h>
#include <math.h>

vtkStandardNewMacro(vtkImageResliceToColors);
vtkCxxSetObjectMacro(vtkImageResliceToColors,LookupTable,vtkScalarsToColors);

//----------------------------------------------------------------------------
vtkImageResliceToColors::vtkImageResliceToColors()
{
  this->HasConvertScalars = 1;
  this->LookupTable = NULL;
  this->DefaultLookupTable = NULL;
  this->OutputFormat = VTK_RGBA;
  this->Bypass = 0;
}

//----------------------------------------------------------------------------
vtkImageResliceToColors::~vtkImageResliceToColors()
{
  if (this->LookupTable)
    {
    this->LookupTable->Delete();
    }
  if (this->DefaultLookupTable)
    {
    this->DefaultLookupTable->Delete();
    }
}

//----------------------------------------------------------------------------
void vtkImageResliceToColors::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  os << indent << "LookupTable: " << this->GetLookupTable() << "\n";
  os << indent << "OutputFormat: " <<
    (this->OutputFormat == VTK_RGBA ? "RGBA" :
     (this->OutputFormat == VTK_RGB ? "RGB" :
      (this->OutputFormat == VTK_LUMINANCE_ALPHA ? "LuminanceAlpha" :
       (this->OutputFormat == VTK_LUMINANCE ? "Luminance" : "Unknown"))))
    << "\n";
  os << indent << "Bypass: " << (this->Bypass ? "On\n" : "Off\n");
}

//----------------------------------------------------------------------------
unsigned long int vtkImageResliceToColors::GetMTime()
{
  unsigned long mTime=this->Superclass::GetMTime();
  unsigned long time;

  if (this->LookupTable && !this->Bypass)
    {
    time = this->LookupTable->GetMTime();
    mTime = ( time > mTime ? time : mTime );
    }

  return mTime;
}

//----------------------------------------------------------------------------
void vtkImageResliceToColors::SetBypass(int bypass)
{
  bypass = (bypass != 0);
  if (bypass != this->Bypass)
    {
    this->Bypass = bypass;
    if (bypass)
      {
      this->HasConvertScalars = 0;
      this->OutputScalarType = VTK_FLOAT;
      }
    else
      {
      this->HasConvertScalars = 1;
      this->OutputScalarType = -1;
      }
    }
}

//----------------------------------------------------------------------------
int vtkImageResliceToColors::ConvertScalarInfo(
  int &scalarType, int &numComponents)
{
  switch (this->OutputFormat)
    {
    case VTK_LUMINANCE:
      numComponents = 1;
      break;
    case VTK_LUMINANCE_ALPHA:
      numComponents = 2;
      break;
    case VTK_RGB:
      numComponents = 3;
      break;
    case VTK_RGBA:
      numComponents = 4;
      break;
    }

  scalarType = VTK_UNSIGNED_CHAR;

  // This is always called before ConvertScalars, and is
  // not called multi-threaded, so set up default table here
  if (!this->LookupTable && !this->DefaultLookupTable)
    {
    // Build a default greyscale lookup table
    this->DefaultLookupTable = vtkScalarsToColors::New();
    this->DefaultLookupTable->SetRange(0.0, 255.0);
    this->DefaultLookupTable->SetVectorModeToRGBColors();
    }

  return 1;
}

//----------------------------------------------------------------------------
void vtkImageResliceToColors::ConvertScalars(
  void *inPtr, void *outPtr, int inputType, int inputComponents, int count,
  int vtkNotUsed(idX), int vtkNotUsed(idY), int vtkNotUsed(idZ),
  int vtkNotUsed(threadId))
{
  vtkScalarsToColors *table = this->LookupTable;
  if (!table)
    {
    table = this->DefaultLookupTable;
    }

  if (inputComponents == 1 && this->LookupTable)
    {
    table->MapScalarsThroughTable(
      inPtr, static_cast<unsigned char *>(outPtr),
      inputType, count, inputComponents, this->OutputFormat);
    }
  else
    {
    table->MapVectorsThroughTable(
      inPtr, static_cast<unsigned char *>(outPtr),
      inputType, count, inputComponents, this->OutputFormat);
    }
}
