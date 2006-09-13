#include "vtkVisibleCellSelector.h"
#include "vtkObjectFactory.h"
#include "vtkIdTypeArray.h"
#include "vtkstd/set"
#include "vtkRenderer.h"
#include "vtkRenderWindow.h"
#include "vtkSelection.h"
#include "vtkInformation.h"
#include "vtkIdentColoredPainter.h"

//----------------------------------------------------------------------------
class vtkVisibleCellSelectorInternals
{
// This class represents one pixel hit record, which may span from one to five
// rendering passes. It is initialized from the color ids rendered into 1..5 
// pixel  buffer. Because 24 bits (one RGB pixel in a standard color buffer) 
// can not distinguish very many cells, three separate cell id entries are 
// provided for the low, mid, and high 24 bit fields of a large integer.
public:
  unsigned char byte[15];

  vtkVisibleCellSelectorInternals()
  {
    memset(byte,0,15);
  }

  void print()
  {
   cerr << "P " << this->GetField(0) << " ";
   cerr << " A " << this->GetField(1) << " ";
   cerr << " H " << this->GetField(2) << " ";
   cerr << " L " << this->GetField(3) << endl;
  }

  void init(unsigned char*proc, 
            unsigned char*actor, 
            unsigned char*cidH, unsigned char*cidM, unsigned char *cidL)
  {
    //initialize this hit record with the current hit information stored at
    //the pixel pointed to in five color buffers
    //null pointers are treated as hitting background (a miss).
    this->initfield(&byte[0], proc);
    this->initfield(&byte[3], actor);
    this->initfield(&byte[6], cidH);
    this->initfield(&byte[9], cidM);
    this->initfield(&byte[12], cidL);
  }

