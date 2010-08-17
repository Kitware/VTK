/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFFMPEGWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkFFMPEGWriter - Uses the FFMPEG library to write video files.
// .SECTION Description
// vtkFFMPEGWriter is an adapter that allows VTK to use the LGPL'd FFMPEG
// library to write movie files. FFMPEG can create a variety of multimedia
// file formats and can use a variety of encoding algorithms (codecs).
// This class creates .avi files containing MP43 encoded video without
// audio.
//
// The FFMPEG multimedia library source code can be obtained from
// the sourceforge web site at http://ffmpeg.sourceforge.net/download.php
// or is a tarball along with installation instructions at
// http://www.vtk.org/files/support/ffmpeg_source.tar.gz
//
// .SECTION See Also vtkGenericMovieWriter vtkAVIWriter vtkMPEG2Writer

#ifndef __vtkFFMPEGWriter_h
#define __vtkFFMPEGWriter_h

#include "vtkGenericMovieWriter.h"

class vtkFFMPEGWriterInternal;

class VTK_IO_EXPORT vtkFFMPEGWriter : public vtkGenericMovieWriter
{
public:
  static vtkFFMPEGWriter *New();
  vtkTypeMacro(vtkFFMPEGWriter,vtkGenericMovieWriter);
  void PrintSelf(ostream& os, vtkIndent indent);
  
  // Description:
  // These methods start writing an Movie file, write a frame to the file
  // and then end the writing process.
  void Start();
  void Write();
  void End();

  // Description:
  // Set/Get the compression quality.
  // 0 means worst quality and smallest file size
  // 2 means best quality and largest file size
  vtkSetClampMacro(Quality, int, 0, 2);
  vtkGetMacro(Quality, int);
 
  // Description:
  // Set/Get the frame rate, in frame/s.
  vtkSetClampMacro(Rate, int , 1, 5000);
  vtkGetMacro(Rate, int);

  // Description:
  // Set/Get the bit-rate
  vtkSetMacro(BitRate, int);
  vtkGetMacro(BitRate, int);

  // Description:
  // Set/Get the bit-rate tolerance
  vtkSetMacro(BitRateTolerance, int);
  vtkGetMacro(BitRateTolerance, int);

protected:
  vtkFFMPEGWriter();
  ~vtkFFMPEGWriter();

  vtkFFMPEGWriterInternal *Internals;

  int Initialized;
  int Quality;
  int Rate;
  int BitRate;
  int BitRateTolerance;

private:
  vtkFFMPEGWriter(const vtkFFMPEGWriter&); // Not implemented
  void operator=(const vtkFFMPEGWriter&); // Not implemented
};

#endif



