/*=========================================================================

  Program:   Visualization Toolkit
  Module:    RenderW.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994

=========================================================================*/
// .NAME vtkRenderWindow - create a window for renderers to draw into
// .SECTION Description
// vtkRenderWindow is an abstract object to specify the behavior of a
// rendering window. A rendering window is a window in a graphical user
// interface where renderers draw their images. Methods are provided to 
// synchronize the rendering process, set window size, and control double
// buffering. Another set of important methods allow the creation of
// device dependent actors, lights, and cameras. These objects are created
// depending upon the value of the environment variable "VTK_RENDERER".

#ifndef __vtkRenderWindow_hh
#define __vtkRenderWindow_hh

#include "Object.hh"
#include "RenderC.hh"

class vtkRenderWindowInteractor;
class vtkLightDevice;
class vtkCameraDevice;
class vtkTextureDevice;
class vtkPropertyDevice;

// lets define the diferent types of stereo
#define VTK_STEREO_CRYSTAL_EYES 1
#define VTK_STEREO_RED_BLUE     2

class vtkRenderWindow : public vtkObject
{
public:
  vtkRenderWindow();
  char *GetClassName() {return "vtkRenderWindow";};
  void PrintSelf(ostream& os, vtkIndent indent);

  void AddRenderers(vtkRenderer *);
  void RemoveRenderers(vtkRenderer *);
  vtkRendererCollection *GetRenderers() {return &(this->Renderers);};

  virtual void Render();

  // Description:
  // Initialize rendering process.
  virtual void Start() = 0;

  // Description:
  // Performed at the end of the rendering process to generate image.
  virtual void Frame() = 0;

  // Description:
  // Performed at the end of the rendering process to generate image.
  virtual void CopyResultFrame();

  // Description:
  // Create a device specific renderer.
  virtual vtkRenderer  *MakeRenderer() = 0;

  // Description:
  // Create a device specific light.
  virtual vtkLightDevice *MakeLight() = 0;

  // Description:
  // Create a device specific camera.
  virtual vtkCameraDevice    *MakeCamera() = 0;

  // Description:
  // Create a device specific property.
  virtual vtkPropertyDevice    *MakeProperty() = 0;

  // Description:
  // Create a device specific texture.
  virtual vtkTextureDevice     *MakeTexture() = 0;

  // Description:
  // Create an interactor to control renderers in this window.
  virtual vtkRenderWindowInteractor *MakeRenderWindowInteractor() = 0;

  // Description:
  // Get the position in screen coordinates of the rendering window.
  virtual int *GetPosition() = 0;

  // Description:
  // Set the position of the window in screen coordinates.
  virtual void SetPosition(int,int);
  virtual void SetPosition(int a[2]);

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
  vtkGetMacro(FullScreen,int);
  vtkBooleanMacro(FullScreen,int);

  // Description:
  // Turn on/off window manager borders.
  vtkSetMacro(Borders,int);
  vtkGetMacro(Borders,int);
  vtkBooleanMacro(Borders,int);

  // Description:
  // Keep track of whether rendering window has been mapped to screen.
  vtkSetMacro(Mapped,int);
  vtkGetMacro(Mapped,int);
  vtkBooleanMacro(Mapped,int);

  // Description:
  // Turn on/off double buffering.
  vtkSetMacro(DoubleBuffer,int);
  vtkGetMacro(DoubleBuffer,int);
  vtkBooleanMacro(DoubleBuffer,int);

  // Description:
  // Turn on/off stereo rendering.
  vtkGetMacro(StereoRender,int);
  vtkSetMacro(StereoRender,int);
  vtkBooleanMacro(StereoRender,int);

  // Description:
  // Set what type of stereo rendering to use.
  vtkGetMacro(StereoType,int);
  vtkSetMacro(StereoType,int);

  virtual void StereoUpdate();
  virtual void StereoMidpoint();
  virtual void StereoRenderComplete();

  virtual int  GetRemapWindow();

  // Description:
  // Get name of rendering window
  vtkGetStringMacro(Name);

  // Description:
  // Set/Get the filename used for saving images.
  vtkSetStringMacro(Filename);
  vtkGetStringMacro(Filename);


  // Description:
  // Save the current image as a PPM file.
  virtual void SaveImageAsPPM();


  // Description:
  // Set/Get the pixel data of an image, transmitted as RGBRGB... 
  virtual unsigned char *GetPixelData(int x,int y,int x2,int y2) = 0;
  virtual void SetPixelData(int x,int y,int x2,int y2,unsigned char *) = 0;

  // Description:
  // Set the number of frames for doing anti aliasing, default is zero.
  vtkGetMacro(AAFrames,int);
  vtkSetMacro(AAFrames,int);

  // Description:
  // Set the number of frames for doing focal depth, default is zero.
  vtkGetMacro(FDFrames,int);
  vtkSetMacro(FDFrames,int);

  // Description:
  // Set the number of sub frames for doing motion blur.
  vtkGetMacro(SubFrames,int);
  vtkSetMacro(SubFrames,int);

protected:
  virtual void DoStereoRender();
  virtual void DoFDRender();
  virtual void DoAARender();

  vtkRendererCollection Renderers;
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
  vtkRenderWindowInteractor *Interactor;
  char *Filename;
  unsigned char* temp_buffer;  // used for red blue stereo
  unsigned char** AABuffer;    // used for anti aliasing
  int AAFrames;
  unsigned char** FDBuffer;    // used for focal depth
  int FDFrames;
  unsigned char** SubBuffer;   // used for sub frames
  int SubFrames;               // number of sub frames
  int CurrentSubFrame;         // what one are we on
  unsigned char* ResultFrame;  // used for any non immediate rendering
};

#endif


