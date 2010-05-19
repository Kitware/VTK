/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVisibleCellSelector.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkVisibleCellSelector - A helper that orchestrates color buffer
// visible cell selection. This is deprecated. Refer to vtkHardwareSelector instead.
//
// .SECTION Description
// DEPRECATED: Please refer to vtkHardwareSelector instead.
// This class can be used to determine what cells are visible within a given
// rectangle of the RenderWindow. To use it, call in order, SetRenderer(), 
// SetArea(), SetProcessorId(), SetRenderPasses(), and then Select(). Select 
// will cause the attached vtkRenderer to render in a special color mode, 
// where each cell is given it own color so that later inspection of the 
// Rendered Pixels can determine what cells are visible. In practice up to 
// five different rendering passes may occur depending on your choices in 
// SetRenderPasses. After Select(), a list of the visible cells can be 
// obtained by calling GetSelectedIds().
//
// Limitations:
// Antialiasing will break this class. If your graphics card settings force
// their use this class will return invalid results.
//
// Currently only cells from PolyDataMappers can be selected from. When 
// vtkRenderer is put into a SelectMode, it temporarily swaps in a new 
// vtkIdentColoredPainter to do the color index rendering of each cell in 
// each vtkProp that it renders. Until alternatives to vtkIdentColoredPainter
// exist that can do a similar coloration of other vtkDataSet types, only
// polygonal data can be selected. If you need to select other data types,
// consider using vtkDataSetMapper and turning on it's PassThroughCellIds 
// feature, or using vtkFrustumExtractor.
//
// Only Opaque geometry in Actors is selected from. Assemblies and LODMappers 
// are not currently supported. 
//
// During selection, visible datasets that can not be selected from are
// temporarily hidden so as not to produce invalid indices from their colors.
//
// .SECTION See Also
// vtkIdentColoredPainter
//


#ifndef __vtkVisibleCellSelector_h
#define __vtkVisibleCellSelector_h

#include "vtkObject.h"

class vtkRenderer;
class vtkIdTypeArray;
class vtkIntArray;
class vtkSelection;
class vtkProp;
class vtkIdentColoredPainter;

class VTK_RENDERING_EXPORT vtkVisibleCellSelector  : public vtkObject
{
public:
  vtkTypeMacro(vtkVisibleCellSelector, vtkObject);
  void PrintSelf(ostream& os, vtkIndent indent);
  static vtkVisibleCellSelector *New();

  // Description:
  // Call to let this know where to select within.
  virtual void SetRenderer(vtkRenderer *);

  // Description:
  // Call to set the selection area region.
  // This crops the selected area to the renderers pixel limits.
  void SetArea(unsigned int x0, unsigned int y0,
               unsigned int x1, unsigned int y1);

  // Description:
  // The caller of SetArea can use to check for cropped limits.
  void GetArea(unsigned int &x0, unsigned int &y0,
               unsigned int &x1, unsigned int &y1);

  // Description:
  // Call to let this know what processor number to render as in the processor
  // select pass. Internally this adds 1 to pid because 0 is reserved for miss.
  virtual void SetProcessorId(unsigned int pid);
  vtkGetMacro(ProcessorId, unsigned int);

  // Description:
  // Call to let this know what selection render passes to do.
  // If you have only one processor or one actor, you can leave DoProcessor 
  // and DoActor as false (the default). If you have less than 2^48 cells in 
  // any actor, you do not need the CellIdHi pass, or similarly if you have 
  // less than 2^24 cells, you do not need DoCellIdMid.
  // The DoPointId will enable another render pass for determining visible
  // vertices.
  void SetRenderPasses(int DoProcessor, int DoActor, 
                       int DoCellIdHi, int DoCellIdMid, int DoCellIdLo,
                       int DoVertexId=0);

  // Description:
  // Execute the selection algorithm.
  void Select();

  // Description:
  // After Select(), this will return the list of selected Ids.
  // The ProcessorId and Actor Id are returned in the first two components.
  // The CellId is returned in the last two components (only 64 bits total).
  void GetSelectedIds(vtkIdTypeArray *ToCopyInto);

  // Description:
  // After Select(), this will return the list of selected Ids.
  void GetSelectedIds(vtkSelection *ToCopyInto);

  // Description:
  // After Select(), (assuming DoVertexId is on), the will return arrays that
  // describe which cell vertices are visible.
  // The VertexPointers array contains one index into the VertexIds array for
  // every visible cell. Any index may be -1 in which case no vertices were 
  // visible for that cell. The VertexIds array contains a set of integers for
  // each cell that has visible vertices. The first entry in the set is
  // the number of visible vertices. The rest are visible vertex ranks. 
  // A set such at 2,0,4, means that a particular polygon's first and fifth
  // vertices were visible.
  void GetSelectedVertices(vtkIdTypeArray *VertexPointers,
                           vtkIdTypeArray *VertexIds);

  // Description:
  // After a select, this will return a pointer to the actor corresponding to
  // a particular id. This will return NULL if id is out of range.
  vtkProp* GetActorFromId(vtkIdType id);

  // Description:
  // For debugging - prints out the list of selected ids.
  void PrintSelectedIds(vtkIdTypeArray *IdsToPrint);

  // Description:
  // Get the cellId, vertexIds, actor, processor rendering the actor at a 
  // given display position. Makes sense only after Select() has been called.
  void GetPixelSelection( int displayPos[2],
                          vtkIdType & procId,
                          vtkIdType & cellId,
                          vtkIdType & vertId,
                          vtkProp  *& actorPtr );  

protected:
  vtkVisibleCellSelector();
  ~vtkVisibleCellSelector();

  // Description:
  // Give this a selected region of the render window after a selection render
  // with one of the passes defined above.
  void SavePixelBuffer(int pass, unsigned char *src);

  // Description:
  // After one or more calls to SavePixelBuffer(), this will convert the saved
  // pixel buffers into a list of Ids.
  void ComputeSelectedIds();

  // Description:
  // Simply calls this->Renderer's method of the same name.
  void SetSelectMode(int mode);

  // Simply calls this->Renderer's method of the same name.
  void SetSelectConst(unsigned int constant);

  // Description:
  void SetIdentPainter(vtkIdentColoredPainter *);

  vtkRenderer *Renderer;

  int DoProcessor;
  int DoActor;
  int DoCellIdHi;
  int DoCellIdMid;
  int DoCellIdLo;
  int DoVertices;

  unsigned int ProcessorId;

  unsigned int X0;
  unsigned int Y0;
  unsigned int X1;
  unsigned int Y1;

  //buffer for id colored pixels
  unsigned char *PixBuffer[6];

  // Description:
  // The results of the selection: processorIds, ActorIds, CellIds.
  vtkIdTypeArray *SelectedIds;
  vtkIntArray *PixelCounts;
  vtkIdTypeArray *VertexPointers;
  vtkIdTypeArray *VertexLists;

  vtkIdentColoredPainter *IdentPainter;
private:
  vtkVisibleCellSelector(const vtkVisibleCellSelector&); //Not implemented
  void operator=(const vtkVisibleCellSelector&); //Not implemented
};

#endif
