/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageFileReader.cxx
  Language:  C++
  Date:      $Date$
  Version:   $Revision$
  Thanks:    Thanks to C. Charles Law who developed this class.

Copyright (c) 1993-1995 Ken Martin, Will Schroeder,ill Lorensen.

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
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include "vtkImageFileReader.h"
#include "vtkImageCache.h"

//----------------------------------------------------------------------------
vtkImageFileReader::vtkImageFileReader()
{
}

//----------------------------------------------------------------------------
vtkImageFileReader::~vtkImageFileReader()
{ 
}

//----------------------------------------------------------------------------
void vtkImageFileReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageReader::PrintSelf(os,indent);
}

//----------------------------------------------------------------------------
// Description:
// This function opens the first file to determine the header size.
void vtkImageFileReader::Initialize()
{
  int idx;
  
  if (this->Initialized)
    {
    return;
    }
  
  // Call the superclass initialize
  this->vtkImageReader::Initialize();
  
  // Set the FileExtent here since it is constant.
  for (idx = 0; idx < VTK_IMAGE_EXTENT_DIMENSIONS; ++idx)
    {
    this->FileExtent[idx] = this->DataExtent[idx];
    }
  
  this->Initialized = 1;
}

//----------------------------------------------------------------------------
// Description:
// This method sets the file name.
void vtkImageFileReader::SetFileName(char *name)
{
  if (this->FileName)
    {
    delete [] this->FileName;
    }
  this->FileName = new char[strlen(name) + 1];
  strcpy(this->FileName, name);
  this->Initialized = 0;
  this->Modified();
}


//----------------------------------------------------------------------------
// Description:
// This function is called by the cache to update a region.
void vtkImageFileReader::UpdatePointData(vtkImageRegion *region)
{
  if ( ! this->Initialized)
    {
    this->Initialize();
    }
  // The file has already been opened, and FileExtent has been set
  // by the initialize method.
  this->UpdateFromFile(region);
}



