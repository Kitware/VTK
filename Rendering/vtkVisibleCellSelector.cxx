/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkVisibleCellSelector.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkVisibleCellSelector.h"

#include "vtkDataSetAttributes.h"
#include "vtkObjectFactory.h"
#include "vtkIdTypeArray.h"
#include "vtkIntArray.h"
#include <set>
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSelection.h"
#include "vtkSelectionNode.h"
#include "vtkInformation.h"
#include "vtkIdentColoredPainter.h"

//----------------------------------------------------------------------------
class vtkVisibleCellSelectorInternals
{
// This class represents one pixel hit record, which may span from one to six
// rendering passes. It is initialized from the color ids rendered into the
// various pixel buffers. Because 24 bits (one RGB pixel in a standard color 
// buffer) can not distinguish between more than 16 million cells, three 
// separate cell id entries are provided for the low, mid, and high 24 bit 
// fields of a large integer for large data sets.
public:
  unsigned char Byte[15];

  //the mutables are here so that they can be changed while this is inside 
  //a std::set
  //ie, while Byte determines a particular cell for the set, these ivars
  //are statistics of that particular cell which are allowed to change
  mutable int PixelCount; 
  mutable std::set<vtkIdType> visverts;

  vtkVisibleCellSelectorInternals()
    {
    memset(Byte,0,15);
    this->PixelCount = 0;
    }

  void Print(ostream &os = cerr)
    {
    os << "P "  << this->GetField(0) << " ";
    os << " A " << this->GetField(1) << " ";
    os << " H " << this->GetField(2) << " ";
    os << " L " << this->GetField(3) << endl;
    }

  void Init(unsigned char *proc, 
            unsigned char *actor, 
            unsigned char *cidH, unsigned char *cidM, unsigned char *cidL)
    {
    //initialize this hit record with the current hit information stored at
    //the pixel pointed to in five color buffers
    //null pointers are treated as hitting background (a miss).
    this->InitField(&Byte[0], proc);
    this->InitField(&Byte[3], actor);
    this->InitField(&Byte[6], cidH);
    this->InitField(&Byte[9], cidM);
    this->InitField(&Byte[12], cidL);
    this->PixelCount = 0;
    }

  void InitField(unsigned char *dest, unsigned char *src)
    {
    if (src)
      {
      dest[0] = src[0];
      dest[1] = src[1];
      dest[2] = src[2];
      }
    else
      {
      dest[0] = 0;
      dest[1] = 0;
      dest[2] = 0;    
      }
    }

  vtkIdType GetField(int i) const
    {
    vtkIdType ret;
    if (i == 0)
      {
      //proc
      ret = 
        (static_cast<vtkIdType>(Byte[0])<<16) |
        (static_cast<vtkIdType>(Byte[1])<< 8) |
        (static_cast<vtkIdType>(Byte[2])    );
      if (ret != 0) //account for the fact that a miss is stored as 0
        {
        ret --;
        }
      }
    else if (i == 1)
      {
      //actor
      ret = 
        (static_cast<vtkIdType>(Byte[3])<<16) |
        (static_cast<vtkIdType>(Byte[4])<< 8) |
        (static_cast<vtkIdType>(Byte[5])    );
      if (ret != 0)
        {
        ret --;
        }
      }
    else
      {
      //cell id, convert from 3 24 bit fields to two 32 bit ones
      //throwing away upper 8 bits, but accounting for miss stored in 0
      vtkIdType hField;
      hField =
        (static_cast<vtkIdType>(Byte[6])<<16) |
        (static_cast<vtkIdType>(Byte[7])<< 8) |
        (static_cast<vtkIdType>(Byte[8])    );
      if (hField != 0) 
        {
        hField --;
        }
      vtkIdType mField;
      mField =
        (static_cast<vtkIdType>(Byte[ 9])<<16) |
        (static_cast<vtkIdType>(Byte[10])<< 8) |
        (static_cast<vtkIdType>(Byte[11])    );
      if (mField != 0) 
        {
        mField --;
        }
      vtkIdType lField;
      lField =
        (static_cast<vtkIdType>(Byte[12])<<16) |
        (static_cast<vtkIdType>(Byte[13])<< 8) |
        (static_cast<vtkIdType>(Byte[14])    );
      if (lField != 0) 
        {
        lField --;
        }
      if (i == 2)
        {
        //upper 32 bits of cell id
        ret = ((hField & 0xFFFF) << 16) | (mField & 0xFFFF00 >> 8);
        }
      else
        {
        //lower 32 bits of cell id
        ret = ((mField & 0xFF) << 24) | lField;
        }
      }
    return ret;
    }

