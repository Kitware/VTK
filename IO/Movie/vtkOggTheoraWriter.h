/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkOggTheoraWriter.h

  Copyright (c) Michael Wild, Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkOggTheoraWriter
 * @brief   Uses the ogg and theora libraries to write video
 * files.
 *
 * vtkOggTheoraWriter is an adapter that allows VTK to use the ogg and theora
 * libraries to write movie files.  This class creates .ogv files containing
 * theora encoded video without audio.
 *
 * This implementation is based on vtkFFMPEGWriter and uses some code derived
 * from the encoder example distributed with libtheora.
 *
*/

#ifndef vtkOggTheoraWriter_h
#define vtkOggTheoraWriter_h

#include "vtkIOMovieModule.h" // For export macro
#include "vtkGenericMovieWriter.h"

class vtkOggTheoraWriterInternal;

class VTKIOMOVIE_EXPORT vtkOggTheoraWriter : public vtkGenericMovieWriter
{
public:
  static vtkOggTheoraWriter *New();
  vtkTypeMacro(vtkOggTheoraWriter,vtkGenericMovieWriter);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * These methods start writing an Movie file, write a frame to the file
   * and then end the writing process.
   */
  void Start();
  void Write();
  void End();
  //@}

  //@{
  /**
   * Set/Get the compression quality.
   * 0 means worst quality and smallest file size
   * 2 means best quality and largest file size
   */
  vtkSetClampMacro(Quality, int, 0, 2);
  vtkGetMacro(Quality, int);
  //@}

  //@{
  /**
   * Set/Get the frame rate, in frame/s.
   */
  vtkSetClampMacro(Rate, int , 1, 5000);
  vtkGetMacro(Rate, int);
  //@}

  //@{
  /**
   * Is the video to be encoded using 4:2:0 subsampling?
   */
  vtkSetMacro(Subsampling, int);
  vtkGetMacro(Subsampling, int);
  vtkBooleanMacro(Subsampling, int);
  //@}

protected:
  vtkOggTheoraWriter();
  ~vtkOggTheoraWriter();

  vtkOggTheoraWriterInternal *Internals;

  int Initialized;
  int Quality;
  int Rate;
  int Subsampling;

private:
  vtkOggTheoraWriter(const vtkOggTheoraWriter&) VTK_DELETE_FUNCTION;
  void operator=(const vtkOggTheoraWriter&) VTK_DELETE_FUNCTION;
};

#endif



