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
// .NAME vtkWebApplication - defines ParaViewWeb application interface.
// .SECTION Description
// vtkWebApplication defines the core interface for a ParaViewWeb application.
// This exposes methods that make it easier to manage views and rendered images
// from views.

#ifndef __vtkWebApplication_h
#define __vtkWebApplication_h

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

  // Description:
  // Set the encoding to be used for rendered images.
  enum
    {
    ENCODING_NONE=0,
    ENCODING_BASE64=1
    };
  vtkSetClampMacro(ImageEncoding, int, ENCODING_NONE, ENCODING_BASE64);
  vtkGetMacro(ImageEncoding, int);

  // Description:
  // Set the compression to be used for rendered images.
  enum
    {
    COMPRESSION_NONE=0,
    COMPRESSION_PNG=1,
    COMPRESSION_JPEG=2
    };
  vtkSetClampMacro(ImageCompression, int, COMPRESSION_NONE, COMPRESSION_JPEG);
  vtkGetMacro(ImageCompression, int);

  // Description:
  // Render a view and obtain the rendered image.
  vtkUnsignedCharArray* StillRender(vtkRenderWindow* view, int quality = 100);
  vtkUnsignedCharArray* InteractiveRender(vtkRenderWindow* view, int quality = 50);
  const char* StillRenderToString(vtkRenderWindow* view, unsigned long time = 0, int quality = 100);

  // Description:
  // StillRenderToString() need not necessary returns the most recently rendered
  // image. Use this method to get whether there are any pending images being
  // processed concurrently.
  bool GetHasImagesBeingProcessed(vtkRenderWindow*);

  // Description:
  // Communicate mouse interaction to a view.
  // Returns true if the interaction changed the view state, otherwise returns false.
  bool HandleInteractionEvent(
    vtkRenderWindow* view, vtkWebInteractionEvent* event);

  // Description:
  // Invalidate view cache
  void InvalidateCache(vtkRenderWindow* view);

  // Description:
  // Return the MTime of the last array exported by StillRenderToString.
  vtkGetMacro(LastStillRenderToStringMTime, unsigned long);

  // Description:
  // Return the Meta data description of the input scene in JSON format.
  // This is using the vtkWebGLExporter to parse the scene.
  // NOTE: This should be called before getting the webGL binary data.
  const char* GetWebGLSceneMetaData(vtkRenderWindow* view);

  // Description:
  // Return the binary data given the part index
  // and the webGL object piece id in the scene.
  const char* GetWebGLBinaryData(vtkRenderWindow *view, const char* id, int partIndex);

  vtkObjectIdMap* GetObjectIdMap();

//BTX
protected:
  vtkWebApplication();
  ~vtkWebApplication();

  int ImageEncoding;
  int ImageCompression;
  unsigned long LastStillRenderToStringMTime;

private:
  vtkWebApplication(const vtkWebApplication&); // Not implemented
  void operator=(const vtkWebApplication&); // Not implemented

  class vtkInternals;
  vtkInternals* Internals;

//ETX
};

#endif