  //provide != and == operators for comparisons and < operator to allow us
  //to use a std lib set container to make a sorted, non repeating list of
  //hit records.
  bool operator<(const vtkVisibleCellSelectorInternals other) const
    {
    for (int i = 0; i < 15; i++)
      {
      if (Byte[i] < other.Byte[i]) 
        {
        return true;
        }
      if (Byte[i] > other.Byte[i]) 
        {
        return false;
        }
      }
    return false;
    }

  bool operator!=(const vtkVisibleCellSelectorInternals other) const
    {
    for (int i = 0; i < 15; i++)
      {
      if (Byte[i] != other.Byte[i]) 
        {
        return true;
        }
      } 
    return false;
    }

  bool operator==(const vtkVisibleCellSelectorInternals other) const
    {
    return !(operator!=(other));
    }
};

//////////////////////////////////////////////////////////////////////////////
vtkStandardNewMacro(vtkVisibleCellSelector);
vtkCxxSetObjectMacro(vtkVisibleCellSelector, Renderer, vtkRenderer);

//-----------------------------------------------------------------------------
vtkVisibleCellSelector::vtkVisibleCellSelector()
{
  this->Renderer = NULL;

  this->X0=0;
  this->Y0=0;
  this->X1=0;
  this->Y1=0;
  
  
  this->DoProcessor=0;
  this->DoActor=0;
  this->DoCellIdHi=0;
  this->DoCellIdMid=0;
  this->DoCellIdLo=1;
  this->DoVertices=0;

  this->ProcessorId = 0;

  this->PixBuffer[0] = NULL;
  this->PixBuffer[1] = NULL;
  this->PixBuffer[2] = NULL;
  this->PixBuffer[3] = NULL;
  this->PixBuffer[4] = NULL;
  this->PixBuffer[5] = NULL;
  this->SelectedIds = vtkIdTypeArray::New();
  this->SelectedIds->SetNumberOfComponents(4);
  this->SelectedIds->SetNumberOfTuples(0);
  this->PixelCounts = vtkIntArray::New();
  this->PixelCounts->SetNumberOfComponents(1);
  this->PixelCounts->SetNumberOfTuples(0);
  this->VertexPointers = vtkIdTypeArray::New();
  this->VertexPointers->SetNumberOfComponents(1);
  this->VertexPointers->SetNumberOfTuples(0);
  this->VertexLists = vtkIdTypeArray::New();
  this->VertexLists->SetNumberOfComponents(1);
  this->VertexLists->SetNumberOfTuples(0);
}

//-----------------------------------------------------------------------------
vtkVisibleCellSelector::~vtkVisibleCellSelector()
{
  for (int i = 0; i < 6; i++)
    {
    if (this->PixBuffer[i] != NULL)
      {
      delete[] this->PixBuffer[i];      
      this->PixBuffer[i] = NULL;
      }
    }
  this->SelectedIds->Delete();
  this->SelectedIds = NULL;
  this->PixelCounts->Delete();
  this->PixelCounts = NULL;
  this->VertexPointers->Delete();
  this->VertexPointers = NULL;
  this->VertexLists->Delete();
  this->VertexLists = NULL;

  if (this->Renderer)
    {
    this->Renderer->UnRegister(this);
    this->Renderer = NULL;
    }
}

