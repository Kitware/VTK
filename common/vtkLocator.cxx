/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLocator.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-2001 Ken Martin, Will Schroeder, Bill Lorensen 
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

 * Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

 * Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

 * Neither name of Ken Martin, Will Schroeder, or Bill Lorensen nor the names
   of any contributors may be used to endorse or promote products derived
   from this software without specific prior written permission.

 * Modified source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS ``AS IS''
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
ARE DISCLAIMED. IN NO EVENT SHALL THE AUTHORS OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

=========================================================================*/
#include "vtkLocator.h"

// Construct with automatic computation of divisions, averaging
// 25 points per bucket.
vtkLocator::vtkLocator()
{
  this->DataSet = NULL;
  this->MaxLevel = 8;
  this->Level = 8;
  this->Tolerance = 0.001;
  this->Automatic = 1;
  this->RetainCellLists = 1;
}

vtkLocator::~vtkLocator()
{
  // commented out because of compiler problems in g++
  //  this->FreeSearchStructure(); 
  this->SetDataSet(NULL);
}

void vtkLocator::Initialize()
{
  // free up hash table
  this->FreeSearchStructure();
}

void vtkLocator::Update()
{
  if (!this->DataSet)
    {
    vtkErrorMacro(<< "Input not set!");
    return;
    }
  if ((this->MTime > this->BuildTime) ||
      (this->DataSet->GetMTime() > this->BuildTime))
    {
    this->BuildLocator();
    }
}

void vtkLocator::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkObject::PrintSelf(os,indent);

  if ( this->DataSet )
    {
    os << indent << "DataSet: " << this->DataSet << "\n";
    }
  else
    {
    os << indent << "DataSet: (none)\n";
    }

  os << indent << "Automatic: " << (this->Automatic ? "On\n" : "Off\n");
  os << indent << "Tolerance: " << this->Tolerance << "\n" ;
  os << indent << "Level: " << this->Level << "\n" ;
  os << indent << "MaxLevel: " << this->MaxLevel << "\n" ;
  os << indent << "Retain Cell Lists: " << (this->RetainCellLists ? "On\n" : "Off\n");
  os << indent << "Build Time: " << this->BuildTime.GetMTime() << "\n";
}

