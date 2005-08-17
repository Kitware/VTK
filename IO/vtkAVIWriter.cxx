/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkAVIWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkWindows.h"
#include "vtkAVIWriter.h"

#include "vtkImageData.h"
#include "vtkObjectFactory.h"

#ifdef _MSC_VER
#pragma warning (push, 3)
#endif

#include <vfw.h>

#ifdef _MSC_VER
#pragma warning (pop)
#endif

class vtkAVIWriterInternal 
{
public:
  PAVISTREAM Stream;
  PAVISTREAM StreamCompressed;
  PAVIFILE AVIFile;
  LPBITMAPINFOHEADER  lpbi;       // pointer to BITMAPINFOHEADER
  HANDLE              hDIB;  // handle to DIB, temp handle
};

//---------------------------------------------------------------------------
vtkStandardNewMacro(vtkAVIWriter);
vtkCxxRevisionMacro(vtkAVIWriter, "1.3");

//---------------------------------------------------------------------------
vtkAVIWriter::vtkAVIWriter()
{
  this->Internals = new vtkAVIWriterInternal;
  this->Internals->Stream = NULL; 
  this->Internals->StreamCompressed = NULL;
  this->Internals->AVIFile = NULL;
  this->Time = 0;
  this->Rate = 15;
  this->Internals->hDIB = NULL;  // handle to DIB, temp handle
}

//---------------------------------------------------------------------------
vtkAVIWriter::~vtkAVIWriter()
{
  if (this->Internals->AVIFile)
    {
    this->End();
    }
  delete this->Internals;
}

//---------------------------------------------------------------------------
void vtkAVIWriter::Start()
{
  // Error checking
  this->Error = 1;
  if ( this->GetInput() == NULL )
    {
    vtkErrorMacro(<<"Write:Please specify an input!");
    return;
    }
  if (!this->FileName)
    {
    vtkErrorMacro(<<"Write:Please specify a FileName");
    return;
    }
  
  // Fill in image information.
  this->GetInput()->UpdateInformation();
  int *wExtent = this->GetInput()->GetWholeExtent();
  this->GetInput()->SetUpdateExtent(wExtent);

  LONG hr;     
  AVISTREAMINFO strhdr;
  
  AVIFileInit();          
  // opens AVIFile library  
  hr = AVIFileOpen(&this->Internals->AVIFile, this->FileName, 
                   OF_WRITE | OF_CREATE, 0L); 
  if (hr != 0)
    {         
    vtkErrorMacro("Unable to open " << this->FileName);         
    return; 
    }  

  // Fill in the header for the video stream....
  // The video stream will run in 15ths of a second....
  memset(&strhdr, 0, sizeof(strhdr));
  strhdr.fccType                = streamtypeVIDEO;// stream type
  strhdr.fccHandler             = 0;
  strhdr.dwScale                = 1;
  strhdr.dwRate                 = this->Rate;
  strhdr.dwQuality              = (DWORD) -1;
  strhdr.dwSuggestedBufferSize  = (wExtent[1] - wExtent[0] + 1)*
    (wExtent[3] - wExtent[2] + 1)*3;
  SetRect(&strhdr.rcFrame, 0, 0, wExtent[1] - wExtent[0] + 1, 
          wExtent[3] - wExtent[2] + 1);
  
  // And create the stream;
  AVIFileCreateStream(this->Internals->AVIFile,  // file pointer
                      &this->Internals->Stream,  // returned stream pointer
                      &strhdr);      // stream header

  // do not want to display this dialog
  AVICOMPRESSOPTIONS opts;
//  AVICOMPRESSOPTIONS FAR * aopts[1] = {&opts};
  memset(&opts, 0, sizeof(opts));  

  // need to setup opts
  opts.fccType = 0;
  opts.fccHandler=mmioFOURCC('m','s','v','c');
  opts.dwQuality = 10000;
  opts.dwBytesPerSecond = 0;
  opts.dwFlags = 8;

//  if (!AVISaveOptions(NULL, 0, 
//                      1, &this->Internals->Stream, 
//                      (LPAVICOMPRESSOPTIONS FAR *) &aopts))
//    {
//    vtkErrorMacro("Unable to save " << this->FileName);         
//    return;
//    }
  
  
  if (AVIMakeCompressedStream(&this->Internals->StreamCompressed, 
                              this->Internals->Stream, 
                              &opts, NULL) != AVIERR_OK)
    {
    vtkErrorMacro("Unable to compress " << this->FileName);         
    return;
    }  
  
  DWORD               dwLen;      // size of memory block
  int dataWidth = (((wExtent[1] - wExtent[0] + 1)*3+3)/4)*4;
  
  dwLen = sizeof(BITMAPINFOHEADER) + dataWidth*(wExtent[3] - wExtent[2] + 1);
  this->Internals->hDIB = ::GlobalAlloc(GHND, dwLen);
  this->Internals->lpbi = (LPBITMAPINFOHEADER) ::GlobalLock(this->Internals->hDIB);
  
  this->Internals->lpbi->biSize = sizeof(BITMAPINFOHEADER);
  this->Internals->lpbi->biWidth = wExtent[1] - wExtent[0] + 1;
  this->Internals->lpbi->biHeight = wExtent[3] - wExtent[2] + 1;
  this->Internals->lpbi->biPlanes = 1;
  this->Internals->lpbi->biBitCount = 24;
  this->Internals->lpbi->biCompression = BI_RGB;
  this->Internals->lpbi->biClrUsed = 0;
  this->Internals->lpbi->biClrImportant = 0;
  this->Internals->lpbi->biSizeImage = dataWidth*(wExtent[3] - wExtent[2] + 1);

  if (AVIStreamSetFormat(this->Internals->StreamCompressed, 0,
                         this->Internals->lpbi, this->Internals->lpbi->biSize))
    {
    vtkErrorMacro("Unable to format " << this->FileName << " Most likely this means that the video compression scheme you seleted could not handle the data. Try selecting a different compression scheme." );         
    return;
    }

  this->Error = 0;
  this->Time = 0;
}

