// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-FileCopyrightText: Copyright (c) Michael Wild
// SPDX-License-Identifier: BSD-3-Clause
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

#include "vtkGenericMovieWriter.h"
#include "vtkIOOggTheoraModule.h" // For export macro

VTK_ABI_NAMESPACE_BEGIN
class vtkOggTheoraWriterInternal;

class VTKIOOGGTHEORA_EXPORT vtkOggTheoraWriter : public vtkGenericMovieWriter
{
public:
  static vtkOggTheoraWriter* New();
  vtkTypeMacro(vtkOggTheoraWriter, vtkGenericMovieWriter);
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
   * Set/Get the frame rate, in frame/s.
   */
  vtkSetClampMacro(Rate, int, 1, 5000);
  vtkGetMacro(Rate, int);
  ///@}

  ///@{
  /**
   * Is the video to be encoded using 4:2:0 subsampling?
   */
  vtkSetMacro(Subsampling, vtkTypeBool);
  vtkGetMacro(Subsampling, vtkTypeBool);
  vtkBooleanMacro(Subsampling, vtkTypeBool);
  ///@}

protected:
  vtkOggTheoraWriter();
  ~vtkOggTheoraWriter() override;

  vtkOggTheoraWriterInternal* Internals;

  int Initialized;
  int Quality;
  int Rate;
  vtkTypeBool Subsampling;

private:
  vtkOggTheoraWriter(const vtkOggTheoraWriter&) = delete;
  void operator=(const vtkOggTheoraWriter&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
