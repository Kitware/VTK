/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkPushPipeline.h
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen 
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even 
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR 
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkPushPipeline - run a pipeline from a data push perspective
// .SECTION Description
// vtkPushPipeline is a class designed to execute a VTK visualization
// pipeline not from a demand driven approach, but from a data push approach.

// .SECTION See also
// vtkPushImageReader

#ifndef __vtkPushPipeline_h
#define __vtkPushPipeline_h

#include "vtkObject.h"
#define VTK_PP_MAX_INPUTS 128

class vtkProcessObject;
class vtkDataObject;
class vtkSource;
class vtkPushPipelineConsumeCommand;
class vtkRenderWindow;
class vtkRenderer;
class vtkPushPipelineProcessInfo;
class vtkPushPipelineDataInfo;

//BTX
template <class T> class vtkVector;
template <class K, class D> class vtkArrayMap;
//ETX
class VTK_HYBRID_EXPORT vtkPushPipeline : public vtkObject
{
public:
  static vtkPushPipeline *New();
  vtkTypeRevisionMacro(vtkPushPipeline,vtkObject);

  // Description:
  // Add a Pusher object to the pipeline
  void AddPusher(vtkProcessObject* pusher);

  // Description:
  // Sets the input ratios for a pipeline member
  void SetInputToExecutionRatio(vtkProcessObject *po, int inNum, int ratio);
  void SetExecutionToOutputRatio(vtkProcessObject *po, int ratio);
                             
  // Description:
  // Push data froma pusher
  void Push(vtkSource *pusher);

  // Description:
  // Run a pipeline until pusher indicates it is out of data
  void Run(vtkSource *pusher);

  // Description:
  // What is the run state, a pusher should set this to 2 
  // when it is out of data
  vtkSetMacro(RunState,int);
  vtkGetMacro(RunState,int);

  // Description: 
  // Add a window to the pipeline. Normally you do not need to call this
  // routine. The windows connected to the pipeline can be found
  // automatically.
  void AddWindow(vtkRenderWindow *win);
  
protected:
  vtkPushPipeline();
  ~vtkPushPipeline();
  
  //BTX
  friend vtkPushPipelineProcessInfo;
  friend vtkPushPipelineDataInfo;
  friend vtkPushPipelineConsumeCommand;
  
  vtkVector<vtkRenderWindow *> *Windows;
  vtkArrayMap<vtkProcessObject *, vtkPushPipelineProcessInfo *> *ProcessMap;
  vtkArrayMap<vtkDataObject *, vtkPushPipelineDataInfo *> *DataMap;
  //ETX
  
  vtkPushPipelineDataInfo *GetPushDataInfo(vtkDataObject *);
  vtkPushPipelineProcessInfo *GetPushProcessInfo(vtkProcessObject *);
  int RunState;
  
  void AddData(vtkDataObject *);
  void AddProcess(vtkProcessObject *);
  void Trace(vtkDataObject *);
  void Trace(vtkProcessObject *);
  void ClearTraceMarkers();
  int  IsRendererReady(vtkRenderer *);
  int  IsRenderWindowReady(vtkRenderWindow *);
  void ConsumeRenderWindowInputs(vtkRenderWindow *);
  void ConsumeRendererInputs(vtkRenderer *);
  void RenderWindows();
  void SetupWindows();
  void SetupRenderWindow(vtkRenderWindow *);
  void SetupRenderer(vtkRenderer *);
  
private:
  vtkPushPipeline(const vtkPushPipeline&);  // Not implemented.
  void operator=(const vtkPushPipeline&);  // Not implemented.
};

#endif

