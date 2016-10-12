/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkBMPReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkBMPReader.h"

#include "vtkByteSwap.h"
#include "vtkImageData.h"
#include "vtkLookupTable.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"

vtkStandardNewMacro(vtkBMPReader);

#ifdef read
#undef read
#endif

#ifdef close
#undef close
#endif

vtkBMPReader::vtkBMPReader()
{
  this->Colors = NULL;
  this->SetDataByteOrderToLittleEndian();
  this->Depth = 0;
  // we need to create it now in case its asked for later (pointer must be valid)
  this->LookupTable = vtkLookupTable::New();
  this->Allow8BitBMP = 0;
}

//----------------------------------------------------------------------------
vtkBMPReader::~vtkBMPReader()
{
  // free any old memory
  delete [] this->Colors;
  this->Colors = NULL;

  if (this->LookupTable)
  {
    this->LookupTable->Delete();
    this->LookupTable = NULL;
  }
}

//----------------------------------------------------------------------------
void vtkBMPReader::ExecuteInformation()
{
  int xsize, ysize;
  FILE *fp;
  vtkTypeInt32 tmp;
  vtkTypeInt32 offset;
  vtkTypeInt32 infoSize;
  vtkTypeInt16 stmp1, stmp2;

  // free any old memory
  delete [] this->Colors;
  this->Colors = NULL;

  // if the user has not set the extent, but has set the VOI
  // set the zaxis extent to the VOI z axis
  if (this->DataExtent[4]==0 && this->DataExtent[5] == 0 &&
      (this->DataVOI[4] || this->DataVOI[5]))
  {
    this->DataExtent[4] = this->DataVOI[4];
    this->DataExtent[5] = this->DataVOI[5];
  }

  this->ComputeInternalFileName(this->DataExtent[4]);
  if (this->InternalFileName == NULL || this->InternalFileName[0] == '\0')
  {
    return;
  }
  // get the magic number by reading in a file
  fp = fopen(this->InternalFileName,"rb");
  if (!fp)
  {
    vtkErrorMacro("Unable to open file " << this->InternalFileName);
    return;
  }

  // compare magic number to determine file type
  if ((fgetc(fp) != 'B')||(fgetc(fp) != 'M'))
  {
    vtkErrorMacro(<<"Unknown file type! " << this->InternalFileName
                  <<" is not a Windows BMP file!");
    fclose(fp);
    return;
  }

  // error indicator
  bool errorOccurred = false;

  // get the size of the file

  // skip 4 bytes
  if (fread(&tmp,4,1,fp) != 1)
  {
    errorOccurred = true;
  }
  // skip 4 more bytes
  else if (fread(&tmp,4,1,fp) != 1)
  {
    errorOccurred = true;
  }
  // read the offset
  else if (fread(&offset,4,1,fp) != 1)
  {
    errorOccurred = true;
  }
  // get size of header
  else if (fread(&infoSize,4,1,fp) != 1)
  {
    errorOccurred = true;
  }

  if (errorOccurred)
  {
    vtkErrorMacro("Error reading file: " << this->InternalFileName
                  <<" Premature end of file.");
    fclose(fp);
    return;
  }

  vtkByteSwap::Swap4LE(&infoSize);

  // error checking
  if ((infoSize != 40)&&(infoSize != 12))
  {
    vtkErrorMacro("Unknown file type! " << this->InternalFileName
                  <<" is not a Windows BMP file!");
    fclose(fp);
    return;
  }

  // there are two different types of BMP files
  if (infoSize == 40)
  {
    // now get the dimensions
    if (fread(&xsize,4,1,fp) != 1)
    {
      errorOccurred = true;
    }
    else if (fread(&ysize,4,1,fp) != 1)
    {
      errorOccurred = true;
    }
    vtkByteSwap::Swap4LE(&xsize);
    vtkByteSwap::Swap4LE(&ysize);
  }
  else
  {
    if (fread(&stmp1,2,1,fp) != 1)
    {
      errorOccurred = true;
    }
    else if (fread(&stmp2,2,1,fp) != 1)
    {
      errorOccurred = true;
    }
    vtkByteSwap::Swap2LE(&stmp1);
    vtkByteSwap::Swap2LE(&stmp2);
    xsize = stmp1;
    ysize = stmp2;
  }

  if (errorOccurred)
  {
    vtkErrorMacro ("BMPReader error reading file: " << this->InternalFileName
                   << " Premature EOF while reading size.");
    fclose (fp);
    return;
  }

  // is corner in upper left or lower left
  if (ysize < 0)
  {
    ysize = ysize*-1;
    this->FileLowerLeft = 0;
  }
  else
  {
    this->FileLowerLeft = 1;
  }

  // ignore planes
  if (fread(&stmp1,2,1,fp) != 1)
  {
    errorOccurred = true;
  }
  // read depth
  else if (fread(&stmp2,2,1,fp) != 1)
  {
    errorOccurred = true;
  }

  if (errorOccurred)
  {
    vtkErrorMacro ("BMPReader error reading file: " << this->InternalFileName
                   << " Premature EOF while reading depth.");
    fclose (fp);
    return;
  }

  vtkByteSwap::Swap2LE(&stmp2);
  this->Depth = stmp2;

  if ((this->Depth != 8)&&(this->Depth != 24))
  {
    vtkErrorMacro("Only BMP depths of (8,24) are supported. Not "
                  << this->Depth);
    fclose(fp);
    return;
  }

  // skip over rest of info for long format
  if (infoSize == 40)
  {
    int skip[6];
    if (fread(&skip,4,6,fp) != 6)
    {
      vtkErrorMacro ("BMPReader error reading file: " << this->InternalFileName
                     << " Premature EOF skipping rest of info for long format.");
      fclose (fp);
      return;
    }
  }

  // read in color table if required
  if (this->Depth < 24)
  {
    int numColors = 256;
    this->Colors = new unsigned char [numColors*3];
    for (tmp = 0; tmp < numColors; tmp++)
    {
      this->Colors[tmp*3+2] = fgetc(fp);
      this->Colors[tmp*3+1] = fgetc(fp);
      this->Colors[tmp*3] = fgetc(fp);
      if (infoSize == 40)
      {
        fgetc(fp);
      }
    }
    if (this->Allow8BitBMP)
    {
      if (!this->LookupTable)
      {
        this->LookupTable = vtkLookupTable::New();
      }
      this->LookupTable->SetNumberOfTableValues(numColors);
      for (tmp=0; tmp<numColors; tmp++)
      {
        this->LookupTable->SetTableValue(tmp,
              (float)this->Colors[tmp*3+0]/255.0,
              (float)this->Colors[tmp*3+1]/255.0,
              (float)this->Colors[tmp*3+2]/255.0,
              1);
      }
      this->LookupTable->SetRange(0,255);
    }
  }

  if (fclose(fp))
  {
    vtkWarningMacro("File close failed on " << this->InternalFileName);
  }

  // Offset is the true header size. See bug 14397
  vtkByteSwap::Swap4LE(&offset);
  this->ManualHeaderSize = 1;
  this->HeaderSize = offset;

  // if the user has set the VOI, just make sure its valid
  if (this->DataVOI[0] || this->DataVOI[1] ||
      this->DataVOI[2] || this->DataVOI[3] ||
      this->DataVOI[4] || this->DataVOI[5])
  {
    if ((this->DataVOI[0] < 0) ||
        (this->DataVOI[1] >= xsize) ||
        (this->DataVOI[2] < 0) ||
        (this->DataVOI[3] >= ysize))
    {
      vtkWarningMacro("The requested VOI is larger than the file's (" << this->InternalFileName << ") extent ");
      this->DataVOI[0] = 0;
      this->DataVOI[1] = xsize - 1;
      this->DataVOI[2] = 0;
      this->DataVOI[3] = ysize - 1;
    }
  }

  this->DataExtent[0] = 0;
  this->DataExtent[1] = xsize - 1;
  this->DataExtent[2] = 0;
  this->DataExtent[3] = ysize - 1;

  this->SetDataScalarTypeToUnsignedChar();
  if ((this->Depth == 8) && this->Allow8BitBMP)
  {
    this->SetNumberOfScalarComponents(1);
  }
  else
  {
    this->SetNumberOfScalarComponents(3);
  }

  this->vtkImageReader::ExecuteInformation();
}