  void initfield(unsigned char *dest, unsigned char *src)
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
      (((vtkIdType)byte[0])<<16) |
      (((vtkIdType)byte[1])<< 8) |
      (((vtkIdType)byte[2])    );
    if (ret != 0) //account for the fact that a miss is stored as 0
      {
      ret --;
      }
    }
  else if (i == 1)
    {
    //actor
    ret = 
      (((vtkIdType)byte[3])<<16) |
      (((vtkIdType)byte[4])<< 8) |
      (((vtkIdType)byte[5])    );
    if (ret != 0)
      {
      ret --;
      }
    }
  else
    {
    //cell id, convert from 3 24 bit fields to two 32 bit ones
    //throwing away upper 8 bits, but accounting for miss stored in 0
    vtkIdType HField;
    HField = 
      (((vtkIdType)byte[6])<<16) |
      (((vtkIdType)byte[7])<< 8) |
      (((vtkIdType)byte[8])    );
    if (HField != 0) 
      {
      HField --;
      }
    vtkIdType MField;
    MField = 
      (((vtkIdType)byte[ 9])<<16) |
      (((vtkIdType)byte[10])<< 8) |
      (((vtkIdType)byte[11])    );
    if (MField != 0) 
      {
      MField --;
      }
    vtkIdType LField;
    LField = 
      (((vtkIdType)byte[12])<<16) |
      (((vtkIdType)byte[13])<< 8) |
      (((vtkIdType)byte[14])    );
    if (LField != 0) 
      {
      LField --;
      }
    if (i == 2)
      {
      //upper 32 bits of cell id
      ret = ((HField & 0xFFFF) << 16) | (MField & 0xFFFF00 >> 8);
      }
    else
      {
      //lower 32 bits of cell id
      ret = ((MField & 0xFF) << 24) | LField;
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
      if (byte[i] < other.byte[i]) 
        {
        return true;
        }
      if (byte[i] > other.byte[i]) 
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
      if (byte[i] != other.byte[i]) 
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
vtkCxxRevisionMacro(vtkVisibleCellSelector, "1.8");
vtkStandardNewMacro(vtkVisibleCellSelector);
vtkCxxSetObjectMacro(vtkVisibleCellSelector, Renderer, vtkRenderer);

//-----------------------------------------------------------------------------
vtkVisibleCellSelector::vtkVisibleCellSelector()
{
  this->Renderer = NULL;

  this->DoProcessor=0;
  this->DoActor=0;
  this->DoCellIdHi=0;
  this->DoCellIdMid=0;
  this->DoCellIdLo=1;
  this->ProcessorId = 0;

  this->PixBuffer[0] = NULL;
  this->PixBuffer[1] = NULL;
  this->PixBuffer[2] = NULL;
  this->PixBuffer[3] = NULL;
  this->PixBuffer[4] = NULL;
  this->SelectedIds = vtkIdTypeArray::New();
  this->SelectedIds->SetNumberOfComponents(4);
  this->SelectedIds->SetNumberOfTuples(0);

}

//-----------------------------------------------------------------------------
vtkVisibleCellSelector::~vtkVisibleCellSelector()
{
  for (int i = 0; i < 5; i++)
    {
    if (this->PixBuffer[i] != NULL)
      {
      delete[] this->PixBuffer[i];      
      this->PixBuffer[i] = NULL;
      }
    }
  this->SelectedIds->Delete();
  this->SelectedIds = NULL;
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
  int idispLL[2];
  int idispUR[2];
  idispLL[0] = (int) dispLL[0];
  idispLL[1] = (int) dispLL[1];
  idispUR[0] = (int) dispUR[0]-1;
  idispUR[1] = (int) dispUR[1]-1;

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
void vtkVisibleCellSelector::SetProcessorId(int pid)
{
  this->ProcessorId = pid + 1; //account for 0 reserved for miss
  this->SetSelectConst(this->ProcessorId);
}

//----------------------------------------------------------------------------
void vtkVisibleCellSelector::SetRenderPasses(int p, int a, int h, int m, int l)
{
  this->DoProcessor = p;
  this->DoActor = a;
  this->DoCellIdHi = h;
  this->DoCellIdMid = m;
  this->DoCellIdLo = l;
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
  rwin->SwapBuffersOff();
  
  unsigned char *buf;
  for (int i = 0; i < 5; i++)
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
  if (pass > 4) 
    {
    pass = 4;
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
  vtkstd::set<vtkVisibleCellSelectorInternals> hitrecords;
  
  unsigned int misscnt, hitcnt, skipcnt, dupcnt;
  misscnt = hitcnt = skipcnt = dupcnt = 0;
  unsigned char *proc = this->PixBuffer[0];
  unsigned char *actor = this->PixBuffer[1];
  unsigned char *cidH = this->PixBuffer[2];
  unsigned char *cidM = this->PixBuffer[3];
  unsigned char *cidL = this->PixBuffer[4];

  vtkVisibleCellSelectorInternals nhit, lhit, zero;
  int Height = this->Y1-this->Y0;
  int Width = this->X1-this->X0;
  for (int y = 0; y<=Height; y++)
    {
    for (int x = 0; x<=Width; x++)
      {
      //convert the pixel colors in the buffers into a very wide integer
      nhit.init(proc, actor, cidH, cidM, cidL);
      
      if (nhit != zero)
        {
        //we hit something besides the background
        if (nhit == lhit) 
          {
          //exploit image coherence to avoid work
          skipcnt++;
          }
        else
          {          
          if (hitrecords.find(nhit) != hitrecords.end()) 
            {
            //avoid dupl. entries
            dupcnt++;
            }
          else
            {
            //a new item behind this pixel, remember it
            //cerr << "NEW HIT @" << this->X0+x << "," << this->Y0+y << " ";
            //nhit.print();

            hitrecords.insert(nhit);
            lhit = nhit;
            hitcnt++;
            }
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
      }
    }
  //cerr << misscnt << " misses" << endl;
  //cerr << hitcnt << " hits" << endl;
  //cerr << skipcnt << " skips" << endl;
  //cerr << dupcnt << " duplicates" << endl;

  //save the hits into a vtkDataArray for external use
  this->SelectedIds->SetNumberOfTuples(hitcnt);
  if (hitcnt != 0)
    {
    //traversing the set will result in a sorted list, because vtkstd::set 
    //inserts entries using operator< to sort them for quick retrieval
    vtkIdType id = 0;
    vtkstd::set<vtkVisibleCellSelectorInternals>::iterator sit;
    for (sit = hitrecords.begin(); sit != hitrecords.end(); sit++)
      {
      vtkIdType info[4];
      info[0] = sit->GetField(0); //procid
      info[1] = sit->GetField(1); //actorid
      info[2] = sit->GetField(2); //cidH
      info[3] = sit->GetField(3); //cidL
      this->SelectedIds->SetTupleValue(id, info);
      id++;
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
  dest->Clear();
  //parent node in the tree
  dest->GetProperties()->Set(
    vtkSelection::CONTENT_TYPE(), vtkSelection::SELECTIONS);

  vtkIdType numTup = this->SelectedIds->GetNumberOfTuples();
  vtkIdType aTuple[4];
  vtkIdType lProcId = -1;
  vtkIdType lActorId = -1;

  vtkIdTypeArray *cellids = NULL;

  vtkSelection* selection = NULL;
  for (vtkIdType i = 0; i < numTup; i++)
    {
    this->SelectedIds->GetTupleValue(i, &aTuple[0]);
    if (aTuple[0] != lProcId)
      {
      //cerr << "New Processor Id: " << aTuple[0] << endl;

      if (selection != NULL)
        {
        //save whatever node we might have been filling before
        dest->AddChild(selection);
        selection->Delete();
        selection = NULL;

        //cellids and selection are created at the same time so this is OK
        cellids->Delete();
        cellids = NULL;
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
        //save whatever node we might have been filling before
        dest->AddChild(selection);
        selection->Delete();
        selection = NULL;
        
        cellids->Delete();
        cellids = NULL;
        }

      //start a new node
      selection = vtkSelection::New();
      //record that we are storing cell ids in the node
      selection->GetProperties()->Set(
        vtkSelection::CONTENT_TYPE(), vtkSelection::CELL_IDS);
      //record the processor we are recording hits on
      selection->GetProperties()->Set(
        vtkSelection::PROCESS_ID(), lProcId);
      //record the prop we have hits from
      selection->GetProperties()->Set(
        vtkSelection::PROP_ID(), aTuple[1]);
      //start a new holder for cell ids
      cellids = vtkIdTypeArray::New();
      cellids->SetNumberOfComponents(1);      
      selection->SetSelectionList(cellids);

      lActorId = aTuple[1];
      }

    //Try to use 64 bits to hold the cell id, if not possible ignore upper 32.
#if (VTK_SIZEOF_ID_TYPE == 8)
    aTuple[3] |= (aTuple[2]<<32);
#endif
    cellids->InsertNextValue(aTuple[3]);    
    }

  if (selection != NULL)
    {
    //save whatever node we might have filled before
    dest->AddChild(selection);
    selection->Delete();
    selection = NULL;

    cellids->Delete();
    cellids = NULL;
    }
}

//----------------------------------------------------------------------------
vtkProp* vtkVisibleCellSelector::GetActorFromId(vtkIdType id)
{
  if ( (this->Renderer == NULL) || 
       (id >= this->Renderer->PropsSelectedFromCount) )
    {
    return NULL;
    }

  return this->Renderer->PropsSelectedFrom[id];
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
  for (vtkIdType id = 0; id < lists->GetNumberOfTuples(); id++)
    {
    lists->GetTupleValue(id, rec);    
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
    os << indent << "PixBuffer[" << i << "]: " << (void*)PixBuffer[i] << endl;
    }

  os << indent << "SelectedIds: " << this->SelectedIds << endl;
}
