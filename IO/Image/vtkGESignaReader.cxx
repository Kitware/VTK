/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkGESignaReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkGESignaReader.h"

#include "vtkByteSwap.h"
#include "vtkDataArray.h"
#include "vtkImageData.h"
#include "vtkObjectFactory.h"
#include "vtkPointData.h"
#include "vtkMedicalImageProperties.h"

#include <cassert>

vtkStandardNewMacro(vtkGESignaReader);


int vtkGESignaReader::CanReadFile(const char* fname)
{
  FILE *fp = fopen(fname, "rb");
  if (!fp)
  {
    return 0;
  }

  int magic;
  if (fread(&magic, 4, 1, fp) != 1)
  {
    fclose (fp);
    return 0;
  }
  vtkByteSwap::Swap4BE(&magic);

  if (magic != 0x494d4746) // "IMGF"
  {
    fclose(fp);
    return 0;
  }
  fclose(fp);
  return 3;
}


void vtkGESignaReader::ExecuteInformation()
{
  this->ComputeInternalFileName(this->DataExtent[4]);
  if (this->InternalFileName == NULL)
  {
    return;
  }

  FILE *fp = fopen(this->InternalFileName, "rb");
  if (!fp)
  {
    vtkErrorMacro("Unable to open file " << this->InternalFileName);
    return;
  }

  int magic;
  if (fread(&magic, 4, 1, fp) != 1)
  {
    vtkErrorMacro ("GESignaReader error reading file: " << this->FileName
                   << " Premature EOF while reading magic.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap4BE(&magic);

  if (magic != 0x494d4746)
  {
    vtkErrorMacro(<<"Unknown file type! Not a GE ximg file!");
    fclose(fp);
    return;
  }

  // read in the pixel offset from the header
  int offset;
  if (fread(&offset, 4, 1, fp) != 1)
  {
    vtkErrorMacro ("GESignaReader error reading file: " << this->FileName
                   << " Premature EOF while reading pixel offset.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap4BE(&offset);
  this->SetHeaderSize(offset);

  int width, height, depth;
  if (fread(&width, 4, 1, fp) != 1)
  {
    vtkErrorMacro ("GESignaReader error reading file: " << this->FileName
                   << " Premature EOF while reading width.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap4BE(&width);
  if (fread(&height, 4, 1, fp) != 1)
  {
    vtkErrorMacro ("GESignaReader error reading file: " << this->FileName
                   << " Premature EOF while reading height.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap4BE(&height);
  // depth in bits
  if (fread(&depth, 4, 1, fp) != 1)
  {
    vtkErrorMacro ("GESignaReader error reading file: " << this->FileName
                   << " Premature EOF while reading depth.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap4BE(&depth);

  int compression;
  if (fread(&compression, 4, 1, fp) != 1)
  {
    vtkErrorMacro ("GESignaReader error reading file: " << this->FileName
                   << " Premature EOF while reading compression.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap4BE(&compression);

  // seek to the exam series and image header offsets
  fseek(fp, 132, SEEK_SET);
  int examHdrOffset;
  if (fread(&examHdrOffset, 4, 1, fp) != 1)
  {
    vtkErrorMacro ("GESignaReader error reading file: " << this->FileName
                   << " Premature EOF while reading exam header offset.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap4BE(&examHdrOffset);
  fseek(fp, 140, SEEK_SET);
  int seriesHdrOffset;
  if (fread(&seriesHdrOffset, 4, 1, fp) != 1)
  {
    vtkErrorMacro ("GESignaReader error reading file: " << this->FileName
                   << " Premature EOF while series header offset.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap4BE(&seriesHdrOffset);
  fseek(fp, 148, SEEK_SET);
  int imgHdrOffset;
  if (fread(&imgHdrOffset, 4, 1, fp) != 1)
  {
    vtkErrorMacro ("GESignaReader error reading file: " << this->FileName
                   << " Premature EOF while image header offset.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap4BE(&imgHdrOffset);

  // seek to the exam and read some info
  char tmpStr[1024];
  // suite ID
  fseek(fp, examHdrOffset + 0, SEEK_SET);
  if (fread(tmpStr,4,1,fp) != 1)
  {
    vtkErrorMacro ("GESignaReader error reading file: " << this->FileName
                   << " Premature EOF while suite ID.");
    fclose (fp);
    return;
  }
  tmpStr[4] = 0;
  this->GetMedicalImageProperties()->SetStudyDescription( tmpStr ); // StudyID would be more suited...
  // exam number
  fseek(fp, examHdrOffset + 8, SEEK_SET);
  unsigned short examnumber;
  if (fread(&examnumber,2,1,fp) != 1)
  {
    vtkErrorMacro ("GESignaReader error reading file: " << this->FileName
                   << " Premature EOF while exam number.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap2BE(&examnumber);
  snprintf(tmpStr,sizeof(tmpStr),"%d",examnumber);
  //this->SetStudyNumber(tmpStr);
  // Patient ID
  fseek(fp, examHdrOffset + 84, SEEK_SET);
  if (fread(tmpStr,13,1,fp) != 1)
  {
    vtkErrorMacro ("GESignaReader error reading file: " << this->FileName
                   << " Premature EOF while patient ID.");
    fclose (fp);
    return;
  }
  tmpStr[13] = 0;
  this->SetPatientID(tmpStr);
  // Patient Name
  if (fread(tmpStr,25,1,fp) != 1)
  {
    vtkErrorMacro ("GESignaReader error reading file: " << this->FileName
                   << " Premature EOF while reading patient name.");
    fclose (fp);
    return;
  }
  tmpStr[25] = 0;
  this->SetPatientName(tmpStr);
  // Patient Age
  fseek(fp, examHdrOffset + 122, SEEK_SET);
  short patientage;
  if (fread(&patientage,2,1,fp) != 1)
  {
    vtkErrorMacro ("GESignaReader error reading file: " << this->FileName
                   << " Premature EOF while reading patient age.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap2BE(&patientage);
  snprintf(tmpStr,sizeof(tmpStr),"%d",patientage);
  this->GetMedicalImageProperties()->SetPatientAge( tmpStr );
  // Patient Sex
  fseek(fp, examHdrOffset + 126, SEEK_SET);
  short patientsex;
  if (fread(&patientsex,2,1,fp) != 1)
  {
    vtkErrorMacro ("GESignaReader error reading file: " << this->FileName
                   << " Premature EOF while reading patient sex.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap2BE(&patientsex);
  snprintf(tmpStr,sizeof(tmpStr),"%d",patientsex);
  this->GetMedicalImageProperties()->SetPatientSex( tmpStr );
  // Modality
  fseek(fp, examHdrOffset + 305, SEEK_SET);
  if (fread(tmpStr,3,1,fp) != 1)
  {
    vtkErrorMacro ("GESignaReader error reading file: " << this->FileName
                   << " Premature EOF while reading modality.");
    fclose (fp);
    return;
  }
  tmpStr[3] = 0;
  this->SetModality(tmpStr);

  // seek to the series and read some info
  // series number
  fseek(fp, seriesHdrOffset + 10, SEEK_SET);
  short series;
  if (fread(&series,2,1,fp) != 1)
  {
    vtkErrorMacro ("GESignaReader error reading file: " << this->FileName
                   << " Premature EOF while reading series.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap2BE(&series);
  snprintf(tmpStr,sizeof(tmpStr),"%d",series);
  this->SetSeries(tmpStr);
  // scan protocol name
  fseek(fp, seriesHdrOffset + 92, SEEK_SET);
  if (fread(tmpStr,25,1,fp) != 1)
  {
    vtkErrorMacro ("GESignaReader error reading file: " << this->FileName
                   << " Premature EOF while reading scan protocol.");
    fclose (fp);
    return;
  }
  tmpStr[25] = 0;
  this->SetStudy(tmpStr); // ??

  // now seek to the image header and read some values
  float tmpX, tmpY, tmpZ;
  float spacingX, spacingY, spacingZ;
  fseek(fp, imgHdrOffset + 50, SEEK_SET);
  if (fread(&spacingX, 4, 1, fp) != 1)
  {
    vtkErrorMacro ("GESignaReader error reading file: " << this->FileName
                   << " Premature EOF while reading spacing x.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap4BE(&spacingX);
  if (fread(&spacingY, 4, 1, fp) != 1)
  {
    vtkErrorMacro ("GESignaReader error reading file: " << this->FileName
                   << " Premature EOF while reading spacing y.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap4BE(&spacingY);
  fseek(fp, imgHdrOffset + 116, SEEK_SET);
  if (fread(&spacingZ, 4, 1, fp) != 1)
  {
    vtkErrorMacro ("GESignaReader error reading file: " << this->FileName
                   << " Premature EOF while reading spacing z.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap4BE(&spacingZ);
  // Slice Thickness
  fseek(fp, imgHdrOffset + 26, SEEK_SET);
  if (fread(&tmpZ, 4, 1, fp) != 1)
  {
    vtkErrorMacro ("GESignaReader error reading file: " << this->FileName
                   << " Premature EOF while reading slice thickness.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap4BE(&tmpZ);
  spacingZ = spacingZ + tmpZ;

  float origX, origY, origZ;
  fseek(fp, imgHdrOffset + 154, SEEK_SET);
  // read TLHC
  if (fread(&origX, 4, 1, fp) != 1)
  {
    vtkErrorMacro ("GESignaReader error reading file: " << this->FileName
                   << " Premature EOF while reading origX.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap4BE(&origX);
  if (fread(&origY, 4, 1, fp) != 1)
  {
    vtkErrorMacro ("GESignaReader error reading file: " << this->FileName
                   << " Premature EOF while reading origY.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap4BE(&origY);
  if (fread(&origZ, 4, 1, fp) != 1)
  {
    vtkErrorMacro ("GESignaReader error reading file: " << this->FileName
                   << " Premature EOF while reading origZ.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap4BE(&origZ);

  // read TRHC
  if (fread(&tmpX, 4, 1, fp) != 1)
  {
    vtkErrorMacro ("GESignaReader error reading file: " << this->FileName
                   << " Premature EOF while reading TRHC x.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap4BE(&tmpX);
  if (fread(&tmpY, 4, 1, fp) != 1)
  {
    vtkErrorMacro ("GESignaReader error reading file: " << this->FileName
                   << " Premature EOF while reading TRCH y.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap4BE(&tmpY);
  if (fread(&tmpZ, 4, 1, fp) != 1)
  {
    vtkErrorMacro ("GESignaReader error reading file: " << this->FileName
                   << " Premature EOF while reading TRCH z.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap4BE(&tmpZ);

  // compute BLHC = TLHC - TRHC + BRHC
  origX = origX - tmpX;
  origY = origY - tmpY;
  origZ = origZ - tmpZ;

  // read BRHC
  if (fread(&tmpX, 4, 1, fp) != 1)
  {
    vtkErrorMacro ("GESignaReader error reading file: " << this->FileName
                   << " Premature EOF while reading BRHC x.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap4BE(&tmpX);
  if (fread(&tmpY, 4, 1, fp) != 1)
  {
    vtkErrorMacro ("GESignaReader error reading file: " << this->FileName
                   << " Premature EOF while reading BRHC y.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap4BE(&tmpY);
  if (fread(&tmpZ, 4, 1, fp) != 1)
  {
    vtkErrorMacro ("GESignaReader error reading file: " << this->FileName
                   << " Premature EOF while reading BRCH z.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap4BE(&tmpZ);

  // compute BLHC = TLHC - TRHC + BRHC
  origX = origX + tmpX;
  origY = origY + tmpY;
  origZ = origZ + tmpZ;

  /*
  http://www.dclunie.com/medical-image-faq/html/part4.html#Signa5X
  image header - for MR (1022 bytes long):

  194 - int      - repetition time(usec)
  198 - int      - inversion time(usec)
  202 - int      - echo time(usec)
  210 - short    - number of echoes
  212 - short    - echo number
  218 - float    - NEX
  308 - char[33] - pulse sequence name
  362 - char[17] - coil name
  640 - short    - ETL for FSE
   */

  this->SetDataOrigin(origX, origY, origZ);

  this->DataExtent[0] = 0;
  this->DataExtent[1] = width - 1;
  this->DataExtent[2] = 0;
  this->DataExtent[3] = height - 1;

  this->SetDataScalarTypeToUnsignedShort();

  this->SetNumberOfScalarComponents(1);
  this->SetDataSpacing(spacingX, spacingY, spacingZ);
  this->vtkImageReader2::ExecuteInformation();

  // close the file
  fclose(fp);
}

static void vtkcopygenesisimage(FILE *infp, int width, int height, int compress,
                                short *map_left, short *map_wide,
                                unsigned short *output)
{
  unsigned short row;
  unsigned short last_pixel=0;
  for (row=0; row<height; ++row)
  {
      unsigned short j;
      unsigned short start;
      unsigned short end;

      if (compress == 2 || compress == 4)
      { // packed/compacked
          assert(map_left);
          assert(map_wide);
          start=map_left[row];
          end=start+map_wide[row];
      }
      else
      {
          start=0;
          end=width;
      }
      // Pad the first "empty" part of the line ...
      for (j=0; j<start; j++)
      {
          (*output) = 0;
          ++output;
      }

      if (compress == 3 || compress == 4)
      { // compressed/compacked
          while (start<end)
          {
              unsigned char byte;
              if (!fread(&byte,1,1,infp))
              {
                  return;
              }
              if (byte & 0x80)
              {
                  unsigned char byte2;
                  if (!fread(&byte2,1,1,infp))
                  {
                      return;
                  }
                  if (byte & 0x40)
                  {      // next word
                      if (!fread(&byte,1,1,infp))
                      {
                          return;
                      }
                      last_pixel=
                        (((unsigned short)byte2<<8)+byte);
                  }
                  else
                  {                  // 14 bit delta
                      if (byte & 0x20)
                      {
                          byte|=0xe0;
                      }
                      else
                      {
                          byte&=0x1f;
                      }
                      last_pixel+=
                        (((short)byte<<8)+byte2);
                  }
              }
              else
              {                          // 7 bit delta
                  if (byte & 0x40)
                  {
                      byte|=0xc0;
                  }
                  last_pixel+=(signed char)byte;
              }
              (*output) = last_pixel;
              ++output;
              ++start;
          }
      }
      else
      {
          while (start<end)
          {
              unsigned short u;
              if (!fread(&u,2,1,infp))
              {
                  return;
              }
              vtkByteSwap::Swap2BE(&u);
              (*output) = u;
              ++output;
              ++start;
          }
      }

      // Pad the last "empty" part of the line ...
      for (j=end; j<width; j++)
      {
          (*output) = 0;
          ++output;
      }
  }
}


static void vtkGESignaReaderUpdate2(vtkGESignaReader *self, unsigned short *outPtr,
                                    int *outExt, vtkIdType *)
{
  FILE *fp = fopen(self->GetInternalFileName(), "rb");
  if (!fp)
  {
    return;
  }

  int magic;
  if (fread(&magic, 4, 1, fp) != 1)
  {
    vtkGenericWarningMacro ("GESignaReader error reading file: " << self->GetInternalFileName()
                            << " Premature EOF while reading magic.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap4BE(&magic);

  if (magic != 0x494d4746)
  {
    vtkGenericWarningMacro(<<"Unknown file type! Not a GE ximg file!");
    fclose(fp);
    return;
  }

  // read in the pixel offset from the header
  int offset;
  if (fread(&offset, 4, 1, fp) != 1)
  {
    vtkGenericWarningMacro ("GESignaReader error reading file: " << self->GetInternalFileName()
                            << " Premature EOF while reading origY.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap4BE(&offset);

  int width, height, depth;
  if (fread(&width, 4, 1, fp) != 1)
  {
    vtkGenericWarningMacro ("GESignaReader error reading file: " << self->GetInternalFileName()
                            << " Premature EOF while reading width.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap4BE(&width);
  if (fread(&height, 4, 1, fp) != 1)
  {
    vtkGenericWarningMacro ("GESignaReader error reading file: " << self->GetInternalFileName()
                            << " Premature EOF while reading height.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap4BE(&height);
  // depth in bits
  if (fread(&depth, 4, 1, fp) != 1)
  {
    vtkGenericWarningMacro ("GESignaReader error reading file: " << self->GetInternalFileName()
                            << " Premature EOF while reading depth.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap4BE(&depth);

  int compression;
  if (fread(&compression, 4, 1, fp) != 1)
  {
    vtkGenericWarningMacro ("GESignaReader error reading file: " << self->GetInternalFileName()
                            << " Premature EOF while reading compression.");
    fclose (fp);
    return;
  }
  vtkByteSwap::Swap4BE(&compression);

  short *leftMap = 0;
  short *widthMap = 0;

  if (compression == 2 || compression == 4)
  { // packed/compacked
      leftMap = new short [height];
      widthMap = new short [height];

      fseek(fp, 64, SEEK_SET);
      int packHdrOffset;
      if (fread(&packHdrOffset, 4, 1, fp) != 1)
      {
        vtkGenericWarningMacro ("GESignaReader error reading file: " << self->GetInternalFileName()
                                << " Premature EOF while reading packHdrOffset.");
        fclose (fp);
        delete [] leftMap;
        delete [] widthMap;
        return;
      }
      vtkByteSwap::Swap4BE(&packHdrOffset);

      // now seek to the pack header and read some values
      fseek(fp, packHdrOffset, SEEK_SET);
      // read in the maps
      int i;
      for (i = 0; i < height; i++)
      {
        if (fread(leftMap+i, 2, 1, fp) != 1)
        {
          vtkGenericWarningMacro ("GESignaReader error reading file: " << self->GetInternalFileName()
                                  << " Premature EOF while reading maps.");
          fclose (fp);
          return;
        }
        vtkByteSwap::Swap2BE(leftMap+i);
        if (fread(widthMap+i, 2, 1, fp) != 1)
        {
          vtkGenericWarningMacro ("GESignaReader error reading file: " << self->GetInternalFileName()
                                  << " Premature EOF while reading maps.");
          fclose (fp);
          return;
        }
        vtkByteSwap::Swap2BE(widthMap+i);
      }
  }

  // seek to pixel data
  fseek(fp, offset, SEEK_SET);

  // read in the pixels
  unsigned short *tmp = new unsigned short [width*height];
  int *dext = self->GetDataExtent();
  vtkcopygenesisimage(fp, dext[1] + 1, dext[3] + 1,
                      compression, leftMap, widthMap, tmp);

  // now copy into desired extent
  int yp;
  for (yp = outExt[2]; yp <= outExt[3]; ++yp)
  {
      int ymod = height - yp - 1;
      memcpy(outPtr,tmp+ymod*width+outExt[0],2*width);
      outPtr = outPtr + width;
  }

  delete [] tmp;
  delete [] leftMap;
  delete [] widthMap;

  fclose(fp);
}

//----------------------------------------------------------------------------
// This function reads in one data of data.
// templated to handle different data types.
static void vtkGESignaReaderUpdate(vtkGESignaReader *self, vtkImageData *data,
                                   unsigned short *outPtr)
{
  vtkIdType outIncr[3];
  int outExtent[6];
  unsigned short *outPtr2;

  data->GetExtent(outExtent);
  data->GetIncrements(outIncr);

  outPtr2 = outPtr;
  int idx2;
  for (idx2 = outExtent[4]; idx2 <= outExtent[5]; ++idx2)
  {
    self->ComputeInternalFileName(idx2);
    // read in a PNG file
    vtkGESignaReaderUpdate2(self, outPtr2, outExtent, outIncr);
    self->UpdateProgress((idx2 - outExtent[4])/
                         (outExtent[5] - outExtent[4] + 1.0));
    outPtr2 += outIncr[2];
  }
}


//----------------------------------------------------------------------------
// This function reads a data from a file.  The datas extent/axes
// are assumed to be the same as the file extent/order.
void vtkGESignaReader::ExecuteDataWithInformation(vtkDataObject *output,
                                                  vtkInformation *outInfo)
{
  vtkImageData *data = this->AllocateOutputData(output, outInfo);

  if (this->InternalFileName == NULL)
  {
    vtkErrorMacro(<< "Either a FileName or FilePrefix must be specified.");
    return;
  }

  data->GetPointData()->GetScalars()->SetName("GESignalImage");

  this->ComputeDataIncrements();

  // Call the correct templated function for the output
  void *outPtr;

  // Call the correct templated function for the input
  outPtr = data->GetScalarPointer();
  vtkGESignaReaderUpdate(this, data, (unsigned short *)(outPtr));
}

//----------------------------------------------------------------------------
void vtkGESignaReader::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os,indent);
}