//---------------------------------------------------------------------------
void vtkAVIWriter::Write()
{
  if (this->Error)
    {
    return;
    }
  
  // get the data
  this->GetInput()->UpdateInformation();
  int *wExtent = this->GetInput()->GetWholeExtent();
  this->GetInput()->SetUpdateExtent(wExtent);
  this->GetInput()->Update();
  // get the pointer to the data
  unsigned char *ptr = 
    (unsigned char *)(this->GetInput()->GetScalarPointer());

  int dataWidth = (((wExtent[1] - wExtent[0] + 1)*3+3)/4)*4;
  int srcWidth = (wExtent[1] - wExtent[0] + 1)*3;
  
  // copy the data to the clipboard
  unsigned char *dest 
    = (unsigned char *)this->Internals->lpbi + this->Internals->lpbi->biSize;
  int i,j;
  for (i = 0; i < this->Internals->lpbi->biHeight; i++)
    {
    for (j = 0; j < this->Internals->lpbi->biWidth; j++)
      {
      *dest++ = ptr[2];
      *dest++ = ptr[1];
      *dest++ = *ptr;
      ptr += 3;
      }
    dest = dest + (dataWidth - srcWidth);
    }

  AVIStreamWrite(this->Internals->StreamCompressed,  // stream pointer
                 this->Time, // time of this frame
                 1,        // number to write
                 (LPBYTE) this->Internals->lpbi +    // pointer to data
                 this->Internals->lpbi->biSize,
                 this->Internals->lpbi->biSizeImage,  // size of this frame
                 AVIIF_KEYFRAME,       // flags....
                 NULL, NULL);
  this->Time++;
}

//---------------------------------------------------------------------------
void vtkAVIWriter::End()
{
  ::GlobalUnlock(this->Internals->hDIB);
  if (this->Internals->Stream)
    {
    AVIStreamClose(this->Internals->Stream);
    this->Internals->Stream = NULL;
    }
  
  if (this->Internals->StreamCompressed)
    {
    AVIStreamClose(this->Internals->StreamCompressed);
    this->Internals->StreamCompressed = NULL;
    }

  if (this->Internals->AVIFile)
    {
    AVIFileClose(this->Internals->AVIFile);
    this->Internals->AVIFile = NULL;
    }
  
  AVIFileExit();          // releases AVIFile library 
}

//---------------------------------------------------------------------------
void vtkAVIWriter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);  
}