//----------------------------------------------------------------------------
// This function opens a file to determine the file size, and to
// automatically determine the header size.
void vtkBMPReader::ComputeDataIncrements()
{
  int idx;
  vtkIdType fileDataLength;

  // Determine the expected length of the data ...
  switch (this->DataScalarType)
  {
    case VTK_FLOAT:
      fileDataLength = sizeof(float);
      break;
    case VTK_INT:
      fileDataLength = sizeof(int);
      break;
    case VTK_SHORT:
      fileDataLength = sizeof(short);
      break;
    case VTK_UNSIGNED_SHORT:
      fileDataLength = sizeof(unsigned short);
      break;
    case VTK_UNSIGNED_CHAR:
      fileDataLength = sizeof(unsigned char);
      break;
    default:
      vtkErrorMacro(<< "Unknown DataScalarType");
      return;
  }

  fileDataLength *= (this->Depth/8);

  // a row must end on a 4 byte boundary
  // so update the Increments[1]
  this->DataIncrements[0] = fileDataLength;
  fileDataLength = fileDataLength *
    (this->DataExtent[1] - this->DataExtent[0] + 1);
  // move to 4 byte boundary
  fileDataLength = fileDataLength + (4 - fileDataLength%4)%4;

  // compute the fileDataLength (in units of bytes)
  for (idx = 1; idx < 3; ++idx)
  {
    this->DataIncrements[idx] = fileDataLength;
    fileDataLength = fileDataLength *
      (this->DataExtent[idx*2+1] - this->DataExtent[idx*2] + 1);
  }
}