//----------------------------------------------------------------------------
void vtkVisibleCellSelector::SetArea(unsigned int x0, unsigned int y0,
                                     unsigned int x1, unsigned int y1)
{
  if (this->Renderer == NULL)
    {
    vtkErrorMacro("vtkVisibleCellSelector must have a vtkRenderer assigned.");
    return;    
    }

  //find this renderer's viewport pixel coordinates on the renderwindow
  double dispLL[3];
  double dispUR[3];
  this->Renderer->SetViewPoint(-1,-1,0);
  this->Renderer->ViewToDisplay();
  this->Renderer->GetDisplayPoint(dispLL);
  this->Renderer->SetViewPoint(1,1,0);
  this->Renderer->ViewToDisplay();
  this->Renderer->GetDisplayPoint(dispUR);
  unsigned int idispLL[2];
  unsigned int idispUR[2];
  idispLL[0] = static_cast<unsigned int>(dispLL[0]);
  idispLL[1] = static_cast<unsigned int>(dispLL[1]);
  idispUR[0] = static_cast<unsigned int>(dispUR[0])-1;
  idispUR[1] = static_cast<unsigned int>(dispUR[1])-1;

  //crop the supplied select area to within the viewport
  if (x0 < idispLL[0])
    {
    x0 = idispLL[0];
    }
  if (x1 < idispLL[0])
    {
    x1 = idispLL[0];
    }
  if (x0 > idispUR[0])
    {
    x0 = idispUR[0];
    }
  if (x1 > idispUR[0])
    {
    x1 = idispUR[0];
    }

  if (y0 < idispLL[1])
    {
    y0 = idispLL[1];
    }
  if (y1 < idispLL[1])
    {
    y1 = idispLL[1];
    }
  if (y0 > idispUR[1])
    {
    y0 = idispUR[1];
    }
  if (y1 > idispUR[1])
    {
    y1 = idispUR[1];
    }

  //make sure we have the selection corners ordered ll to ur
  this->X0 = (x0<x1)?x0:x1;
  this->Y0 = (y0<y1)?y0:y1;
  this->X1 = (x0<x1)?x1:x0;
  this->Y1 = (y0<y1)?y1:y0;
}

//----------------------------------------------------------------------------
void vtkVisibleCellSelector::GetArea(unsigned int &x0, unsigned int &y0,
                                     unsigned int &x1, unsigned int &y1)
{ 
  x0 = this->X0;
  x1 = this->X1;
  y0 = this->Y0;
  y1 = this->Y1;
}

//----------------------------------------------------------------------------
void vtkVisibleCellSelector::SetProcessorId(unsigned int pid)
{
  this->ProcessorId = pid + 1; //account for 0 reserved for miss
  this->SetSelectConst(this->ProcessorId);
}

//----------------------------------------------------------------------------
void vtkVisibleCellSelector::SetRenderPasses(int p, 
                                             int a, 
                                             int h, int m, int l,
                                             int v)
{
  this->DoProcessor = p;
  this->DoActor = a;
  this->DoCellIdHi = h;
  this->DoCellIdMid = m;
  this->DoCellIdLo = l;
  this->DoVertices = v;
}

//-----------------------------------------------------------------------------
void vtkVisibleCellSelector::SetSelectMode(int mode)
{
  if (this->Renderer == NULL)
    {
    return;
    }
  this->Renderer->SetSelectMode(mode);
}

//-----------------------------------------------------------------------------
void vtkVisibleCellSelector::SetSelectConst(unsigned int constant)
{
  if (this->Renderer == NULL)
    {
    return;
    }
  this->Renderer->SetSelectConst(constant);
}

//----------------------------------------------------------------------------
void vtkVisibleCellSelector::Select()
{
  if (this->Renderer == NULL || this->Renderer->GetRenderWindow() == NULL)
    {
    return;    
    }

  vtkRenderWindow *rwin = this->Renderer->GetRenderWindow();
  int rgba[4];
  rwin->GetColorBufferSizes(rgba);
  if (rgba[0] < 8 || rgba[1] < 8 || rgba[2] < 8)
    {
    return;
    }

  rwin->SwapBuffersOff();
  
  unsigned char *buf;
  for (int i = 0; i < 6; i++)
    {
    if (i==0 && !this->DoProcessor)
      {
      continue;
      }
    if (i==1 && !this->DoActor)
      {
      continue;
      }
    if (i==2 && !this->DoCellIdHi)
      {
      continue;
      }
    if (i==3 && !this->DoCellIdMid)
      {
      continue;
      }
    if (i==4 && !this->DoCellIdLo)
      {
      continue;
      }
    if (i==5 && !this->DoVertices)
      {
      continue;
      }
    this->SetSelectMode(i+1);
    if (i==0)
      {
      this->SetSelectConst(this->ProcessorId);
      }
    rwin->Render();
    buf = rwin->GetRGBACharPixelData(this->X0,this->Y0,this->X1,this->Y1,0); 
    this->SavePixelBuffer(i, buf);
    }

  this->ComputeSelectedIds();
  this->SetSelectMode(0);
  rwin->SwapBuffersOn();
}

