/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkLegendBoxActor.h
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
// .NAME vtkLegendBoxActor - draw symbols with text
// .SECTION Description
// vtkLegendBoxActor is used to associate a symbol with a text string.
// The user specifies a vtkPolyData to use as the symbol, and a string
// associated with the symbol. The actor can then be placed in the scene
// in the same way that any other vtkActor2D can be used.
//
// To use this class, you must define the position of the legend box by using
// the superclasses' vtkActor2D::Position coordinate and
// Position2 coordinate. Then define the set of symbols and text strings that
// make up the menu box (as well as properties such as font). The class will
// scale the symbols and text to fit in the legend box defined by
// (Position,Position2). Optional features like turning on a border line and
// setting the spacing between the border and the symbols/text can also be
// set.

// .SECTION See Also
// vtkXYPlotActor vtkActor2D vtkGlyphSource2D

#ifndef __vtkLegendBoxActor_h
#define __vtkLegendBoxActor_h

#include "vtkActor2D.h"
#include "vtkTextMapper.h"

class vtkActor;
class vtkPolyData;
class vtkPolyDataMapper;
class vtkPolyDataMapper2D;
class vtkTransformPolyDataFilter;
class vtkTransform;

class VTK_HYBRID_EXPORT vtkLegendBoxActor : public vtkActor2D
{
public:
  vtkTypeRevisionMacro(vtkLegendBoxActor,vtkActor2D);
  void PrintSelf(ostream& os, vtkIndent indent);

  // Description:
  // Instantiate object with a rectangle in normaled view coordinates
  // of (0.2,0.85, 0.8, 0.95).
  static vtkLegendBoxActor *New();

  // Description:
  // Specify the number of entries in the legend box.
  void SetNumberOfEntries(int num);
  int GetNumberOfEntries()
    {return this->NumberOfEntries;}

  // Description:
  // Add an entry to the legend box. You must supply a vtkPolyData to be
  // used as a symbol (it can be NULL) and a text string (which also can
  // be NULL). The vtkPolyData is assumed to be defined in the x-y plane,
  // and the text is assumed to be a single line in height. Note that when
  // this method is invoked previous entries are deleted. Also supply a text
  // string and optionally a color. (If a color is not specified, then the
  // entry color is the same as this actor's color.) (Note: use the set
  // methods when you use SetNumberOfEntries().)
  void SetEntry(int i, vtkPolyData *symbol, const char* string, float color[3]);
  void SetEntrySymbol(int i, vtkPolyData *symbol);
  void SetEntryString(int i, const char* string);
  void SetEntryColor(int i, float color[3]);
  void SetEntryColor(int i, float r, float g, float b);
  vtkPolyData *GetEntrySymbol(int i);
  const char* GetEntryString(int i);
  float *GetEntryColor(int i);

  // Description:
  // Enable/Disable bolding legend entries.
  vtkSetMacro(Bold, int);
  vtkGetMacro(Bold, int);
  vtkBooleanMacro(Bold, int);

  // Description:
  // Enable/Disable italicizing legend entries.
  vtkSetMacro(Italic, int);
  vtkGetMacro(Italic, int);
  vtkBooleanMacro(Italic, int);

  // Description:
  // Enable/Disable creating shadows on the legend entries. Shadows make
  // the text easier to read.
  vtkSetMacro(Shadow, int);
  vtkGetMacro(Shadow, int);
  vtkBooleanMacro(Shadow, int);

  // Description:
  // Set/Get the font family for the legend entries. Three font types
  // are available: Arial (VTK_ARIAL), Courier (VTK_COURIER), and
  // Times (VTK_TIMES).
  vtkSetMacro(FontFamily, int);
  vtkGetMacro(FontFamily, int);
  void SetFontFamilyToArial() {this->SetFontFamily(VTK_ARIAL);};
  void SetFontFamilyToCourier() {this->SetFontFamily(VTK_COURIER);};
  void SetFontFamilyToTimes() {this->SetFontFamily(VTK_TIMES);};

  // Description:
  // Set/Get the flag that controls whether a border will be drawn
  // around the legend box.
  vtkSetMacro(Border, int);
  vtkGetMacro(Border, int);
  vtkBooleanMacro(Border, int);

  // Description:
  // Set/Get the flag that controls whether the border and legend
  // placement is locked into the rectangle defined by (Position,Position2).
  // If off, then the legend box will adjust its size so that the border
  // fits nicely around the text and symbols. (The ivar is off by default.)
  // Note: the legend box is guaranteed to lie within the original border
  // definition.
  vtkSetMacro(LockBorder, int);
  vtkGetMacro(LockBorder, int);
  vtkBooleanMacro(LockBorder, int);

  // Description:
  // Set/Get the padding between the legend entries and the border. The value
  // is specified in pixels.
  vtkSetClampMacro(Padding, int, 0, 50);
  vtkGetMacro(Padding, int);

  // Description:
  // Turn on/off flag to control whether the symbol's scalar data
  // is used to color the symbol. If off, the color of the 
  // vtkLegendBoxActor is used.
  vtkSetMacro(ScalarVisibility,int);
  vtkGetMacro(ScalarVisibility,int);
  vtkBooleanMacro(ScalarVisibility,int);

  // Description:
  // Shallow copy of this scaled text actor. Overloads the virtual
  // vtkProp method.
  void ShallowCopy(vtkProp *prop);

//BTX
  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS.
  // Release any graphics resources that are being consumed by this actor.
  // The parameter window could be used to determine which graphic
  // resources to release.
  virtual void ReleaseGraphicsResources(vtkWindow *);

  // Description:
  // WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
  // DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS.
  // Draw the legend box to the screen.
  int RenderOpaqueGeometry(vtkViewport* viewport);
  int RenderTranslucentGeometry(vtkViewport* ) {return 0;};
  int RenderOverlay(vtkViewport* viewport);
//ETX

protected:
  vtkLegendBoxActor();
  ~vtkLegendBoxActor();

  void InitializeEntries();

  int   Bold;
  int   Italic;
  int   Shadow;
  int   FontFamily;
  int   Border;
  int   Padding;
  int   LockBorder;
  int   ScalarVisibility;

  // Internal actors, mappers, data to represent the legend
  int                        NumberOfEntries;
  int                        Size; //allocation size
  vtkFloatArray              *Colors;
  vtkTextMapper              **TextMapper;
  vtkActor2D                 **TextActor;
  vtkPolyData                **Symbol;
  vtkTransform               **Transform;
  vtkTransformPolyDataFilter **SymbolTransform;
  vtkPolyDataMapper2D        **SymbolMapper;
  vtkActor2D                 **SymbolActor;
  vtkPolyData                *BorderPolyData;
  vtkPolyDataMapper2D        *BorderMapper;
  vtkActor2D                 *BorderActor;

  // Used to control whether the stuff is recomputed
  int           LegendEntriesVisible;
  int           CachedSize[2];
  vtkTimeStamp  BuildTime;

private:
  vtkLegendBoxActor(const vtkLegendBoxActor&);  // Not implemented.
  void operator=(const vtkLegendBoxActor&);  // Not implemented.
};


#endif

