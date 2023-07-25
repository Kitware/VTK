// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkFFMPEGWriter
 * @brief   Uses the FFMPEG library to write video files.
 *
 * vtkFFMPEGWriter is an adapter that allows VTK to use the LGPL'd FFMPEG
 * library to write movie files. FFMPEG can create a variety of multimedia
 * file formats and can use a variety of encoding algorithms (codecs).
 * This class creates .avi files containing MP43 encoded video without
 * audio.
 *
 * The FFMPEG multimedia library source code can be obtained from
 * the sourceforge web site at http://ffmpeg.sourceforge.net/download.php
 * or is a tarball along with installation instructions at
 * http://www.vtk.org/files/support/ffmpeg_source.tar.gz
 *
 */

#ifndef vtkFFMPEGWriter_h
#define vtkFFMPEGWriter_h

#include "vtkGenericMovieWriter.h"
#include "vtkIOFFMPEGModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkFFMPEGWriterInternal;

class VTKIOFFMPEG_EXPORT vtkFFMPEGWriter : public vtkGenericMovieWriter
{
public:
  static vtkFFMPEGWriter* New();
  vtkTypeMacro(vtkFFMPEGWriter, vtkGenericMovieWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * These methods start writing an Movie file, write a frame to the file
   * and then end the writing process.
   */
  void Start() override;
  void Write() override;
  void End() override;
  ///@}

  ///@{
  /**
   * Set/Get the compression quality.
   * 0 means worst quality and smallest file size
   * 2 means best quality and largest file size
   */
  vtkSetClampMacro(Quality, int, 0, 2);
  vtkGetMacro(Quality, int);
  ///@}

  ///@{
  /**
   * Turns on(the default) or off compression.
   * Turning off compression overrides quality setting.
   */
  vtkSetMacro(Compression, bool);
  vtkGetMacro(Compression, bool);
  vtkBooleanMacro(Compression, bool);
  ///@}

  ///@{
  /**
   * Set/Get the frame rate, in frame/s.
   */
  vtkSetClampMacro(Rate, int, 1, 5000);
  vtkGetMacro(Rate, int);
  ///@}

  ///@{
  /**
   * Set/Get the bit-rate
   */
  vtkSetMacro(BitRate, int);
  vtkGetMacro(BitRate, int);
  ///@}

  ///@{
  /**
   * Set/Get the bit-rate tolerance
   */
  vtkSetMacro(BitRateTolerance, int);
  vtkGetMacro(BitRateTolerance, int);
  ///@}

protected:
  vtkFFMPEGWriter();
  ~vtkFFMPEGWriter() override;

  vtkFFMPEGWriterInternal* Internals;

  int Initialized;
  int Quality;
  int Rate;
  int BitRate;
  int BitRateTolerance;
  bool Compression;

private:
  vtkFFMPEGWriter(const vtkFFMPEGWriter&) = delete;
  void operator=(const vtkFFMPEGWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
