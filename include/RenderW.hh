/*=========================================================================

  Program:   Visualization Library
  Module:    RenderW.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Library. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vlRenderWindow - create a window for renderers to draw into
// .SECTION Description
// vlRenderWindow is an abstract object to specify the behavior of a
// rendering window. A rendering window is a window in a graphical user
// interface where renderers draw their images. Methods are provided to 
// synchronize the rendering process, set window size, and control double
// buffering. Another set of important methods allow the creation of
// device dependent actors, lights, and cameras. These objects are created
// depending upon the value of the environment variable "VL_RENDERER".

#ifndef __vlRenderWindow_hh
#define __vlRenderWindow_hh

#include "Object.hh"
#include "RenderC.hh"

class vlRenderWindowInteractor;

// lets define the diferent types of stereo
#define VL_STEREO_CRYSTAL_EYES 1
#define VL_STEREO_RED_BLUE     2

class vlRenderWindow : public vlObject
{
public:
  vlRenderWindow();
  char *GetClassName() {return "vlRenderWindow";};
  void PrintSelf(ostream& os, vlIndent indent);

  void AddRenderers(vlRenderer *);
  void RemoveRenderers(vlRenderer *);
  vlRendererCollection *GetRenderers() {return &(this->Renderers);};

  virtual void Render();

  // Description:
  // Initialize rendering process.
  virtual void Start() = 0;

  // Description:
  // Performed at the end of the rendering process to generate image.
  virtual void Frame() = 0;

  // Description:
  // Create a device specific renderer.
  virtual vlRenderer  *MakeRenderer() = 0;

  // Description:
  // Create a device specific actor.
  virtual vlActor     *MakeActor() = 0;

  // Description:
  // Create a device specific light.
  virtual vlLight     *MakeLight() = 0;

  // Description:
  // Create a device specific camera.
  virtual vlCamera    *MakeCamera() = 0;

  // Description:
  // Create a device specific property.
  virtual vlProperty    *MakeProperty() = 0;

  // Description:
  // Create an interactor to control renderers in this window.
  virtual vlRenderWindowInteractor *MakeRenderWindowInteractor() = 0;

  // Description:
  // Get the position in screen coordinates of the rendering window.
  virtual int *GetPosition() = 0;

  // Description:
  // Get the size of the window in screen coordinates.
  virtual int *GetSize() = 0;

  // Description:
  // Set the size of the window in screen coordinates.
  virtual void SetSize(int,int) = 0;
  virtual void SetSize(int a[2]);

  // Description:
  // Turn on/off rendering full screen window size.
  virtual void SetFullScreen(int) = 0;
  vlGetMacro(FullScreen,int);
  vlBooleanMacro(FullScreen,int);

  // Description:
  // Turn on/off window manager borders.
  vlSetMacro(Borders,int);
  vlGetMacro(Borders,int);
  vlBooleanMacro(Borders,int);

  // Description:
  // Keep track of whether rendering window has been mapped to screen.
  vlSetMacro(Mapped,int);
  vlGetMacro(Mapped,int);
  vlBooleanMacro(Mapped,int);

  // Description:
  // Turn on/off double buffering.
  vlSetMacro(DoubleBuffer,int);
  vlGetMacro(DoubleBuffer,int);
  vlBooleanMacro(DoubleBuffer,int);

  // Description:
  // Turn on/off stereo rendering.
  vlGetMacro(StereoRender,int);
  vlSetMacro(StereoRender,int);
  vlBooleanMacro(StereoRender,int);

  // Description:
  // Set what type of stereo rendering to use.
  vlGetMacro(StereoType,int);
  vlSetMacro(StereoType,int);

  virtual void StereoUpdate();
  virtual void StereoMidpoint();
  virtual void StereoRenderComplete();

  virtual int  GetRemapWindow();

  // Description:
  // Get name of rendering window
  vlGetStringMacro(Name);

  // Description:
  // Set/Get the filename used for saving images.
  vlSetStringMacro(FileName);
  vlGetStringMacro(FileName);


  // Description:
  // Save the current image as a PPM file.
  virtual void SaveImageAsPPM();


  // Description:
  // Set/Get the pixel data of an image, transmitted as RGBRGB... 
  virtual unsigned char *GetPixelData(int x,int y,int x2,int y2) = 0;
  virtual void SetPixelData(int x,int y,int x2,int y2,unsigned char *) = 0;

protected:
  vlRendererCollection Renderers;
  char Name[80];
  int Size[2];
  int Position[2];
  int Borders;
  int FullScreen;
  int OldScreen[5];
  int Mapped;
  int DoubleBuffer;
  int StereoRender;
  int StereoType;
  int StereoStatus; // used for keeping track of what's going on
  vlRenderWindowInteractor *Interactor;
  char *FileName;
  unsigned char* temp_buffer;  // used for red blue stereo
};

#endif
