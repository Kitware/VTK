/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkFFMPEGVideoSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkFFMPEGVideoSource
 * @brief   Reader for ffmpeg supported formats
 *
 *
 * @sa
 * vtkVideoSource
*/

#ifndef vtkFFMPEGVideoSource_h
#define vtkFFMPEGVideoSource_h

#include "vtkIOFFMPEGModule.h" // For export macro
#include "vtkVideoSource.h"
#include "vtkMultiThreader.h" // for ivar

class vtkFFMPEGVideoSourceInternal;

class VTKIOFFMPEG_EXPORT vtkFFMPEGVideoSource : public vtkVideoSource
{
public:
  static vtkFFMPEGVideoSource *New();
  vtkTypeMacro(vtkFFMPEGVideoSource,vtkVideoSource);

  /**
   * Standard VCR functionality: Record incoming video.
   */
  void Record() override;

  /**
   * Standard VCR functionality: Play recorded video.
   */
  void Play() override;

  /**
   * Standard VCR functionality: Stop recording or playing.
   */
  void Stop() override;

  /**
   * Grab a single video frame.
   */
  void Grab() override;

  //@{
  /**
   * Request a particular frame size (set the third value to 1).
   */
  void SetFrameSize(int x, int y, int z) override;
  void SetFrameSize(int dim[3]) override {
    this->SetFrameSize(dim[0], dim[1], dim[2]); };
  //@}

  /**
   * Request a particular frame rate (default 30 frames per second).
   */
  void SetFrameRate(float rate) override;

  /**
   * Request a particular output format (default: VTK_RGB).
   */
  void SetOutputFormat(int format) override;

  /**
   * Initialize the driver (this is called automatically when the
   * first grab is done).
   */
  void Initialize() override;

  /**
   * Free the driver (this is called automatically inside the
   * destructor).
   */
  void ReleaseSystemResources() override;

  //@{
  /**
   * Specify file name of the video
   */
  vtkSetStringMacro(FileName);
  vtkGetStringMacro(FileName);
  //@}

  /**
   * The internal function which actually does the grab.  You will
   * definitely want to override this if you develop a vtkVideoSource
   * subclass.
   */
  void InternalGrab() override;


protected:
  vtkFFMPEGVideoSource();
  ~vtkFFMPEGVideoSource();

  int DecodePacket(int *got_frame);
  static void *RecordThread(
    vtkMultiThreader::ThreadInfo *data);

  vtkFFMPEGVideoSourceInternal *Internal;
  char *FileName;
  bool EndOfFile;

private:
  vtkFFMPEGVideoSource(const vtkFFMPEGVideoSource&) = delete;
  void operator=(const vtkFFMPEGVideoSource&) = delete;
};

#endif
