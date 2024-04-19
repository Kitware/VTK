// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkMP4Writer
 * @brief   Writes Windows MP4 files on Windows platforms.
 *
 * vtkMP4Writer writes H.264-encoded MP4 files. Note that this class in only available
 * on the Microsoft Windows platform.
 *
 * Implementation inspired from the following tutorial:
 * https://docs.microsoft.com/en-us/windows/win32/medfound/tutorial--using-the-sink-writer-to-encode-video?redirectedfrom=MSDN
 * @sa
 * vtkGenericMovieWriter vtkAVIWriter
 */

#ifndef vtkMP4Writer_h
#define vtkMP4Writer_h

#include "vtkGenericMovieWriter.h"
#include "vtkIOMovieModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class VTKIOMOVIE_EXPORT vtkMP4Writer : public vtkGenericMovieWriter
{
public:
  static vtkMP4Writer* New();
  vtkTypeMacro(vtkMP4Writer, vtkGenericMovieWriter);
  void PrintSelf(ostream& os, vtkIndent indent) override;

  ///@{
  /**
   * These methods start writing an MP4 file, write a frame to the file
   * and then end the writing process.
   */
  void Start() override;
  void Write() override;
  void End() override;
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
   * Set/Get the average bit rate of the video.
   * Higher produces better quality, but a larger file size.
   */
  vtkSetMacro(BitRate, int);
  vtkGetMacro(BitRate, int);
  ///@}

protected:
  vtkMP4Writer();
  ~vtkMP4Writer() override;

  class vtkMP4WriterInternals;
  vtkMP4WriterInternals* Internals;

  bool Writing = false;
  int Rate;
  int BitRate;

private:
  vtkMP4Writer(const vtkMP4Writer&) = delete;
  void operator=(const vtkMP4Writer&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
