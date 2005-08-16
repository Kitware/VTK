/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPEG2Writer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkMPEG2Writer.h"

#include "vtkImageData.h"
#include "vtkMPEG2WriterHelper.h"
#include "vtkObjectFactory.h"

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkMPEG2Writer);
vtkCxxRevisionMacro(vtkMPEG2Writer, "1.1");

//---------------------------------------------------------------------------
vtkMPEG2Writer::vtkMPEG2Writer()
{
  this->MPEG2WriterHelper = vtkMPEG2WriterHelper::New();
}

//---------------------------------------------------------------------------
vtkMPEG2Writer::~vtkMPEG2Writer()
{
  this->MPEG2WriterHelper->Delete();
}

//---------------------------------------------------------------------------
void vtkMPEG2Writer::Start()
{
  this->MPEG2WriterHelper->SetFileName(this->FileName);
  this->MPEG2WriterHelper->Start();
}

//---------------------------------------------------------------------------
void vtkMPEG2Writer::Write()
{
  this->MPEG2WriterHelper->Write();
}

//---------------------------------------------------------------------------
void vtkMPEG2Writer::End()
{
  this->MPEG2WriterHelper->End();
}

//----------------------------------------------------------------------------
void vtkMPEG2Writer::SetInput(vtkImageData *input)
{
  this->Superclass::SetInput(input);
  this->MPEG2WriterHelper->SetInput(input);
}

//----------------------------------------------------------------------------
vtkImageData *vtkMPEG2Writer::GetInput()
{
  return this->MPEG2WriterHelper->GetInput();
}

//---------------------------------------------------------------------------
void vtkMPEG2Writer::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