//----------------------------------------------------------------------------
void vtkVisibleCellSelector::SavePixelBuffer(int pass, unsigned char *buff)
{  
  if (pass < 0) 
    {
    pass = 0;
    }
  if (pass > 5) 
    {
    pass = 5;
    }
  
  if (this->PixBuffer[pass] != NULL)
    {
    delete[] this->PixBuffer[pass];      
    this->PixBuffer[pass] = NULL;
    }
  this->PixBuffer[pass] = buff;
}

//----------------------------------------------------------------------------
void vtkVisibleCellSelector::ComputeSelectedIds()
{
  //go over the pixels, determine the proc, actor and cell id.
  //create a sorted list of hitrecords without duplicates
  std::set<vtkVisibleCellSelectorInternals> hitrecords;
  
  unsigned int misscnt, hitcnt, dupcnt;
  misscnt = hitcnt = dupcnt = 0;
  unsigned char *proc = this->PixBuffer[0];
  unsigned char *actor = this->PixBuffer[1];
  unsigned char *cidH = this->PixBuffer[2];
  unsigned char *cidM = this->PixBuffer[3];
  unsigned char *cidL = this->PixBuffer[4];
  unsigned char *vert = this->PixBuffer[5];

  vtkVisibleCellSelectorInternals nhit, zero;
  int Height = this->Y1-this->Y0;
  int Width = this->X1-this->X0;
  for (int y = 0; y<=Height; y++)
    {
    for (int x = 0; x<=Width; x++)
      {
      //convert the pixel colors in the buffers into a very wide integer
      nhit.Init(proc, actor, cidH, cidM, cidL);
      
      if (nhit != zero)
        {
        //we hit something besides the background
        if (hitrecords.find(nhit) != hitrecords.end()) 
          {
          //avoid dupl. entries
          //increment hit count          
          (hitrecords.find(nhit)->PixelCount)++;
          dupcnt++;
          }
        else
          {
          //a new item behind this pixel, remember it
          //cerr << "NEW HIT @" << this->X0+x << "," << this->Y0+y << " ";
          //nhit.Print();
          nhit.PixelCount = 1;
          hitrecords.insert(nhit);
          hitcnt++;
          }

        //if we care about vertices, record any vert info too
        if (this->DoVertices && vert && (vert[0] || vert[1] || vert[2]))
          {          
          vtkIdType vertid = 
            ((static_cast<vtkIdType>(vert[0])<<16) |
             (static_cast<vtkIdType>(vert[1])<< 8) |
             (static_cast<vtkIdType>(vert[2])    )) - 1;
          //cerr << "found vertex " << vertid << endl;
          hitrecords.find(nhit)->visverts.insert(vertid);
          }
        }
      else        
        {
        misscnt++;
        }

      //move on to the next pixel
      proc = (proc==NULL) ? NULL : proc+4;
      actor = (actor==NULL) ? NULL : actor+4;
      cidH = (cidH==NULL) ? NULL : cidH+4;
      cidM = (cidM==NULL) ? NULL : cidM+4;
      cidL = (cidL==NULL) ? NULL : cidL+4;
      vert = (vert==NULL) ? NULL : vert+4;
      }
    }
  //cerr << misscnt << " misses" << endl;
  //cerr << hitcnt << " hits" << endl;
  //cerr << dupcnt << " duplicates" << endl;

  //save the hits into a vtkDataArray for external use
  this->SelectedIds->SetNumberOfTuples(hitcnt);
  this->PixelCounts->SetNumberOfTuples(hitcnt);

  if (this->DoVertices)
    {
    this->VertexPointers->SetNumberOfTuples(hitcnt);
    this->VertexLists->SetNumberOfTuples(0);  
    }

  if (hitcnt != 0)
    {
    //traversing the set will result in a sorted list, because std::set
    //inserts entries using operator< to sort them for quick retrieval
    vtkIdType cellid = 0;
    std::set<vtkVisibleCellSelectorInternals>::iterator sit;
    for (sit = hitrecords.begin(); sit != hitrecords.end(); sit++)
      {
      vtkIdType info[5];
      info[0] = sit->GetField(0); //procid
      info[1] = sit->GetField(1); //actorid
      info[2] = sit->GetField(2); //cidH
      info[3] = sit->GetField(3); //cidL
      this->SelectedIds->SetTupleValue(cellid, info);
      this->PixelCounts->SetValue(cellid, sit->PixelCount);

      if (this->DoVertices)
        {
        vtkIdType top = this->VertexLists->GetNumberOfTuples();
        std::set<vtkIdType>::iterator vit;
        //record each visible vertex of this cell
        if (sit->visverts.size()>0)
          {
          this->VertexLists->InsertNextValue(sit->visverts.size());
          for (vit = sit->visverts.begin(); vit != sit->visverts.end(); vit++)
            {
            this->VertexLists->InsertNextValue(*vit);            
            }        
          //record the index where this cell's vertices can be found in the 
          //vertex list, or -1 if no verts were visible
          this->VertexPointers->SetValue(cellid, top);
          }
        else
          {
          this->VertexPointers->SetValue(cellid, -1);
          }
        }

      cellid++;
      }    
    }
}