//----------------------------------------------------------------------------
// This function reads in one data of data.
// templated to handle different data types.
template <class OT>
void vtkBMPReaderUpdate2(vtkBMPReader *self, vtkImageData *data, OT *outPtr)
{
  vtkIdType inIncr[3], outIncr[3];
  OT *outPtr0, *outPtr1, *outPtr2;
  vtkIdType streamSkip0, streamSkip1;
  vtkIdType streamRead;
  int idx0, idx1, idx2, pixelRead;
  int inExtent[6];
  int dataExtent[6];
  int pixelSkip;
  unsigned char *inPtr;
  unsigned char *Colors;
  unsigned long count = 0;
  unsigned long target;
  int Keep8bit = 0;

  // Get the requested extents.
  data->GetExtent(inExtent);
  // Convert them into to the extent needed from the file.
  self->ComputeInverseTransformedExtent(inExtent,dataExtent);

  // get and transform the increments
  data->GetIncrements(inIncr);
  self->ComputeInverseTransformedIncrements(inIncr,outIncr);

  // get the color lut
  Colors = self->GetColors();

  // are we converting to RGB or staying as 8bit
  if ((self->GetDepth() == 8) && self->GetAllow8BitBMP())
  {
    Keep8bit = 1;
  }

  // compute outPtr2
  outPtr2 = outPtr;
  if (outIncr[0] < 0)
  {
    outPtr2 = outPtr2 - outIncr[0]*(dataExtent[1] - dataExtent[0]);
  }
  if (outIncr[1] < 0)
  {
    outPtr2 = outPtr2 - outIncr[1]*(dataExtent[3] - dataExtent[2]);
  }
  if (outIncr[2] < 0)
  {
    outPtr2 = outPtr2 - outIncr[2]*(dataExtent[5] - dataExtent[4]);
  }

  // length of a row, num pixels read at a time
  pixelRead = dataExtent[1] - dataExtent[0] + 1;
  streamRead = (vtkIdType) (pixelRead * self->GetDataIncrements()[0]);
  streamSkip0 = (vtkIdType) (self->GetDataIncrements()[1] - streamRead);
  streamSkip1 = (vtkIdType) (self->GetDataIncrements()[2] -
    (dataExtent[3] - dataExtent[2] + 1)* self->GetDataIncrements()[1]);
  pixelSkip = self->GetDepth()/8;

  // read from the bottom up
  if (!self->GetFileLowerLeft())
  {
    streamSkip0 = (vtkIdType) (-streamRead - self->GetDataIncrements()[1]);
  }

  target = (unsigned long)((dataExtent[5]-dataExtent[4]+1)*
                           (dataExtent[3]-dataExtent[2]+1)/50.0);
  target++;

  if (self->GetFileDimensionality() == 3)
  {
    if (!self->OpenAndSeekFile(dataExtent,0))
    {
      return;
    }
  }

  // create a buffer to hold a row of the data
  unsigned char *buf = new unsigned char[streamRead];

  // read the data row by row
  for (idx2 = dataExtent[4]; idx2 <= dataExtent[5]; ++idx2)
  {
    if (self->GetFileDimensionality() == 2)
    {
      if (!self->OpenAndSeekFile(dataExtent,idx2))
      {
        delete [] buf;
        return;
      }
    }
    outPtr1 = outPtr2;
    for (idx1 = dataExtent[2];
         !self->AbortExecute && idx1 <= dataExtent[3]; ++idx1)
    {
      if (!(count%target))
      {
        self->UpdateProgress(count/(50.0*target));
      }
      count++;
      outPtr0 = outPtr1;

      // read the row.
      if ( ! self->GetFile()->read((char *)buf, streamRead))
      {
        vtkErrorWithObjectMacro(self,
                               "File operation failed. row = " << idx1
                               << ", Read = " << streamRead
                               << ", Skip0 = " << streamSkip0
                               << ", Skip1 = " << streamSkip1
                               << ", FilePos = " << static_cast<vtkIdType>(self->GetFile()->tellg())
                               << ", FileName = " << self->GetInternalFileName()
                               );
        self->GetFile()->close();
        delete [] buf;
        return;
      }


      // copy the bytes into the typed data
      inPtr = buf;
      for (idx0 = dataExtent[0]; idx0 <= dataExtent[1]; ++idx0)
      {
        // Copy pixel into the output.
        if (self->GetDepth() == 8 && !Keep8bit)
        {
          outPtr0[0] = (OT)(Colors[inPtr[0]*3]);
          outPtr0[1] = (OT)(Colors[inPtr[0]*3+1]);
          outPtr0[2] = (OT)(Colors[inPtr[0]*3+2]);
        }
        else if (self->GetDepth() == 8 && Keep8bit)
        {
          outPtr0[0] = (OT)(inPtr[0]);
        }
        else
        {
          outPtr0[0] = (OT)(inPtr[2]);
          outPtr0[1] = (OT)(inPtr[1]);
          outPtr0[2] = (OT)(inPtr[0]);
        }
        // move to next pixel
        inPtr += pixelSkip;
        outPtr0 += outIncr[0];
      }
      // move to the next row in the file and data
      self->GetFile()->seekg(static_cast<long>(self->GetFile()->tellg()) + streamSkip0, ios::beg);
      outPtr1 += outIncr[1];
    }
    // move to the next image in the file and data
    self->GetFile()->seekg(static_cast<long>(self->GetFile()->tellg()) + streamSkip1, ios::beg);
    outPtr2 += outIncr[2];
  }

  self->GetFile()->close();

  // delete the temporary buffer
  delete [] buf;
}


