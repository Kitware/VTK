/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkImageSeriesReader.cxx
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
#include "vtkImageSeriesReader.h"



//----------------------------------------------------------------------------
vtkImageSeriesReader::vtkImageSeriesReader()
{
  this->FilePrefix = NULL;
  this->FilePattern = NULL;
  
  this->SetFilePattern("%s.%d");
  this->First = 1;
}

//----------------------------------------------------------------------------
vtkImageSeriesReader::~vtkImageSeriesReader()
{ 
  if (this->FilePrefix)
    {
    delete [] this->FilePrefix;
    this->FilePrefix = NULL;
    }

  if (this->FilePattern)
    {
    delete [] this->FilePattern;
    this->FilePattern = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkImageSeriesReader::PrintSelf(ostream& os, vtkIndent indent)
{
  vtkImageReader::PrintSelf(os,indent);

  if (this->FilePrefix)
    {
    os << indent << "FilePrefix: " << this->FilePrefix << "\n";
    }
  
  if (this->FilePattern)
    {
    os << indent << "FilePattern: " << this->FilePattern << "\n";
    }

  os << indent << "First: " << this->First << "\n";
}



//----------------------------------------------------------------------------
// Description:
// This function opens the first file to determine the header size.
void vtkImageSeriesReader::Initialize()
{
  int idx;
  
  if (this->Initialized)
    {
    return;
    }
  
  if ( ! this->FilePattern || ! this->FilePrefix)
    {
    vtkErrorMacro(<< "Initialize: No FilePrefix/Pattern.");
    return;
    }
  
  // Allocate the space for the filename
  if (this->FileName)
    {
    delete [] this->FileName;
    }
  this->FileName = new char[strlen(this->FilePrefix) 
			   + strlen(this->FilePattern) + 50]; // 50 for number

  // Set the file name to the first image.
  sprintf(this->FileName, this->FilePattern, this->FilePrefix, this->First);
  
  // call the superclass initialize.
  this->vtkImageReader::Initialize();

  // Set two axes of FileExtent here since they are constant.
  for (idx = 0; idx < 4; ++idx)
    {
    this->FileExtent[idx] = this->DataExtent[idx];
    }  
  
  // some increments are invalid because files are 2D images.
  for (idx = 3; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    this->FileIncrements[idx] = this->FileIncrements[2];
    }
  
  // Recompute the header size.
  this->HeaderSize = this->FileSize - this->FileIncrements[4];
  vtkDebugMacro(<< "Initialize: Header " << this->HeaderSize << " bytes");
  
  this->Initialized = 1;
}

//----------------------------------------------------------------------------
// Description:
// This method sets the file prefix.
void vtkImageSeriesReader::SetFilePrefix(char *prefix)
{
  if (this->FilePrefix)
    {
    delete [] this->FilePrefix;
    }
  this->FilePrefix = new char[strlen(prefix) + 1];
  strcpy(this->FilePrefix, prefix);
  this->Initialized = 0;
  this->Modified();
}

//----------------------------------------------------------------------------
// Description:
// This method sets the file pattern.
void vtkImageSeriesReader::SetFilePattern(char *pattern)
{
  if (this->FilePattern)
    {
    delete [] this->FilePattern;
    }
  this->FilePattern = new char[strlen(pattern) + 1];
  strcpy(this->FilePattern, pattern);
  this->Initialized = 0;
  this->Modified();
}


//----------------------------------------------------------------------------
// Description:
// This function is called by the cache to update a region.
// It splits up the request into 2D images, and then reads the images 
// from the files.  The axis[2] file numbers loop before the axis[3].
// At the moment, it can only handle 4d data sets.
void vtkImageSeriesReader::UpdatePointData(vtkImageRegion *region)
{
  int idx2, idx3;
  int min2, max2,  min3, max3;
  int *axis;
  int saveExtent[VTK_IMAGE_EXTENT_DIMENSIONS];
  int fileInc3;  // how to increment the file numbers
  int fileNumber;
  
  if ( ! this->Initialized)
    {
    this->Initialize();
    }
  
  // Save the extent of the original region.
  region->GetExtent(saveExtent);
  
  // Increments used to compute file number. the dimensions of axis2.
  fileInc3 = this->DataExtent[5] - this->DataExtent[4] + 1;

  // Get the extent of the extra axes. (Needed to loop over images.)
  axis = region->GetAxes();
  region->GetAxisExtent(axis[2], min2, max2);
  region->GetAxisExtent(axis[3], min3, max3);
  
  // loop over images
  for (idx3 = min3; idx3 <= max3; ++idx3)
    {
    region->SetAxisExtent(axis[3], idx3, idx3);
    this->FileExtent[6] = this->FileExtent[7] = idx3;
    for (idx2 = min2; idx2 <= max2; ++idx2)
      {
      region->SetAxisExtent(axis[2], idx2, idx2);
      this->FileExtent[4] = this->FileExtent[5] = idx2;
    
      // compute the file number (I do not like this. Too messy)
      fileNumber = this->First + (idx2 - this->DataExtent[4])
	+ fileInc3 * (idx3 - this->DataExtent[6]);
      
      // Compute the file name.
      sprintf(this->FileName, this->FilePattern, this->FilePrefix, fileNumber);
      
      // Close file from any previous image
      if (this->File)
	{
	this->File->close();
	delete this->File;
	this->File = NULL;
	}
      
      // Open the new file.
      vtkDebugMacro(<< "SetFileName: opening Short file " << this->FileName);
      this->File = new ifstream(this->FileName, ios::in);
      if (! this->File || this->File->fail())
	{
	vtkErrorMacro(<< "Could not open file " << this->FileName);
	return;
	}      
      
      // Get the data from this file.
      this->UpdateFromFile(region);
      }
    }
  
  // Restore the extent of the original region.
  region->SetExtent(saveExtent);
}



  
  
  
  
  
  