//----------------------------------------------------------------------------
void vtkVisibleCellSelector::GetSelectedIds(vtkIdTypeArray *dest)
{
  if (dest==NULL)
    {
    return;
    }

  //doesn't shallow copy exist for arrays?
  dest->SetNumberOfComponents(4);
  vtkIdType numTup = this->SelectedIds->GetNumberOfTuples();
  dest->SetNumberOfTuples(numTup);

  vtkIdType aTuple[4];
  for (vtkIdType i = 0; i < numTup; i++)
    {
    this->SelectedIds->GetTupleValue(i, &aTuple[0]);
    dest->SetTupleValue(i, aTuple);
    }
}

//----------------------------------------------------------------------------
void vtkVisibleCellSelector::GetSelectedIds(vtkSelection *dest)
{
  if (dest==NULL)
    {
    return;
    }
  dest->Initialize();

  vtkIdType numTup = this->SelectedIds->GetNumberOfTuples();
  vtkIdType aTuple[4];
  vtkIdType lProcId = -1;
  vtkIdType lActorId = -1;

  vtkIdTypeArray *cellids = NULL;
  vtkIdTypeArray *cellvertptrs = NULL;
  vtkIdTypeArray *cellvertlist = NULL;

  vtkSelectionNode* selection = NULL;

  int pixelCount = 0;
  for (vtkIdType i = 0; i < numTup; i++)
    {
    this->SelectedIds->GetTupleValue(i, &aTuple[0]);
    if (aTuple[0] != lProcId)
      {
      //cerr << "New Processor Id: " << aTuple[0] << endl;

      if (selection != NULL)
        {
        selection->GetProperties()->Set(
          vtkSelectionNode::PIXEL_COUNT(), pixelCount);
        //save whatever node we might have been filling before
        dest->AddNode(selection);
        selection->Delete();
        selection = NULL;

        //cellids and selection are created at the same time so this is OK
        cellids->Delete();
        cellids = NULL;

        if (cellvertptrs)
          {
          cellvertptrs->Delete();
          cellvertptrs = NULL;
          cellvertlist->Delete();
          cellvertlist = NULL;
          }

        }

      //remember what processor we are recording hits on
      lProcId = aTuple[0];
      //be sure to start off with a new actor
      lActorId = -1;
      }

    if (aTuple[1] != lActorId)
      {
      //cerr << "New Actor Id: " << aTuple[1] << endl;

      if (selection != NULL)
        {
        selection->GetProperties()->Set(
          vtkSelectionNode::PIXEL_COUNT(), pixelCount);
        //save whatever node we might have been filling before
        dest->AddNode(selection);
        selection->Delete();
        selection = NULL;
        
        cellids->Delete();
        cellids = NULL;

        if (cellvertptrs)
          {
          cellvertptrs->Delete();
          cellvertptrs = NULL;
          cellvertlist->Delete();
          cellvertlist = NULL;
          }
        }

      pixelCount = 0;
      //start a new node
      selection = vtkSelectionNode::New();
      //record that we are storing cell ids in the node
      selection->GetProperties()->Set(
        vtkSelectionNode::CONTENT_TYPE(), vtkSelectionNode::INDICES);
      selection->GetProperties()->Set(
        vtkSelectionNode::FIELD_TYPE(), vtkSelectionNode::CELL);
      //record the processor we are recording hits on
      selection->GetProperties()->Set(
        vtkSelectionNode::PROCESS_ID(), lProcId);
      //record the prop we have hits from
      selection->GetProperties()->Set(
        vtkSelectionNode::PROP_ID(), aTuple[1]);
      //start a new holder for cell ids
      cellids = vtkIdTypeArray::New();
      cellids->SetNumberOfComponents(1);      
      selection->SetSelectionList(cellids);

      lActorId = aTuple[1];

      if (this->DoVertices)
        {
        cellvertptrs = vtkIdTypeArray::New();
        cellvertptrs->SetName("vertptrs");
        cellvertptrs->SetNumberOfComponents(1);
        selection->GetSelectionData()->AddArray(cellvertptrs);
        cellvertlist = vtkIdTypeArray::New();
        cellvertlist->SetName("vertlist");
        cellvertlist->SetNumberOfComponents(1);
        selection->GetSelectionData()->AddArray(cellvertlist);
        selection->GetProperties()->Set(
          vtkSelectionNode::INDEXED_VERTICES(), 1);
        }
      }

    //Try to use 64 bits to hold the cell id, if not possible ignore upper 32.
#if (VTK_SIZEOF_ID_TYPE == 8)
    aTuple[3] |= (aTuple[2]<<32);
#endif
    cellids->InsertNextValue(aTuple[3]);    
    pixelCount += this->PixelCounts->GetValue(i);
    
    if (this->DoVertices)
      {
      //pull out the list of selected vertices for this cell 
      //and put them into the current vtkSelection in the selection tree
      vtkIdType ptr = this->VertexPointers->GetValue(i);
      if (ptr == -1)
        {
        cellvertptrs->InsertNextValue(-1); //no visible cells
        }
      else
        {
        //update offset to point into this selection's list
        cellvertptrs->InsertNextValue(cellvertlist->GetNumberOfTuples());
        vtkIdType npts = this->VertexLists->GetValue(ptr);
        cellvertlist->InsertNextValue(npts);
        for (vtkIdType pt = 0; pt < npts; pt++)
          {
          cellvertlist->InsertNextValue(this->VertexLists->GetValue(ptr+1+pt));
          }
        }
      }
    }

  if (selection != NULL)
    {
    selection->GetProperties()->Set(
      vtkSelectionNode::PIXEL_COUNT(), pixelCount);
    //save whatever node we might have filled before
    dest->AddNode(selection);
    selection->Delete();
    selection = NULL;

    cellids->Delete();
    cellids = NULL;

    if (cellvertptrs)
      {
      cellvertptrs->Delete();
      cellvertptrs = NULL;
      cellvertlist->Delete();
      cellvertlist = NULL;
      }
    }
}

