/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkMapper.hh
  Language:  C++
  Date:      $Date$
  Version:   $Revision$


Copyright (c) 1993-1995 Ken Martin, Will Schroeder, Bill Lorensen.

This software is copyrighted by Ken Martin, Will Schroeder and Bill Lorensen.
The following terms apply to all files associated with the software unless
explicitly disclaimed in individual files. This copyright specifically does
not apply to the related textbook "The Visualization Toolkit" ISBN
013199837-4 published by Prentice Hall which is covered by its own copyright.

The authors hereby grant permission to use, copy, and distribute this
software and its documentation for any purpose, provided that existing
copyright notices are retained in all copies and that this notice is included
verbatim in any distributions. Additionally, the authors grant permission to
modify this software and its documentation for any purpose, provided that
such modifications are not distributed without the explicit consent of the
authors and that existing copyright notices are retained in all copies. Some
of the algorithms implemented by this software are patented, observe all
applicable patent law.

IN NO EVENT SHALL THE AUTHORS OR DISTRIBUTORS BE LIABLE TO ANY PARTY FOR
DIRECT, INDIRECT, SPECIAL, INCIDENTAL, OR CONSEQUENTIAL DAMAGES ARISING OUT
OF THE USE OF THIS SOFTWARE, ITS DOCUMENTATION, OR ANY DERIVATIVES THEREOF,
EVEN IF THE AUTHORS HAVE BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

THE AUTHORS AND DISTRIBUTORS SPECIFICALLY DISCLAIM ANY WARRANTIES, INCLUDING,
BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A
PARTICULAR PURPOSE, AND NON-INFRINGEMENT.  THIS SOFTWARE IS PROVIDED ON AN
"AS IS" BASIS, AND THE AUTHORS AND DISTRIBUTORS HAVE NO OBLIGATION TO PROVIDE
MAINTENANCE, SUPPORT, UPDATES, ENHANCEMENTS, OR MODIFICATIONS.


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

#include "vtkObject.hh"
#include "vtkLookupTable.hh"
#include "vtkDataSet.hh"

class vtkRenderer;
class vtkActor;

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
  virtual void Render(vtkRenderer *ren, vtkActor *a) = 0;

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
  // Specify range in terms of scalar minimum and maximum (smin,smax). These
  // values are used to map scalars into lookup table.
  vtkSetVector2Macro(ScalarRange,float);
  vtkGetVectorMacro(ScalarRange,float,2);

  // Description:
  // Return bounding box of data in terms of (xmin,xmax, ymin,ymax, zmin,zmax).
  // Used in the rendering process to automatically create a camera in the 
  // proper initial configuration.
  virtual float *GetBounds() = 0;

  float *GetCenter();

  // Description:
  // Update the network connected to this mapper.
  virtual void Update();

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


