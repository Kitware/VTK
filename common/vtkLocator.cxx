/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLocator.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1998 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


=========================================================================*/
#include "vtkLocator.h"

// Description:
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
}

void vtkLocator::Initialize()
{
  // free up hash table
  this->FreeSearchStructure();
}

void vtkLocator::Update()
{
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