//----------------------------------------------------------------------------
void vtkVisibleCellSelector::GetSelectedVertices(
  vtkIdTypeArray *pointers,
  vtkIdTypeArray *ids)
{
  if (pointers==NULL || ids==NULL)
    {
    return;
    }
  
  vtkIdType numTup;
  
  numTup = this->VertexPointers->GetNumberOfTuples();
  pointers->SetNumberOfComponents(1);
  pointers->SetNumberOfTuples(numTup);
  //cerr << "VERTPOINTERS:" << endl;
  for (vtkIdType i = 0; i < numTup; i++)
    {
    pointers->SetValue(i, this->VertexPointers->GetValue(i));
    //cerr << this->VertexPointers->GetValue(i) << endl;
    }

  numTup = this->VertexLists->GetNumberOfTuples();
  ids->SetNumberOfComponents(1);
  ids->SetNumberOfTuples(numTup);
  //cerr << "VERTICES:" << endl;
  for (vtkIdType i = 0; i < numTup; i++)
    {
    ids->SetValue(i, this->VertexLists->GetValue(i));
    //cerr << this->VertexLists->GetValue(i) << endl;
    }
}

//----------------------------------------------------------------------------
vtkProp* vtkVisibleCellSelector::GetActorFromId(vtkIdType id)
{
  if (!this->Renderer || !this->Renderer->IdentPainter)
    {
    return 0;
    }
  return this->Renderer->IdentPainter->GetActorFromId(id);
}

//----------------------------------------------------------------------------
void vtkVisibleCellSelector::PrintSelectedIds(vtkIdTypeArray *lists)
{
  if ((lists == NULL) || (lists->GetNumberOfComponents() != 4))
    {
    return;
    }

  if (lists->GetNumberOfTuples() == 0)
    {
    cerr << "MISS" << endl;
    return;
    }
  
  cerr << "PROC\tACTOR\t\tH L" << endl;
  vtkIdType rec[4];
  for (vtkIdType cellid = 0; cellid < lists->GetNumberOfTuples(); cellid++)
    {
    lists->GetTupleValue(cellid, rec);    
    cerr << rec[0] << '\t' << rec[1] << "\t\t" << rec[2] << ' ' << rec[3] << endl; 
    }
}