//----------------------------------------------------------------------------
// This function reads a data from a file.  The datas extent/axes
// are assumed to be the same as the file extent/order.
void vtkBMPReader::ExecuteDataWithInformation(vtkDataObject *output,
                                              vtkInformation* outInfo)
{
  vtkImageData *data = this->AllocateOutputData(output, outInfo);

  if (this->UpdateExtentIsEmpty(outInfo, output))
  {
    return;
  }
  if (this->InternalFileName == NULL)
  {
    vtkErrorMacro(<< "Either a FileName or FilePrefix must be specified.");
    return;
  }

  data->GetPointData()->GetScalars()->SetName("BMPImage");

  this->ComputeDataIncrements();

  // Call the correct templated function for the output
  void *outPtr;

  // Call the correct templated function for the input
  outPtr = data->GetScalarPointer();
  switch (data->GetScalarType())
  {
    vtkTemplateMacro(
      vtkBMPReaderUpdate2(this, data, static_cast<VTK_TT*>(outPtr))
      );
    default:
      vtkErrorMacro(<< "Execute: Unknown data type");
  }
}

//----------------------------------------------------------------------------
void vtkBMPReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);

  // this->Colors is not printed
  os << indent << "Depth: " << this->Depth << "\n";
  os << indent << "Allow8BitBMP: " << this->Allow8BitBMP << "\n";
  if (this->LookupTable)
  {
    os << indent << "LookupTable: " << this->LookupTable << "\n";
  }
    else
    {
    os << indent << "LookupTable: NULL\n";
    }
}

//----------------------------------------------------------------------------
int vtkBMPReader::CanReadFile(const char* fname)
{
  // get the magic number by reading in a file
  FILE* fp = fopen(fname,"rb");
  if (!fp)
  {
    return 0;
  }

  // compare magic number to determine file type
  if ((fgetc(fp) != 'B')||(fgetc(fp) != 'M'))
  {
    fclose(fp);
    return 0;
  }

  vtkTypeInt32 tmp;
  vtkTypeInt32 infoSize = 0;

  // error indicator
  bool errorOccurred = false;

  // skip 4 bytes
  if (fread(&tmp,4,1,fp) != 1)
  {
    errorOccurred = true;
  }
  // skip 4 more bytes
  else if (fread(&tmp,4,1,fp) != 1)
  {
    errorOccurred = true;
  }
  // read the offset
  else if (fread(&tmp,4,1,fp) != 1)
  {
    errorOccurred = true;
  }
  // get size of header
  else if (fread(&infoSize,4,1,fp) != 1)
  {
    infoSize = 0;
    errorOccurred = true;
  }

  vtkByteSwap::Swap4LE(&infoSize);

  // error checking
  if ((infoSize != 40)&&(infoSize != 12))
  {
    errorOccurred = true;
  }

  fclose(fp);
  return !errorOccurred;
}
