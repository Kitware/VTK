// SPDX-FileCopyrightText: Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
// SPDX-License-Identifier: BSD-3-Clause
/**
 * @class   vtkWin32VideoSource
 * @brief   Video-for-Windows video digitizer
 *
 * vtkWin32VideoSource grabs frames or streaming video from a
 * Video for Windows compatible device on the Win32 platform.
 * @warning
 * With some capture cards, if this class is leaked and ReleaseSystemResources
 * is not called, you may have to reboot before you can capture again.
 * vtkVideoSource used to keep a global list and delete the video sources
 * if your program leaked, due to exit crashes that was removed.
 *
 * @sa
 * vtkVideoSource vtkMILVideoSource
 */

#ifndef vtkWin32VideoSource_h
#define vtkWin32VideoSource_h

#include "vtkIOVideoModule.h" // For export macro
#include "vtkVideoSource.h"

VTK_ABI_NAMESPACE_BEGIN
class vtkWin32VideoSourceInternal;

class VTKIOVIDEO_EXPORT vtkWin32VideoSource : public vtkVideoSource
{
public:
  static vtkWin32VideoSource* New();
  vtkTypeMacro(vtkWin32VideoSource, vtkVideoSource);
  void PrintSelf(ostream& os, vtkIndent indent) override;

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

  ///@{
  /**
   * Request a particular frame size (set the third value to 1).
   */
  void SetFrameSize(int x, int y, int z) override;
  void SetFrameSize(int dim[3]) override { this->SetFrameSize(dim[0], dim[1], dim[2]); }
  ///@}

  /**
   * Request a particular frame rate (default 30 frames per second).
   */
  void SetFrameRate(float rate) override;

  /**
   * Request a particular output format (default: VTK_RGB).
   */
  void SetOutputFormat(int format) override;

  ///@{
  /**
   * Turn on/off the preview (overlay) window.
   */
  void SetPreview(int p);
  vtkBooleanMacro(Preview, int);
  vtkGetMacro(Preview, int);
  ///@}

  /**
   * Bring up a modal dialog box for video format selection.
   */
  void VideoFormatDialog();

  /**
   * Bring up a modal dialog box for video input selection.
   */
  void VideoSourceDialog();

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

  ///@{
  /**
   * For internal use only
   */
  void LocalInternalGrab(void*);
  void OnParentWndDestroy();
  ///@}

protected:
  vtkWin32VideoSource();
  ~vtkWin32VideoSource() override;

  char WndClassName[16];
  int BitMapSize;
  int Preview;

  vtkWin32VideoSourceInternal* Internal;

  void CheckBuffer();
  void UnpackRasterLine(char* outptr, char* inptr, int start, int count) override;

  void DoVFWFormatSetup();
  void DoVFWFormatCheck();

private:
  vtkWin32VideoSource(const vtkWin32VideoSource&) = delete;
  void operator=(const vtkWin32VideoSource&) = delete;
};

VTK_ABI_NAMESPACE_END
#endif
