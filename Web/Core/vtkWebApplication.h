/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkWebApplication.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   vtkWebApplication
 * @brief   defines ParaViewWeb application interface.
 *
 * vtkWebApplication defines the core interface for a ParaViewWeb application.
 * This exposes methods that make it easier to manage views and rendered images
 * from views.
*/

#ifndef vtkWebApplication_h
#define vtkWebApplication_h

#include "vtkObject.h"
#include "vtkWebCoreModule.h" // needed for exports

class vtkObjectIdMap;
class vtkRenderWindow;
class vtkUnsignedCharArray;
class vtkWebInteractionEvent;

class VTKWEBCORE_EXPORT vtkWebApplication : public vtkObject
{
public:
  static vtkWebApplication* New();
  vtkTypeMacro(vtkWebApplication, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);

  //@{
  /**
   * Set the encoding to be used for rendered images.
   */
  enum
  {
    ENCODING_NONE=0,
    ENCODING_BASE64=1
  };
  vtkSetClampMacro(ImageEncoding, int, ENCODING_NONE, ENCODING_BASE64);
  vtkGetMacro(ImageEncoding, int);
  //@}

  //@{
  /**
   * Set the compression to be used for rendered images.
   */
  enum
  {
    COMPRESSION_NONE=0,
    COMPRESSION_PNG=1,
    COMPRESSION_JPEG=2
  };
  vtkSetClampMacro(ImageCompression, int, COMPRESSION_NONE, COMPRESSION_JPEG);
  vtkGetMacro(ImageCompression, int);
  //@}

  //@{
  /**
   * Render a view and obtain the rendered image.
   */
  vtkUnsignedCharArray* StillRender(vtkRenderWindow* view, int quality = 100);
  vtkUnsignedCharArray* InteractiveRender(vtkRenderWindow* view, int quality = 50);
  const char* StillRenderToString(vtkRenderWindow* view, vtkMTimeType time = 0, int quality = 100);
  //@}

  /**
   * StillRenderToString() need not necessary returns the most recently rendered
   * image. Use this method to get whether there are any pending images being
   * processed concurrently.
   */
  bool GetHasImagesBeingProcessed(vtkRenderWindow*);

  /**
   * Communicate mouse interaction to a view.
   * Returns true if the interaction changed the view state, otherwise returns false.
   */
  bool HandleInteractionEvent(
    vtkRenderWindow* view, vtkWebInteractionEvent* event);

  /**
   * Invalidate view cache
   */
  void InvalidateCache(vtkRenderWindow* view);

  //@{
  /**
   * Return the MTime of the last array exported by StillRenderToString.
   */
  vtkGetMacro(LastStillRenderToStringMTime, vtkMTimeType);
  //@}

  /**
   * Return the Meta data description of the input scene in JSON format.
   * This is using the vtkWebGLExporter to parse the scene.
   * NOTE: This should be called before getting the webGL binary data.
   */
  const char* GetWebGLSceneMetaData(vtkRenderWindow* view);

  /**
   * Return the binary data given the part index
   * and the webGL object piece id in the scene.
   */
  const char* GetWebGLBinaryData(vtkRenderWindow *view, const char* id, int partIndex);

  vtkObjectIdMap* GetObjectIdMap();

protected:
  vtkWebApplication();
  ~vtkWebApplication();

  int ImageEncoding;
  int ImageCompression;
  vtkMTimeType LastStillRenderToStringMTime;

private:
  vtkWebApplication(const vtkWebApplication&) VTK_DELETE_FUNCTION;
  void operator=(const vtkWebApplication&) VTK_DELETE_FUNCTION;

  class vtkInternals;
  vtkInternals* Internals;

};

#endif