//---------------------------------------------------------------------------
void vtkVisibleCellSelector::SetIdentPainter(vtkIdentColoredPainter *ip)
{
  if (this->Renderer != NULL)
    {
    this->Renderer->SetIdentPainter(ip);
    }
}

//----------------------------------------------------------------------------
// Get the cellId, actor etc at this display position. Makes sense only
// after Select() has been called.
//
void vtkVisibleCellSelector::GetPixelSelection( 
    int displayPos[2],
    vtkIdType & procId,
    vtkIdType & cellId,
    vtkIdType & vertId,
    vtkProp  *& actorPtr )
{
  procId = vertId = cellId = -1;
  actorPtr = NULL;
  
  // Check if its within the rendered area

  if (static_cast<unsigned int>(displayPos[0]) < this->X0 ||
      static_cast<unsigned int>(displayPos[0]) > this->X1 ||
      static_cast<unsigned int>(displayPos[1]) < this->Y0 || 
      static_cast<unsigned int>(displayPos[1]) > this->Y1)
    {
    return;
    }

  // Locate the pointers in the rendered color buffers.

  int Width = this->X1-this->X0+1;
  unsigned long offset =     
    ((displayPos[1]-this->Y0) * Width + displayPos[0]-this->X0);

  unsigned char *proc  = this->PixBuffer[0];
  unsigned char *actor = this->PixBuffer[1];
  unsigned char *cidH  = this->PixBuffer[2];
  unsigned char *cidM  = this->PixBuffer[3];
  unsigned char *cidL  = this->PixBuffer[4];
  unsigned char *vert  = this->PixBuffer[5];
  
  proc  = (proc ==NULL) ? NULL : proc  +4 * offset;
  actor = (actor==NULL) ? NULL : actor +4 * offset;
  cidH  = (cidH ==NULL) ? NULL : cidH  +4 * offset;
  cidM  = (cidM ==NULL) ? NULL : cidM  +4 * offset;
  cidL  = (cidL ==NULL) ? NULL : cidL  +4 * offset;
  vert  = (vert ==NULL) ? NULL : vert  +4 * offset;

  // Now decode the info from these pointers

  vtkVisibleCellSelectorInternals nhit, zero;
  nhit.Init(proc, actor, cidH, cidM, cidL);

  if (nhit != zero)
    {

    procId    = nhit.GetField(0);                           //procid
    actorPtr  = this->GetActorFromId( nhit.GetField(1) );   //actorid

#if (VTK_SIZEOF_ID_TYPE == 8)
      cellId = (nhit.GetField(2) << 32) | nhit.GetField(3); // cellId
#else 
      cellId = nhit.GetField(3);                            // cellId
#endif

    // vertId
    if (this->DoVertices && vert && (vert[0] || vert[1] || vert[2]))
      {
        vertId = ((static_cast<vtkIdType>(vert[0])<<16) |
                  (static_cast<vtkIdType>(vert[1])<< 8) |
                  (static_cast<vtkIdType>(vert[2])    )) - 1;
      }
    }
}

//-----------------------------------------------------------------------------
void vtkVisibleCellSelector::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Renderer: " << this->Renderer << endl;
  os << indent << "X0: " << this->X0 << endl;
  os << indent << "Y0: " << this->Y0 << endl;
  os << indent << "X1: " << this->X1 << endl;
  os << indent << "Y1: " << this->Y1 << endl;
  os << indent << "DoProcessor" << this->DoProcessor << endl;
  os << indent << "DoActor" << this->DoActor << endl;
  os << indent << "DoCellIdLo" << this->DoCellIdLo << endl;
  os << indent << "DoCellIdMid" << this->DoCellIdMid << endl;
  os << indent << "DoCellIdHi" << this->DoCellIdHi << endl;
  os << indent << "ProcessorId" << this->ProcessorId << endl;

  for (int i = 0; i < 5; i++)
    {
    os << indent << "PixBuffer[" << i << "]: "
       << static_cast<void*>(PixBuffer[i]) << endl;
    }

  os << indent << "SelectedIds: " << this->SelectedIds << endl;
}
