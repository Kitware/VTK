/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMPEG2Writer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkMPEG2Writer - Writes MPEG2 Movie files.
//
// .SECTION Description
// vtkMPEG2Writer writes Movie files. The data type
// of the file is unsigned char regardless of the input type.
//
// This class is conditionally compiled into VTK only if VTK's CMake
// option VTK_USE_MPEG2_ENCODER is ON. It is OFF by default.
//
// Portions of the mpeg2 library are patented. VTK does not enable linking to
// this library by default so VTK can remain "patent free". Users who wish to
// link in mpeg2 functionality must build that library separately and then
// turn on VTK_USE_MPEG2_ENCODER when configuring VTK. After turning on
// VTK_USE_MPEG2_ENCODER, you must also set the CMake variables
// vtkMPEG2Encode_INCLUDE_PATH and vtkMPEG2Encode_LIBRARIES.
//
// You are solely responsible for any legal issues associated with using
// patented code in your software.
//
// You can download a "CMake-ified" source tree of the MPEG2 library by
// visiting the download page at http://www.vtk.org and scrolling down to
// the "Download Additional Components" section.
//
// .SECTION See Also
// vtkGenericMovieWriter vtkAVIWriter vtkFFMPEGWriter

#ifndef __vtkMPEG2Writer_h
#define __vtkMPEG2Writer_h

#include "vtkIOMovieModule.h" // For export macro
#include "vtkGenericMovieWriter.h"

class vtkMPEG2WriterInternal;
class vtkImageData;
struct MPEG2_structure;

class VTKIOMOVIE_EXPORT vtkMPEG2Writer : public vtkGenericMovieWriter
{
public:
  static vtkMPEG2Writer *New();
  vtkTypeMacro(vtkMPEG2Writer,vtkGenericMovieWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // These methods start writing an Movie file, write a frame to the file
  // and then end the writing process.
  void Start();
  void Write();
  void End();

protected:
  vtkMPEG2Writer();
  ~vtkMPEG2Writer();

  vtkMPEG2WriterInternal *Internals;

  long Time;
  int ActualWrittenTime;

  void Initialize();

  int Initialized;

  MPEG2_structure* MPEGStructure;

private:
  vtkMPEG2Writer(const vtkMPEG2Writer&); // Not implemented
  void operator=(const vtkMPEG2Writer&); // Not implemented
};

#endif
