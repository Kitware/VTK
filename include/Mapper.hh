/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Mapper.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$

This file is part of the Visualization Toolkit. No part of this file or its 
contents may be copied, reproduced or altered in any way without the express
written consent of the authors.

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen 1993, 1994 

=========================================================================*/
// .NAME vtkMapper - abstract class specifies interface to map data to graphics primitives
// .SECTION Description
// vtkMapper is an abstract class to specify interface between data and 
// graphics primitives. Subclasses of vtkMapper map data through a 
// lookuptable and control the creation of rendering primitives that
// interface to the graphics library. The mapping can be controlled by 
// supplying a lookup table and specifying a scalar range to map data
// through.

#ifndef __vtkMapper_hh
#define __vtkMapper_hh

#include "Object.hh"
#include "GeomPrim.hh"
#include "Lut.hh"
#include "DataSet.hh"

class vtkRenderer;

class vtkMapper : public vtkObject 
{
public:
  vtkMapper();
  ~vtkMapper();
  char *GetClassName() {return "vtkMapper";};
  void PrintSelf(ostream& os, vtkIndent indent);
  void operator=(const vtkMapper& m);

  unsigned long int GetMTime();

  void SetStartRender(void (*f)(void *), void *arg);
  void SetEndRender(void (*f)(void *), void *arg);
  void SetStartRenderArgDelete(void (*f)(void *));
  void SetEndRenderArgDelete(void (*f)(void *));

  // Description:
  // Method initiates the mapping process. Generally sent by the actor 
  // as each frame is rendered.
  virtual void Render(vtkRenderer *) = 0;

  void SetLookupTable(vtkLookupTable *lut);
  void SetLookupTable(vtkLookupTable& lut) {this->SetLookupTable(&lut);};
  vtkLookupTable *GetLookupTable();

  // Description:
  // Create default lookup table. Generally used to create one when none
  // is available.
  virtual void CreateDefaultLookupTable();

  // Description:
  // Turn on/off flag to control whether scalar data is used to color objects.
  vtkSetMacro(ScalarsVisible,int);
  vtkGetMacro(ScalarsVisible,int);
  vtkBooleanMacro(ScalarsVisible,int);

  // Description:
  // Specify range in terms of (smin,smax) through which to map scalars
  // into lookup table.
  vtkSetVector2Macro(ScalarRange,float);
  vtkGetVectorMacro(ScalarRange,float,2);

  // Description:
  // Return bounding box of data in terms of (xmin,xmax, ymin,ymax, zmin,zmax).
  // Used in the rendering process to automatically create a camera in the 
  // proper initial configuration.
  virtual float *GetBounds() = 0;

  float *GetCenter();
  virtual vtkDataSet *GetInput() {return this->Input;};

protected:
  vtkDataSet *Input;

  void (*StartRender)(void *);
  void (*StartRenderArgDelete)(void *);
  void *StartRenderArg;
  void (*EndRender)(void *);
  void (*EndRenderArgDelete)(void *);
  void *EndRenderArg;
  vtkLookupTable *LookupTable;
  int ScalarsVisible;
  vtkTimeStamp BuildTime;
  float ScalarRange[2];
  int SelfCreatedLookupTable;

};

#endif


