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
  this->FileDimensionality = 2;
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
  os << indent << "FileDimensionality: " << this->FileDimensionality << "\n";
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

  // Set FileExtent here since it is constant.
  for (idx = 0; idx < 2 * this->FileDimensionality; ++idx)
    {
    this->FileExtent[idx] = this->DataExtent[idx];
    }  
  
  // some increments are invalid because files are 2D images.
  for (idx = this->FileDimensionality+1; idx < VTK_IMAGE_DIMENSIONS; ++idx)
    {
    this->FileIncrements[idx] = this->FileIncrements[this->FileDimensionality];
    }
  
  // Recompute the header size.
  if ( ! this->ManualHeaderSize)
    {
    this->HeaderSize = this->FileSize - this->FileIncrements[4];
    vtkDebugMacro(<< "Initialize: Header " << this->HeaderSize << " bytes");
    }
  
  this->Initialized = 1;
}

//----------------------------------------------------------------------------
// Description:
// Sets the range (extent) of the third axis.
// This is for compatability with the old V16 reader.
// In most cases this will be the range/extent of the series.
void vtkImageSeriesReader::SetImageRange(int start, int end)
{
  this->First = start;
  this->DataDimensions[2] = end - start + 1;
  this->DataExtent[4] = 0;
  this->DataExtent[5] = end - start;
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
// It loops over the last dimension that has data
// It splits up the request into "images", and then reads the images 
// from the files.  It assumes that the series contains at most
// 2 dimensions.
void vtkImageSeriesReader::UpdatePointData(vtkImageRegion *region)
{
  int idx0, idx1, temp;
  int dataIdx0, dataIdx1;
  int outMin0, outMax0,  outMin1, outMax1;
  int *axis;
  int saveExtent[VTK_IMAGE_EXTENT_DIMENSIONS];
  int fileInc1;  // how to increment the file numbers
  int fileNumber;
  
  if ( ! this->Initialized)
    {
    this->Initialize();
    }
  
  // Save the extent of the original region.
  region->GetExtent(saveExtent);
  
  // Increments used to compute file number. (If more than one axis in series)
  temp = this->FileDimensionality * 2;
  fileInc1 = this->DataExtent[temp+1] - this->DataExtent[temp] + 1;

  // Get the extent of the extra axes. (Needed to loop over images.)
  axis = region->GetAxes();
  region->GetAxisExtent(axis[this->FileDimensionality], outMin0, outMax0);
  region->GetAxisExtent(axis[this->FileDimensionality+1], outMin1, outMax1);
  
  // loop over images
  for (idx1 = outMin1; idx1 <= outMax1; ++idx1)
    {
    // Convert extent from out coordinates to data coordinates.
    if ( this->Flips[this->FileDimensionality+1])
      {
      temp = (this->FileDimensionality+1) * 2;
      dataIdx1 = -idx1 + this->DataExtent[temp] + this->DataExtent[temp+1];
      }
    else
      {
      dataIdx1 = idx1;
      }
    temp = this->FileDimensionality+1;
    region->SetAxisExtent(axis[temp], idx1, idx1);
    temp = temp * 2;
    this->FileExtent[temp] = this->FileExtent[temp+1] = idx1;
    for (idx0 = outMin0; idx0 <= outMax0; ++idx0)
      {
      // Convert extent from out coordinates to data coordinates.
      if ( this->Flips[this->FileDimensionality])
	{
	dataIdx0 = -idx0 + this->DataExtent[this->FileDimensionality*2] 
	  + this->DataExtent[this->FileDimensionality*2+1];
	}  
      else
	{
	dataIdx0 = idx0;
	}
      region->SetAxisExtent(axis[this->FileDimensionality], idx0, idx0);
      this->FileExtent[this->FileDimensionality*2] 
	= this->FileExtent[this->FileDimensionality*2+1] = idx0;
    
      // compute the file number (I do not like this. Too messy)
      fileNumber = this->First 
	+ (dataIdx0 - this->DataExtent[this->FileDimensionality*2])
	+ fileInc1*(dataIdx1-this->DataExtent[(this->FileDimensionality+1)*2]);
      
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
      vtkDebugMacro(<< "UpdatePointData: opening Short file " 
         << this->FileName);
#ifdef _WIN32
      this->File = new ifstream(this->FileName, ios::in | ios::binary);
#else
      this->File = new ifstream(this->FileName, ios::in);
#endif
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



  
  
  
  
  
  







