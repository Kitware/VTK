/*=========================================================================

  Program:   Visualization Toolkit
  Module:    $RCSfile: vtkReebGraph.cxx,v $

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

#include "vtkReebGraph.h"

vtkCxxRevisionMacro(vtkReebGraph, "$Revision: 0.1 $");
vtkStandardNewMacro(vtkReebGraph);

//----------------------------------------------------------------------------
void vtkReebGraph::SetLabel(vtkIdType arcId,vtkReebLabelTag Label)
{

  ResizeMainLabelTable(1);
  vtkIdType L;
  vtkReebGraphNewLabel(this,L);
  vtkReebLabel* l=vtkReebGraphGetLabel(this,L);
  l->HPrev=0;
  l->HNext=0;
  vtkReebGraphGetArc(this,arcId)->LabelId0=L;
  vtkReebGraphGetArc(this,arcId)->LabelId1=L;

  l->ArcId = arcId;
  l->label=Label;

  vtkIdType Lp=FindDwLabel(vtkReebGraphGetArc(this,arcId)->NodeId0,Label);
  vtkIdType Ln=FindUpLabel(vtkReebGraphGetArc(this,arcId)->NodeId1,Label);

  l->VPrev=Lp;
  if (Lp) vtkReebGraphGetLabel(this,Lp)->VNext=L;
  l->VNext=Ln;
  if (Ln) vtkReebGraphGetLabel(this,Ln)->VPrev=L;
}

//----------------------------------------------------------------------------
void vtkReebGraph::FastArcSimplify(vtkIdType arcId, int ArcNumber,
                                    vtkIdType* arcTable)
{

  // Remove the arc which opens the loop
  vtkIdType nodeId0 = vtkReebGraphGetArc(this, arcId)->NodeId0;
  vtkIdType nodeId1 = vtkReebGraphGetArc(this, arcId)->NodeId1;

  vtkReebGraphRemoveUpArc(this, nodeId0, arcId);
  vtkReebGraphRemoveDownArc(this, nodeId1, arcId);
  vtkReebGraphDeleteArc(this, arcId);
}

//----------------------------------------------------------------------------
vtkIdType vtkReebGraph::FindGreater(vtkIdType nodeId, vtkIdType startingNodeId,
                                    vtkReebLabelTag label)
{
  if (!vtkReebGraphGetNode(this, nodeId)->IsFinalized)
    return 0;

  //base case
  if (vtkReebGraphIsHigherThan2(this, nodeId, startingNodeId))
    return nodeId;

  //iterative case
  for (vtkIdType A=vtkReebGraphGetNode(this, nodeId)->ArcUpId;
    A;A=vtkReebGraphGetArc(this,A)->ArcDwId0)
  {
    vtkReebArc* a=vtkReebGraphGetArc(this,A);
    vtkIdType M=vtkReebGraphGetArc(this,A)->NodeId1;
    vtkReebNode*  m=vtkReebGraphGetNode(this,M);

    if (a->LabelId0 || !m->IsFinalized) //other labels or not final node
        continue;

    if (M=FindGreater(M, startingNodeId, label))
    {
      if (label) SetLabel(A, label);
      return M;
    }
  }

  return 0;
}

//----------------------------------------------------------------------------
vtkIdType vtkReebGraph::FindLess(vtkIdType nodeId, vtkIdType startingNodeId,
                                 vtkReebLabelTag label)
{
  if (!vtkReebGraphGetNode(this, nodeId)->IsFinalized)
    return 0;

  //base case
  if (vtkReebGraphIsSmaller2(this, nodeId, startingNodeId))
    return nodeId;

  //iterative case
  for (vtkIdType A=vtkReebGraphGetNode(this, nodeId)->ArcDownId;
    A;A=vtkReebGraphGetArc(this,A)->ArcDwId1)
  {
    vtkReebArc*  a=vtkReebGraphGetArc(this,A);
    vtkIdType M=vtkReebGraphGetArc(this,A)->NodeId0;
    vtkReebNode*  m=vtkReebGraphGetNode(this,M);

    if (a->LabelId0 || !m->IsFinalized) //other labels or not final node
        continue;

    if (M=FindLess(M, startingNodeId, label))
    {
      if (label) SetLabel(A, label);
      return M;
    }
  }

  return 0;
}

//----------------------------------------------------------------------------
vtkIdType vtkReebGraph::FindJoinNode(vtkIdType arcId,
                                     double startingFunctionValue,
                                     double persistenceFilter,
                                     vtkReebLabelTag label,
                                     bool onePathOnly)
{

  vtkIdType N=vtkReebGraphGetArc(this, arcId)->NodeId1;
  vtkIdType Ret,C;

  if (vtkReebGraphGetArc(this, arcId)->LabelId0
    || !vtkReebGraphGetNode(this,N)->IsFinalized)
    //other labels or not final node
    return 0;

  if (persistenceFilter && (vtkReebGraphGetNode(this,N)->Value
    - startingFunctionValue) >= persistenceFilter)
      return 0;

  if (onePathOnly
    && (vtkReebGraphGetArc(this, arcId)->ArcDwId0
    || vtkReebGraphGetArc(this, arcId)->ArcUpId0))
    return 0;

  //base case
  if (vtkReebGraphGetArc(this, arcId)->ArcDwId1
    || vtkReebGraphGetArc(this, arcId)->ArcUpId1)
  {
    if (label) SetLabel(arcId, label);
    return N;
  }

  for (C=vtkReebGraphGetNode(this,N)->ArcUpId;
    C;C=vtkReebGraphGetArc(this,C)->ArcDwId0)
  {
    Ret = FindJoinNode(C,
      startingFunctionValue, persistenceFilter, label, onePathOnly);

    if (Ret)
    {
      if (label) SetLabel(arcId, label);
      return Ret;
    }
  }

  return 0;
}

//----------------------------------------------------------------------------
vtkIdType vtkReebGraph::FindSplitNode(vtkIdType arcId,
                                      double startingFunctionValue,
                                      double persistenceFileter,
                                      vtkReebLabelTag label,
                                      bool onePathOnly)
{

  vtkIdType N=vtkReebGraphGetArc(this, arcId)->NodeId0;
  vtkIdType Ret,C;

  if (vtkReebGraphGetArc(this, arcId)->LabelId0
   || !vtkReebGraphGetNode(this,N)->IsFinalized)
    //other labels or not final node
    return 0;

  if (persistenceFileter
    && (startingFunctionValue -vtkReebGraphGetNode(this,N)->Value)
     >= persistenceFileter)
      return 0;

  if (onePathOnly
    && (vtkReebGraphGetArc(this, arcId)->ArcDwId1
    || vtkReebGraphGetArc(this, arcId)->ArcUpId1))
    return 0;

  //base case
  if (vtkReebGraphGetArc(this, arcId)->ArcDwId0
    || vtkReebGraphGetArc(this, arcId)->ArcUpId0)
  {
    if (label) SetLabel(arcId, label);
    return N;
  }

  //iterative case
  for (C=vtkReebGraphGetNode(this,N)->ArcDownId;
    C;C=vtkReebGraphGetArc(this,C)->ArcDwId1)
  {
    Ret = FindSplitNode(C, startingFunctionValue, persistenceFileter,
      label, onePathOnly);

    if (Ret)
    {
      if (label) SetLabel(arcId, label);
      return Ret;
    }
  }

  return 0;
}

//----------------------------------------------------------------------------
vtkReebGraph::vtkReebPath vtkReebGraph::FindPath(vtkIdType arcId,
                                                 double functionScale)
{
  vtkReebPath entry;
  std::priority_queue<vtkReebPath> pq;
  int size;

  vtkIdType N0=vtkReebGraphGetArc(this, arcId)->NodeId0;
  vtkIdType N1=vtkReebGraphGetArc(this, arcId)->NodeId1;

  char* Ntouch=0;
  char* Atouch=0;

  double coeff = 1.0f / (this->MaximumScalarValue - this->MinimumScalarValue);
  double f0=vtkReebGraphGetNode(this,N0)->Value;
  double f1=vtkReebGraphGetNode(this,N1)->Value;

  //the arc itself has a good persistence
  if (functionScale && (f1-f0)>= functionScale)
  {
  NOT_FOUND:
    if (Ntouch) free(Ntouch);
    if (Atouch) free(Atouch);
    vtkReebPath fake;memset(&fake,0,sizeof(vtkReebPath));
    fake.MinimumScalarValue  =-1e18; //assume infinite persistence
    fake.MaximumScalarValue  =+1e18;
    return fake;
  }

  Atouch=(char*)malloc(sizeof(char)*this->MainArcTable.Size);
  Ntouch=(char*)malloc(sizeof(char)*this->MainNodeTable.Size);
  memset(Atouch,0,sizeof(char)*this->MainArcTable.Size);
  memset(Ntouch,0,sizeof(char)*this->MainNodeTable.Size);

  Ntouch[N0]=1;

  //I don't want to use the arc given by the user
  Atouch[arcId]=1;

  entry.NodeNumber=1;
  entry.NodeTable = new vtkIdType[1];
  entry.NodeTable[0]=N0;
  entry.ArcNumber=0;
  entry.ArcTable =0;
  entry.MinimumScalarValue=entry.MaximumScalarValue =
    vtkReebGraphGetNode(this,N0)->Value;
  pq.push(entry);

  while (size=pq.size())
  {
    entry=pq.top();
    pq.pop();

    int N=entry.NodeTable[entry.NodeNumber-1];

    for (int dir=0;dir<=1;dir++)
    {
      for (int A=(!dir)?vtkReebGraphGetNode(this,N)->ArcDownId
        :vtkReebGraphGetNode(this,N)->ArcUpId; A;
             A=(!dir)?vtkReebGraphGetArc(this,A)->ArcDwId1
               :vtkReebGraphGetArc(this,A)->ArcDwId0)
      {

        int M=(!dir)?(vtkReebGraphGetArc(this,A)->NodeId0)
          :(vtkReebGraphGetArc(this,A)->NodeId1);

        if (Atouch[A]) continue;
        Atouch[A]=1;

        //already used (==there is a better path to reach M)
        if (Ntouch[M]) continue;
        Ntouch[M]=1;

        //found!!!
        if (M==N1)
        {
          //clear all the items in the priority queue
          while (pq.size())
          {
            vtkReebPath aux=pq.top();pq.pop();
            if (aux.ArcTable) delete aux.ArcTable;
            if (aux.NodeTable) delete aux.NodeTable;
          }

          if (Ntouch) free(Ntouch);
          if (Atouch) free(Atouch);


          vtkIdType* tmp=new vtkIdType[entry.NodeNumber+1];
          memcpy(tmp,entry.NodeTable,sizeof(vtkIdType)*entry.NodeNumber);
          tmp[entry.NodeNumber]=N1;
          delete [] entry.NodeTable;
          entry.NodeTable = tmp;
          entry.NodeNumber++;
          return entry;
        }

        // The loop persistence is greater than functionScale
        double value   = vtkReebGraphGetNode(this,M)->Value;
        double newminf = vtkReebGraphMin(entry.MinimumScalarValue,value);
        double newmaxf = vtkReebGraphMax(entry.MaximumScalarValue, value);

        if (functionScale && (newmaxf-newminf) >= functionScale)
          continue;

        vtkReebPath newentry;
        newentry.MinimumScalarValue = newminf;
        newentry.MaximumScalarValue = newmaxf;
        newentry.ArcNumber = entry.ArcNumber+1;
        newentry.ArcTable = new vtkIdType [newentry.ArcNumber];
        newentry.NodeNumber = entry.NodeNumber+1;
        newentry.NodeTable = new vtkIdType [newentry.NodeNumber];
        if (entry.ArcNumber)
          memcpy(newentry.ArcTable,entry.ArcTable,
            sizeof(vtkIdType)*entry.ArcNumber);
        if (entry.NodeNumber)
          memcpy(newentry.NodeTable,entry.NodeTable,
            sizeof(vtkIdType)*entry.NodeNumber);

        newentry.ArcTable[entry.ArcNumber]=A;
        newentry.NodeTable[entry.NodeNumber]=M;
        pq.push(newentry);
      }
    }

    //finished with this entry
    if (entry.ArcTable) delete entry.ArcTable;
    if (entry.NodeTable) delete entry.NodeTable;
  }

  goto NOT_FOUND;
}

//----------------------------------------------------------------------------
int vtkReebGraph::FilterLoopsByPersistence(double functionScalePercentage)
{

 double userfilter=
   functionScalePercentage
     *(this->MaximumScalarValue - this->MinimumScalarValue);

 if (!userfilter)
   return 0;

 // refresh information about ArcLoopTable
 this->FindLoops();

 int step = (int) ((double)LoopNumber/ 100);

 int NumSimplified=0;


 for (int n=0;n<this->LoopNumber;n++)
 {
   int A  =this->ArcLoopTable[n];

   if (vtkReebGraphIsArcCleared(this,A))
     continue;


   vtkIdType N0 =vtkReebGraphGetArc(this,A)->NodeId0;
   double f0=vtkReebGraphGetNode(this,N0)->Value;
   vtkIdType N1 =vtkReebGraphGetArc(this,A)->NodeId1;
   double f1=vtkReebGraphGetNode(this,N1)->Value;

   if ((f1-f0)>=userfilter)
     continue;

   vtkReebPath entry = this->FindPath(this->ArcLoopTable[n],userfilter);

   //too high for persistence
   if (!entry.NodeNumber
     || (entry.MaximumScalarValue-entry.MinimumScalarValue)>=userfilter)
     continue;


   //distribute its bucket to the loop and delete the arc
   this->FastArcSimplify(ArcLoopTable[n],entry.ArcNumber,entry.ArcTable);
   delete entry.ArcTable;
   delete entry.NodeTable;

   ++NumSimplified;
 }

 //check for regular points
 for (int N=1;N<this->MainNodeTable.Size;N++)
 {
   if (vtkReebGraphIsNodeCleared(this,N))
     continue;

   if (vtkReebGraphGetNode(this,N)->ArcDownId==0
     && vtkReebGraphGetNode(this,N)->ArcUpId==0)
   {
     vtkReebGraphDeleteNode(this,N);
   }

   else if (vtkReebGraphIsRegular(this,vtkReebGraphGetNode(this,N)))
   {
     EndVertex(N);
   }
 }

 this->RemovedLoopNumber = NumSimplified;

 return NumSimplified;
}

//----------------------------------------------------------------------------
int vtkReebGraph::FilterBranchesByPersistence(double functionScalePercentage)
{
  int N;

  static const vtkReebLabelTag RouteOld=100;
  static const vtkReebLabelTag RouteNew=200;
  int  nstack,mstack=0;
  int* stack=0;

  if (!functionScalePercentage)
    return 0;

  double userfilter =
    functionScalePercentage
      *(this->MaximumScalarValue - this->MinimumScalarValue);

  int nsimp=0;
  int cont=0;
  const int step=10000;
  bool redo;

  REDO:

  nstack=0;
  redo=false;

  for (N=1;N<this->MainNodeTable.Size;N++)
  {
    if (vtkReebGraphIsNodeCleared(this,N))
      continue;

    vtkReebNode* n=vtkReebGraphGetNode(this,N);

    //simplify atomic nodes
    if (!n->ArcDownId && !n->ArcUpId)
    {
      vtkReebGraphDeleteNode(this,N);
    }
    else if (!n->ArcDownId)
    {
      //insert into stack branches to simplify
      for (int _A_=n->ArcUpId;_A_;_A_=vtkReebGraphGetArc(this,_A_)->ArcDwId0)
      {
        vtkReebArc* _a_=vtkReebGraphGetArc(this,_A_);
        if (vtkReebGraphGetArcPersistence(this,_a_)<userfilter)
        {
          vtkReebGraphStackPush(_A_);
        }
      }
    }
    else if (!n->ArcUpId)
    {
      //insert into stack branches to simplify
      for (int _A_=n->ArcDownId;_A_;_A_=vtkReebGraphGetArc(this,_A_)->ArcDwId1)
      {
        vtkReebArc* _a_=vtkReebGraphGetArc(this,_A_);
        if (vtkReebGraphGetArcPersistence(this,_a_)<userfilter)
        {
          vtkReebGraphStackPush(_A_);
        }
      }
    }
  }

  while (vtkReebGraphStackSize())
  {
    int A=vtkReebGraphStackTop();vtkReebGraphStackPop();

    if (!--cont)
    {
      cont=step;
    }

    if (vtkReebGraphIsArcCleared(this,A))
      continue;

    cont++;

    vtkReebArc* a=vtkReebGraphGetArc(this,A);

    int N=a->NodeId0;
    int M=a->NodeId1;

    if (vtkReebGraphGetNode(this,N)->ArcDownId
      && vtkReebGraphGetNode(this,M)->ArcUpId)
      continue;

    double persistence = vtkReebGraphGetArcPersistence(this,a);

    //is the actual persistence (in percentage) greater than the applied filter?
    if (persistence>=userfilter)
      continue;

    int Mdown,Nup,Ndown,Mup;
    vtkReebGraphGetDownDegree(Mdown,this,M);
    vtkReebGraphGetUpDegree(Nup,this,N);
    vtkReebGraphGetDownDegree(Ndown,this,N);
    vtkReebGraphGetUpDegree(Mup,this,M);

    //isolated arc
    if (!Ndown && Nup==1 && Mdown==1 && !Mup)
    {
      vtkReebGraphRemoveUpArc  (this,N,A);
      vtkReebGraphRemoveDownArc(this,M,A);
      vtkReebGraphDeleteArc(this,A);

      if (!vtkReebGraphIsNodeCleared(this,N)
        && vtkReebGraphIsRegular(this,vtkReebGraphGetNode(this,N)))
      {
        EndVertex(N);
      }
      if (!vtkReebGraphIsNodeCleared(this,M)
        && vtkReebGraphIsRegular(this,vtkReebGraphGetNode(this,M)))
      {
        EndVertex(M);
      }

      nsimp++;redo=true;
      continue;
    }

    int Down,Up;

    bool simplified=false;

    // M is a maximum
    if (!simplified && !Mup)
    {
      if (Down=FindSplitNode(A,vtkReebGraphGetNode(this,M)->Value,
        userfilter,RouteOld))
      {
        if (Up=FindGreater(Down,M,RouteNew))
        {
          SetLabel(AddArc(M,Up),RouteOld);
          Collapse(Down,Up,RouteOld,RouteNew);
          simplified=true;
        }
        else
        {
          this->SimplifyLabels(Down);
        }
      }
    }

    //N is a minimum
    if (!simplified && !Ndown)
    {
      if (Up= FindJoinNode(A,vtkReebGraphGetNode(this,N)->Value,
        userfilter,RouteOld))
      {

        if (Down=FindLess(Up,N,RouteNew))
        {
          SetLabel(AddArc(Down,N),RouteOld);
          Collapse(Down,Up,RouteOld,RouteNew);
          simplified=true;
        }
        else
        {
          this->SimplifyLabels(Up);
        }
      }
    }

    if (simplified)
    {


      if (!vtkReebGraphIsNodeCleared(this,Down))
      {
        this->SimplifyLabels(Down);

        if (!vtkReebGraphGetNode(this,Down)->ArcDownId) //minimum
        {
          for (vtkIdType _A_=vtkReebGraphGetNode(this,Down)->ArcUpId;
            _A_;_A_=vtkReebGraphGetArc(this,_A_)->ArcDwId0)
          {
            vtkReebArc* _a_=vtkReebGraphGetArc(this,_A_);
            if (vtkReebGraphGetArcPersistence(this,_a_)<userfilter)
            {
              vtkReebGraphStackPush(_A_);
            }
          }
        }
      }

      if (!vtkReebGraphIsNodeCleared(this,Up))
      {
        this->SimplifyLabels(Up);

        if (!vtkReebGraphGetNode(this,Up)->ArcUpId)
        {
          for (int _A_=vtkReebGraphGetNode(this,Up)->ArcDownId;
            _A_;_A_=vtkReebGraphGetArc(this,_A_)->ArcDwId1)
          {
            vtkReebArc* _a_=vtkReebGraphGetArc(this,_A_);
            if (vtkReebGraphGetArcPersistence(this,_a_)<userfilter)
            {
              vtkReebGraphStackPush(_A_);
            }
          }
        }
      }

      nsimp++;
      redo=true;
    }

  } //while


  if (redo)
    goto REDO;

  free(stack);

  return nsimp;
}

//----------------------------------------------------------------------------
void vtkReebGraph::ResizeMainNodeTable(int newSize)
{
  int oldsize,i;

  if ((this->MainNodeTable.Size-this->MainNodeTable.Number)< newSize)
  {
    oldsize=this->MainNodeTable.Size;

    if (!this->MainNodeTable.Size) this->MainNodeTable.Size = newSize;
    while ((this->MainNodeTable.Size-this->MainNodeTable.Number)< newSize)
      this->MainNodeTable.Size<<=1;

    this->MainNodeTable.Buffer = (vtkReebNode*)realloc(
      this->MainNodeTable.Buffer,sizeof(vtkReebNode)*this->MainNodeTable.Size);

    for (i=oldsize;i<this->MainNodeTable.Size-1;i++)
    {
        vtkReebGraphGetDownArc(this,i)=i+1;
        vtkReebGraphClearNode(this,i);
    }

    vtkReebGraphGetDownArc(this,i)=this->MainNodeTable.FreeZone;
    vtkReebGraphClearNode(this,i);
    this->MainNodeTable.FreeZone=oldsize;
  }
}

//----------------------------------------------------------------------------
int vtkReebGraph::FilterByPersistence(double functionScalePercentage)
{

   this->ArcNumber = 0;
   this->NodeNumber = 0;

   return this->FilterBranchesByPersistence(functionScalePercentage)
          + this->FilterLoopsByPersistence(functionScalePercentage)
          + this->FilterBranchesByPersistence(functionScalePercentage);
}

//----------------------------------------------------------------------------
void vtkReebGraph::FlushLabels()
{
  for (int A=1;A<this->MainArcTable.Size;A++)
  {
    if (!vtkReebGraphIsArcCleared(this,A))
      vtkReebGraphGetArc(this,A)->LabelId0 =
        vtkReebGraphGetArc(this,A)->LabelId1=0;
  }

  if (this->MainLabelTable.Buffer)
  {
    free(this->MainLabelTable.Buffer);
  }

  this->MainLabelTable.Buffer=(vtkReebLabel*)malloc(sizeof(vtkReebLabel)*2);
  this->MainLabelTable.Size=2;
  this->MainLabelTable.Number=1;
  this->MainLabelTable.FreeZone=1;
  vtkReebGraphClearLabel(this,1);vtkReebGraphGetLabelArc(this,1)=0;
}

//----------------------------------------------------------------------------
void vtkReebGraph::DeepCopy(vtkReebGraph *src)
{
  memcpy(this, src, sizeof(vtkReebGraph));

  if (src->MainArcTable.Buffer)
  {
    this->MainArcTable.Buffer = (vtkReebArc*)malloc(
      sizeof(vtkReebArc)*src->MainArcTable.Size);

    memcpy(this->MainArcTable.Buffer,src->MainArcTable.Buffer,
      sizeof(vtkReebArc)*src->MainArcTable.Size);
  }

  if (src->MainNodeTable.Buffer)
  {
    this->MainNodeTable.Buffer = (vtkReebNode*)malloc(
      sizeof(vtkReebNode)*src->MainNodeTable.Size);

    memcpy(this->MainNodeTable.Buffer,src->MainNodeTable.Buffer,
      sizeof(vtkReebNode)*src->MainNodeTable.Size);
  }

  if (src->MainLabelTable.Buffer)
  {
    this->MainLabelTable.Buffer = (vtkReebLabel*)malloc(
      sizeof(vtkReebLabel)*src->MainLabelTable.Size);
    memcpy(this->MainLabelTable.Buffer,src->MainLabelTable.Buffer,
      sizeof(vtkReebLabel)*src->MainLabelTable.Size);
  }

  if (src->ArcLoopTable)
  {
    this->ArcLoopTable=(vtkIdType *)malloc(sizeof(vtkIdType)*src->LoopNumber);
    memcpy(this->ArcLoopTable,
      src->ArcLoopTable,sizeof(vtkIdType)*src->LoopNumber);
  }

	if(src->VertexMapSize){
		this->VertexMapSize = src->VertexMapSize;
		this->VertexMapAllocatedSize = src->VertexMapAllocatedSize;
		this->VertexMap = (vtkIdType *) malloc(
			sizeof(vtkIdType)*this->VertexMapAllocatedSize);
		memcpy(this->VertexMap, src->VertexMap,
			sizeof(vtkIdType)*src->VertexMapAllocatedSize);
	}

	if(src->TriangleVertexMapSize){
		this->TriangleVertexMapSize = src->TriangleVertexMapSize;
		this->TriangleVertexMapAllocatedSize = src->TriangleVertexMapAllocatedSize;
		this->TriangleVertexMap = (int *) malloc(
			sizeof(int)*this->TriangleVertexMapAllocatedSize);
		memcpy(this->TriangleVertexMap, src->TriangleVertexMap,
			sizeof(int)*src->TriangleVertexMapAllocatedSize);
	}

  vtkMutableDirectedGraph::DeepCopy(src);

}

//----------------------------------------------------------------------------
void vtkReebGraph::CloseStream()
{

  vtkIdType prevArcId = -1, arcId = 0;
  while(arcId != prevArcId)
    {
    prevArcId = arcId;
    arcId = GetPreviousArcId();
    }
  prevArcId = -1;

  // loop over the arcs and build the local adjacency map

  // vertex -> (down vertices, up vertices)
  std::map<int, std::pair<std::vector<int>, std::vector<int> > > localAdjacency;
  std::map<int, std::pair<std::vector<int>, std::vector<int> > >::iterator aIt;

  while(prevArcId != arcId)
    {
    vtkIdType downVertexId, upVertexId;
    downVertexId = vtkReebGraphGetNode(this,
      (vtkReebGraphGetArc(this, arcId))->NodeId0)->VertexId;
    upVertexId = vtkReebGraphGetNode(this,
      (vtkReebGraphGetArc(this, arcId))->NodeId1)->VertexId;

    std::map<int,
      std::pair<std::vector<int>, std::vector<int> > >::iterator aIt;

    // lookUp for the down vertex
    aIt = localAdjacency.find(downVertexId);
    if(aIt == localAdjacency.end())
      {
      std::pair<std::vector<int>, std::vector<int> > adjacencyItem;
      adjacencyItem.second.push_back(upVertexId);
      localAdjacency[downVertexId] = adjacencyItem;
      }
    else
      {
      aIt->second.second.push_back(upVertexId);
      }

    // same thing for the up vertex
    aIt = localAdjacency.find(upVertexId);
    if(aIt == localAdjacency.end())
      {
      std::pair<std::vector<int>, std::vector<int> > adjacencyItem;
      adjacencyItem.first.push_back(downVertexId);
      localAdjacency[upVertexId] = adjacencyItem;
      }
    else
      {
      aIt->second.first.push_back(downVertexId);
      }

    prevArcId = arcId;
    arcId = GetNextArcId();
    }

  // now build the super-arcs with deg-2 nodes

  // <vertex,vertex>,<vertex list> (arc, deg2 node list)
  std::vector<std::pair<std::pair<int, int>, std::vector<int> > >
    globalAdjacency;

  aIt = localAdjacency.begin();
  do
    {
    if(!((aIt->second.first.size() == 1)&&(aIt->second.second.size() == 1)))
      {
      // not a deg-2 node
      if(aIt->second.second.size())
        {
        // start the sweep up
        for(int i = 0; i < aIt->second.second.size(); i++)
          {
          std::vector<int> deg2List;
          std::map<int,
            std::pair<std::vector<int>, std::vector<int> > >::iterator nextIt;

          nextIt = localAdjacency.find(aIt->second.second[i]);
          while((nextIt->second.first.size() == 1)
            &&(nextIt->second.second.size() == 1))
            {
            deg2List.push_back(nextIt->first);
            nextIt = localAdjacency.find(nextIt->second.second[0]);
            }
            globalAdjacency.push_back(
              std::pair<std::pair<int, int>,
                std::vector<int> >(std::pair<int, int>(
                  aIt->first, nextIt->first), deg2List));
          }
        }
      }
      aIt++;
    }
  while(aIt != localAdjacency.end());

  // now cleanup the internal representation
  int nmyend=0;
  for (vtkIdType N=1;N<this->MainNodeTable.Size;N++)
  {
    if (vtkReebGraphIsNodeCleared(this,N))
      continue;

    vtkReebNode* n=vtkReebGraphGetNode(this,N);

    if (!n->IsFinalized)
    {
      nmyend++;
      EndVertex(N);
    }
  }

  this->FlushLabels();


  // now construct the actual graph
  vtkIdType prevNodeId = -1, nodeId = 0;
  while(prevNodeId != nodeId)
    {
    prevNodeId = nodeId;
    nodeId = GetPreviousNodeId();
    }
  prevNodeId = -1;

  vtkVariantArray *vertexProperties = vtkVariantArray::New();
  vertexProperties->SetNumberOfValues(1);

  vtkIdTypeArray  *vertexIds = vtkIdTypeArray::New();
  vertexIds->SetName("Vertex Ids");
  GetVertexData()->AddArray(vertexIds);

  std::map<int, int> vMap;
  int vIt = 0;

  while(prevNodeId != nodeId)
    {
    vtkIdType nodeVertexId = GetNodeVertexId(nodeId);
    vMap[nodeVertexId] = vIt;
    vertexProperties->SetValue(0, nodeVertexId);
    AddVertex(vertexProperties);

    prevNodeId = nodeId;
    nodeId = GetNextNodeId();
    vIt++;
    }
  vertexIds->Delete();
  vertexProperties->Delete();

  vtkVariantArray  *deg2NodeIds = vtkVariantArray::New();
  deg2NodeIds->SetName("Vertex Ids");
  GetEdgeData()->AddArray(deg2NodeIds);

  for(int i = 0; i < globalAdjacency.size(); i++)
    {
    std::map<int, int>::iterator downIt, upIt;
    downIt = vMap.find(globalAdjacency[i].first.first);
    upIt = vMap.find(globalAdjacency[i].first.second);

    if((downIt != vMap.end())&&(upIt != vMap.end()))
      {
      vtkVariantArray *edgeProperties = vtkVariantArray::New();
      vtkIdTypeArray  *vertexList = vtkIdTypeArray::New();
      vertexList->SetNumberOfValues(globalAdjacency[i].second.size());
      for(int j = 0; j < globalAdjacency[i].second.size(); j++)
        vertexList->SetValue(j, globalAdjacency[i].second[j]);
      edgeProperties->SetNumberOfValues(1);
      edgeProperties->SetValue(0, vertexList);
      AddEdge(downIt->second, upIt->second, edgeProperties);
      vertexList->Delete();
      edgeProperties->Delete();
      }
    }
  deg2NodeIds->Delete();
}

//----------------------------------------------------------------------------
vtkReebGraph::vtkReebGraph()
{

  this->MainNodeTable.Buffer = (vtkReebNode*)malloc(sizeof(vtkReebNode)*2);
  this->MainArcTable.Buffer = (vtkReebArc*)malloc(sizeof(vtkReebArc)*2);
  this->MainLabelTable.Buffer = (vtkReebLabel*)malloc(sizeof(vtkReebLabel)*2);

  this->MainNodeTable.Size=2;
  this->MainNodeTable.Number=1; //the item "0" is blocked
  this->MainArcTable.Size=2;
  this->MainArcTable.Number=1;
  this->MainLabelTable.Size=2;
  this->MainLabelTable.Number=1;

  this->MainNodeTable.FreeZone=1;
  vtkReebGraphClearNode(this,1);
  vtkReebGraphGetDownArc(this,1)=0;
  this->MainArcTable.FreeZone=1;
  vtkReebGraphClearArc(this,1);
  vtkReebGraphGetArcLabel(this,1)=0;
  this->MainLabelTable.FreeZone=1;
  vtkReebGraphClearLabel(this,1);
  vtkReebGraphGetLabelArc(this,1)=0;

  this->MinimumScalarValue = this->MaximumScalarValue =0;

  this->ArcNumber = 0;
  this->NodeNumber = 0;
  this->LoopNumber = 0;
  this->RemovedLoopNumber = 0;
  this->ArcLoopTable = 0;

  this->currentNodeId = 0;
  this->currentArcId = 0;

	// streaming support
	this->VertexMapSize = 0;
	this->VertexMapAllocatedSize = 0;
	this->TriangleVertexMapSize = 0;
	this->TriangleVertexMapAllocatedSize = 0;

}

//----------------------------------------------------------------------------
vtkReebGraph::~vtkReebGraph()
{

  free(this->MainNodeTable.Buffer);
  this->MainNodeTable.Buffer = NULL;

  free(this->MainArcTable.Buffer);
  this->MainArcTable.Buffer = NULL;

  free(this->MainLabelTable.Buffer);
  this->MainLabelTable.Buffer = NULL;

  this->MainNodeTable.Size = this->MainNodeTable.Number = 0;
  this->MainArcTable.Size = this->MainArcTable.Number = 0;
  this->MainLabelTable.Size = this->MainLabelTable.Number = 0;

  this->MainNodeTable.FreeZone = 0;
  this->MainArcTable.FreeZone = 0;
  this->MainLabelTable.FreeZone = 0;

  if(this->ArcLoopTable) free(this->ArcLoopTable);

	if(this->VertexMapAllocatedSize) free(this->VertexMap);

	if(this->TriangleVertexMapAllocatedSize) free(this->TriangleVertexMap);

}

//----------------------------------------------------------------------------
void vtkReebGraph::PrintSelf(ostream& os, vtkIndent indent)
{

  vtkIdType arcId = 0, nodeId = 0;

  vtkObject::PrintSelf(os, indent);
  os << indent << "Reeb graph general statistics:" << endl;
  os << indent << indent << "Number Of Node(s): "
    << this->GetNumberOfNodes() << endl;
  os << indent << indent << "Number Of Arc(s): "
    << this->GetNumberOfArcs() << endl;
  os << indent << indent << "Number Of Connected Component(s): "
    << this->GetNumberOfConnectedComponents() << endl;
  os << indent << indent << "Number Of Loop(s): "
    << this->GetNumberOfLoops() << endl;

  os << indent << "Node Data:" << endl;
  vtkIdType prevNodeId = -1;
 
  // roll back to the beginning of the list
	while(prevNodeId != nodeId){
		prevNodeId = nodeId;
		nodeId = this->GetPreviousNodeId();
	}
	prevNodeId = -1;


  while(prevNodeId != nodeId)
  {
    prevNodeId = nodeId;
    vtkIdList *downArcIdList = vtkIdList::New();
    vtkIdList *upArcIdList = vtkIdList::New();

    this->GetNodeDownArcIds(nodeId, downArcIdList);
    this->GetNodeUpArcIds(nodeId, upArcIdList);

    cout << indent << indent << "Node " << nodeId << ":" << endl;
    cout << indent << indent << indent;
    cout << "Vert: " << this->GetNodeVertexId(nodeId);
    cout << ", Val: " << this->GetNodeScalarValue(nodeId);
    cout << ", DwA:";
    for(vtkIdType i = 0; i < downArcIdList->GetNumberOfIds(); i++)
      cout << " " << this->GetArcDownNodeId(downArcIdList->GetId(i));
    cout << ", UpA:";
    for(vtkIdType i = 0; i < upArcIdList->GetNumberOfIds(); i++)
      cout << " " << this->GetArcUpNodeId(upArcIdList->GetId(i));
    cout << endl;
      
    downArcIdList->Delete();
    upArcIdList->Delete();
    nodeId = this->GetNextNodeId();
  }

  os << indent << "Arc Data:" << endl;
  vtkIdType prevArcId = -1;
	arcId = 0;

	// roll back to the beginning of the list
	while(prevArcId != arcId){
		prevArcId = arcId;
		arcId = this->GetPreviousArcId();
	}
	prevArcId = -1;

  while(prevArcId != arcId)
  {
    prevArcId = arcId;
    cout << indent << indent << "Arc " << arcId << ":" << endl;
    cout << indent << indent << indent;
    cout << "Down: " << this->GetArcDownNodeId(arcId);
    cout << ", Up: " << this->GetArcUpNodeId(arcId);
    cout << ", Persistence: " 
      << this->GetNodeScalarValue(this->GetArcUpNodeId(arcId))
        - this->GetNodeScalarValue(this->GetArcDownNodeId(arcId));
    cout << endl;
    arcId = this->GetNextArcId();
  }
}

//----------------------------------------------------------------------------
void vtkReebGraph::GetNodeDownArcIds(vtkIdType nodeId, vtkIdList *arcIdList)
{

  vtkIdType i  = 0;

  if(!arcIdList) return;

  arcIdList->Reset();

  for(vtkIdType arcId = vtkReebGraphGetNode(this, nodeId)->ArcDownId;
    arcId; arcId = vtkReebGraphGetArc(this, arcId)->ArcDwId1)
  {
      arcIdList->InsertId(i, arcId);
      i++;
  }
}

//----------------------------------------------------------------------------
void vtkReebGraph::GetNodeUpArcIds(vtkIdType nodeId, vtkIdList *arcIdList)
{

  vtkIdType i  = 0;

  if(!arcIdList) return;

  for(vtkIdType arcId = vtkReebGraphGetNode(this, nodeId)->ArcUpId;
    arcId; arcId = vtkReebGraphGetArc(this, arcId)->ArcDwId0)
  {
    arcIdList->InsertId(i, arcId);
    i++;
  }
}

//----------------------------------------------------------------------------
void vtkReebGraph::FindLoops()
{
  if (this->ArcLoopTable)
  {
    free(this->ArcLoopTable);
    this->ArcLoopTable=0;
    this->LoopNumber=0;
  }

  this->ConnectedComponentNumber=0;

  int  nstack=0,mstack=0;
  int* stack=0;

  char* Ntouch=(char*)malloc(sizeof(char)*this->MainNodeTable.Size);
  char* Atouch=(char*)malloc(sizeof(char)*this->MainArcTable.Size);

  memset(Ntouch,0,sizeof(char)*this->MainNodeTable.Size);

  for(int Node=1;Node<this->MainNodeTable.Size;Node++)
  {
    if (vtkReebGraphIsNodeCleared(this,Node))
      continue;

    if (!Ntouch[Node])
    {
      int LoopNumber=0;
      ++(this->ConnectedComponentNumber);

      memset(Atouch,0,sizeof(bool)*this->MainArcTable.Size);

      Ntouch[Node]=1;
      nstack=0;
      vtkReebGraphStackPush(Node);

      while (vtkReebGraphStackSize())
      {

        int N=vtkReebGraphStackTop();
        vtkReebGraphStackPop();

        for (int dir=0;dir<=1;dir++)
        {
          for (int A=(!dir)?(vtkReebGraphGetNode(this,N)->ArcDownId)
            :(vtkReebGraphGetNode(this,N)->ArcUpId); A;
              A=(!dir)?(vtkReebGraphGetArc(this,A)->ArcDwId1)
                :(vtkReebGraphGetArc(this,A)->ArcDwId0))
          {
            int M=(!dir)?(vtkReebGraphGetArc(this,A)->NodeId0)
              :(vtkReebGraphGetArc(this,A)->NodeId1);

            if (Atouch[A])
              continue;

            if (!Ntouch[M])
            {
              vtkReebGraphStackPush(M);
            }
            else
            {
              this->LoopNumber++;
              this->ArcLoopTable =
                (vtkIdType *) realloc(this->ArcLoopTable,
                  sizeof(vtkIdType)*this->LoopNumber);
              this->ArcLoopTable[this->LoopNumber-1]=A;
            }

            Atouch[A]=1;
            Ntouch[M]=1;
          }
        }
      }
    }
  }

  free(stack);
  free(Ntouch);
  free(Atouch);

}

//----------------------------------------------------------------------------
vtkIdType vtkReebGraph::AddMeshVertex(vtkIdType vertexId, double scalar){

  vtkIdType N0;
  ResizeMainNodeTable(1);
  vtkReebGraphNewNode(this,N0);
  vtkReebNode* node=vtkReebGraphGetNode(this,N0);
  node->VertexId=vertexId;
  node->Value = scalar;
  node->ArcDownId=0;
  node->ArcUpId=0;
  node->IsFinalized = false;

  if((!this->MaximumScalarValue) || (node->Value > this->MaximumScalarValue))
    this->MaximumScalarValue = node->Value;
  if((!this->MinimumScalarValue) || (node->Value < this->MinimumScalarValue))
    this->MinimumScalarValue = node->Value;

  return N0;
}

//----------------------------------------------------------------------------
vtkIdType vtkReebGraph::FindDwLabel(vtkIdType nodeId, vtkReebLabelTag label)
{
  for (vtkIdType arcId = vtkReebGraphGetNode(this, nodeId)->ArcDownId;
    arcId; arcId = vtkReebGraphGetArc(this, arcId)->ArcDwId1)
  {
    for (vtkIdType labelId = vtkReebGraphGetArc(this, arcId)->LabelId0;
      labelId; labelId = vtkReebGraphGetLabel(this, labelId)->HNext)
    {
      if (vtkReebGraphGetLabel(this, labelId)->label== label)
        return labelId;
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
vtkIdType vtkReebGraph::FindUpLabel(vtkIdType nodeId, vtkReebLabelTag label)
{
  for (vtkIdType arcId=vtkReebGraphGetNode(this,nodeId)->ArcUpId;
    arcId; arcId = vtkReebGraphGetArc(this, arcId)->ArcDwId0)
  {
    for (vtkIdType labelId=vtkReebGraphGetArc(this,arcId)->LabelId0;
      labelId; labelId=vtkReebGraphGetLabel(this, labelId)->HNext)
    {
      if (vtkReebGraphGetLabel(this, labelId)->label==label)
        return labelId;
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
void vtkReebGraph::ResizeMainArcTable(int newSize)
{
  int oldsize,i;
  if ((this->MainArcTable.Size-this->MainArcTable.Number)< newSize)
  {
    oldsize=this->MainArcTable.Size;
    if (!this->MainArcTable.Size) this->MainArcTable.Size= newSize;
    while ((this->MainArcTable.Size-this->MainArcTable.Number)< newSize)
      this->MainArcTable.Size<<=1;

    this->MainArcTable.Buffer =
      (vtkReebArc*)realloc(this->MainArcTable.Buffer,
        sizeof(vtkReebArc)*this->MainArcTable.Size);
    for (i=oldsize;i<this->MainArcTable.Size-1;i++)
    {
      vtkReebGraphGetArcLabel(this,i)=i+1;
      vtkReebGraphClearArc(this,i);
    }

    vtkReebGraphGetArcLabel(this,i)=this->MainArcTable.FreeZone;
    vtkReebGraphClearArc(this,i);
    this->MainArcTable.FreeZone=oldsize;
  }
}

//----------------------------------------------------------------------------
void vtkReebGraph::ResizeMainLabelTable(int newSize)
{
  int oldsize,i;
  if ((this->MainLabelTable.Size-this->MainLabelTable.Number)< newSize)
  {
    oldsize=this->MainLabelTable.Size;
    if (!this->MainLabelTable.Size) this->MainLabelTable.Size = newSize;
    while ((this->MainLabelTable.Size-this->MainLabelTable.Number)< newSize)
      this->MainLabelTable.Size<<=1;

    this->MainLabelTable.Buffer =
      (vtkReebLabel*)realloc(this->MainLabelTable.Buffer,
        sizeof(vtkReebLabel)*this->MainLabelTable.Size);

    for (i=oldsize;i<this->MainLabelTable.Size-1;i++)
    {
      vtkReebGraphGetLabelArc(this,i)=i+1;
      vtkReebGraphClearLabel(this,i);
    }

    vtkReebGraphGetLabelArc(this,i)=this->MainLabelTable.FreeZone;
    vtkReebGraphClearLabel(this,i);
    this->MainLabelTable.FreeZone=oldsize;
  }
}


//----------------------------------------------------------------------------
vtkIdType vtkReebGraph::AddPath(int nodeNumber, vtkIdType* nodeOffset,
                                vtkReebLabelTag label)
{

  vtkIdType i, Lprev, Ret=0;

  this->ResizeMainArcTable(nodeNumber - 1);

  if (label)
    ResizeMainLabelTable(nodeNumber - 1);

  Lprev=0;
  for (i = 0; i < (nodeNumber - 1);i++)
  {
    vtkIdType N0 = nodeOffset[i ];
    vtkReebNode* n0=vtkReebGraphGetNode(this,N0);
    vtkIdType N1= nodeOffset[i + 1];
    vtkReebNode* n1=vtkReebGraphGetNode(this,N1);

    int A;vtkReebGraphNewArc(this,A);
    vtkReebArc* arc =vtkReebGraphGetArc(this,A);
    int L=0;

    if (!Ret) Ret=A;

    if (label)
    {
      vtkReebLabel* temporaryLabel;
      vtkReebGraphNewLabel(this,L);
      temporaryLabel = vtkReebGraphGetLabel(this,L);
      temporaryLabel->ArcId = A;
      temporaryLabel->label=label;
      temporaryLabel->VPrev=Lprev;
    }

    arc->NodeId0=N0;
    arc->NodeId1=N1;
    arc->LabelId0=arc->LabelId1=L;

    vtkReebGraphAddUpArc(this,N0,A);
    vtkReebGraphAddDownArc(this,N1,A);

    if (label)
    {
      if (Lprev) vtkReebGraphGetLabel(this,Lprev)->VNext=L;
      Lprev=L;
    }
  }

  return Ret;
}


//----------------------------------------------------------------------------
void vtkReebGraph::Collapse(vtkIdType startingNode, vtkIdType endingNode,
                            vtkReebLabelTag startingLabel,
                            vtkReebLabelTag endingLabel)
{

  int L0,L0n,L1,L1n;
  int cont[3]={0,0,0},Case;

  vtkReebGraphIsNodeCleared(this, startingNode);
  vtkReebGraphIsNodeCleared(this, startingNode);

  if (startingNode == endingNode)
    return;

  vtkReebNode* nstart=vtkReebGraphGetNode(this,startingNode);
  vtkReebNode* nend  =vtkReebGraphGetNode(this,endingNode);

  vtkReebGraphIsSmaller(this,startingNode,endingNode,nstart,nend);

  if (!vtkReebGraphIsSmaller(this,startingNode,endingNode,nstart,nend))
  {
		vtkReebGraphSwapVars(int,startingNode,endingNode);
    vtkReebGraphSwapVars(vtkReebNode* ,nstart,nend);
  }

  L0 = FindUpLabel(startingNode, startingLabel);
  L1 = FindUpLabel(startingNode, endingLabel);

  while (1)
  {
    int A0=vtkReebGraphGetLabel(this,L0)->ArcId;
    vtkReebArc* a0=vtkReebGraphGetArc(this,A0);
    int A1=vtkReebGraphGetLabel(this,L1)->ArcId;
    vtkReebArc* a1=vtkReebGraphGetArc(this,A1);

    /* it is the same arc, no semplification is done */
    if (A0==A1)
    {
      Case=0;
      L0n=vtkReebGraphGetLabel(this,L0)->VNext;
      L1n=vtkReebGraphGetLabel(this,L1)->VNext;
    }
    /* there are two arcs connecting the same start-end node */
    else if (A0!=A1 && a0->NodeId1==a1->NodeId1)
    {
      Case=1;

      vtkReebGraphRemoveUpArc(this,a0->NodeId0,A1);
      vtkReebGraphRemoveDownArc(this,a0->NodeId1,A1);

      // move labels from A1 to A0
      vtkReebGraphGetArc(this,A1)->LabelId0;
      vtkReebGraphGetArc(this,A0)->LabelId0;

      for (int Lcur=vtkReebGraphGetArc(this,A1)->LabelId0;
        Lcur;Lcur=vtkReebGraphGetLabel(this,Lcur)->HNext)
        vtkReebGraphGetLabel(this,Lcur)->ArcId=A0;

      vtkReebGraphGetLabel(this,vtkReebGraphGetArc(this,A1)->LabelId0)->HPrev
        = vtkReebGraphGetArc(this,A0)->LabelId1;

      vtkReebGraphGetLabel(this,vtkReebGraphGetArc(this,A0)->LabelId1)->HNext
        = vtkReebGraphGetArc(this,A1)->LabelId0;

      vtkReebGraphGetArc(this,A0)->LabelId1
        = vtkReebGraphGetArc(this,A1)->LabelId1;

      vtkReebGraphGetArc(this,A1)->LabelId0=0;
      vtkReebGraphGetArc(this,A1)->LabelId1=0;
      vtkReebGraphDeleteArc(this,A1);

      L0n=vtkReebGraphGetLabel(this,L0)->VNext;
      L1n=vtkReebGraphGetLabel(this,L1)->VNext;
    }
    else
    {
      // a more complicate situation, collapse reaching the less ending point of
      // the arcs.
      Case=2;
      {
        vtkReebNode* a0n1=vtkReebGraphGetNode(this,a0->NodeId1);
        vtkReebNode* a1n1=vtkReebGraphGetNode(this,a1->NodeId1);
        if (!vtkReebGraphIsSmaller(this,a0->NodeId1,a1->NodeId1,a0n1,a1n1))
        {
          vtkReebGraphSwapVars(int,A0,A1);
          vtkReebGraphSwapVars(int,L0,L1);
          vtkReebGraphSwapVars(vtkReebArc* ,a0,a1);
        }
      }

      vtkReebGraphRemoveUpArc(this,a0->NodeId0,A1);
      a1->NodeId0=a0->NodeId1;
      vtkReebGraphAddUpArc(this,a0->NodeId1,A1);

      //"replicate" labels from A1 to A0
      for (int Lcur=vtkReebGraphGetArc(this,A1)->LabelId0;
        Lcur;Lcur=vtkReebGraphGetLabel(this,Lcur)->HNext)
      {
        int Lnew;
        ResizeMainLabelTable(1);
        vtkReebGraphNewLabel(this,Lnew);
        vtkReebLabel* lnew=vtkReebGraphGetLabel(this,Lnew);
        vtkReebLabel* lcur=vtkReebGraphGetLabel(this,Lcur);
        lnew->ArcId = A0;
        lnew->VPrev = lcur->VPrev;

        if (lcur->VPrev)
          vtkReebGraphGetLabel(this,lcur->VPrev)->VNext=Lnew;

        lcur->VPrev = Lnew;
        lnew->VNext = Lcur;
        lnew->label = lcur->label;

        lnew->HNext = 0;
        lnew->HPrev = vtkReebGraphGetArc(this,A0)->LabelId1;
        vtkReebGraphGetLabel(this,vtkReebGraphGetArc(this,A0)->LabelId1)->HNext
          =Lnew;

        vtkReebGraphGetArc(this,A0)->LabelId1=Lnew;
      }

      L0n=vtkReebGraphGetLabel(this,L0)->VNext;
      L1n=L1;
    }

    ++cont[Case];

    int N0=a0->NodeId0;
    vtkReebNode* n0=vtkReebGraphGetNode(this,N0);

    if (n0->IsFinalized && vtkReebGraphIsRegular(this,n0))
    {
      vtkReebGraphVertexCollapse(this,N0,n0);
    }

    /* end condition */
    if (a0->NodeId1==endingNode)
    {
      vtkReebNode* nend=vtkReebGraphGetNode(this,endingNode);

      if (nend->IsFinalized && vtkReebGraphIsRegular(this,nend))
      {
        vtkReebGraphVertexCollapse(this,endingNode,nend);
      }

      return;
    }

    L0=L0n;
    L1=L1n;

  }

}

void vtkReebGraph::SimplifyLabels(const vtkIdType nodeId,
                                  vtkReebLabelTag onlyLabel,
                                  bool goDown, bool goUp)
{
  static int nactivation=0;
  ++nactivation;

  int A,L,Lprev,Lnext;
  vtkReebLabel *l;
  vtkReebNode *n=vtkReebGraphGetNode(this, nodeId);

  //I remove all Labels (paths) which start from me
  if (goDown)
  {
    int Anext;
    for (A=n->ArcDownId;A;A=Anext)
    {
      Anext=vtkReebGraphGetArc(this,A)->ArcDwId1;
      for (L=vtkReebGraphGetArc(this,A)->LabelId0;L;L=Lnext)
      {
        Lnext=vtkReebGraphGetLabel(this,L)->HNext;

        if (!(l=vtkReebGraphGetLabel(this,L))->VNext)  //...starts from me!
        {
          if (!onlyLabel || onlyLabel==vtkReebGraphGetLabel(this,L)->label)
          {
            int Lprev;
            for (int Lcur=L;Lcur;Lcur=Lprev)
            {
              vtkReebLabel* lcur=vtkReebGraphGetLabel(this,Lcur);
              Lprev=lcur->VPrev;
              int CurA=lcur->ArcId;
              if (lcur->HPrev)
                vtkReebGraphGetLabel(this,lcur->HPrev)->HNext=lcur->HNext;
              else vtkReebGraphGetArc(this,CurA)->LabelId0=lcur->HNext;
              if (lcur->HNext)
                vtkReebGraphGetLabel(this,lcur->HNext)->HPrev=lcur->HPrev;
              else vtkReebGraphGetArc(this,CurA)->LabelId1=lcur->HPrev;
              vtkReebGraphDeleteLabel(this,Lcur);

            }
          }
        }
      }
    }
  }

  // Remove all Labels (paths) which start from here

  if (goUp && !vtkReebGraphIsNodeCleared(this, nodeId))
  {
    int Anext;
    for (A=n->ArcUpId;A;A=Anext)
    {
      Anext=vtkReebGraphGetArc(this,A)->ArcDwId0;
      for (L=vtkReebGraphGetArc(this,A)->LabelId0;L;L=Lnext)
      {
        Lnext=vtkReebGraphGetLabel(this,L)->HNext;

        if (!(l=vtkReebGraphGetLabel(this,L))->VPrev)  //...starts from me!
        {
          if (!onlyLabel || onlyLabel==vtkReebGraphGetLabel(this,L)->label)
          {
            int Lnext;
            for (int Lcur=L;Lcur;Lcur=Lnext)
            {
              vtkReebLabel* lcur=vtkReebGraphGetLabel(this,Lcur);
              Lnext=lcur->VNext;
              int CurA=lcur->ArcId;
              vtkReebArc* cura=vtkReebGraphGetArc(this,CurA);
              if (lcur->HPrev)
                vtkReebGraphGetLabel(this,lcur->HPrev)->HNext=lcur->HNext;
              else cura->LabelId0=lcur->HNext;
              if (lcur->HNext)
                vtkReebGraphGetLabel(this,lcur->HNext)->HPrev=lcur->HPrev;
              else cura->LabelId1=lcur->HPrev;
              vtkReebGraphDeleteLabel(this,Lcur);
            }
          }
        }
      }
    }
  }
}

//----------------------------------------------------------------------------
void vtkReebGraph::EndVertex(const vtkIdType N)
{

  vtkReebNode  *n=vtkReebGraphGetNode(this,N);

  n->IsFinalized = true;

  if (!vtkReebGraphIsNodeCleared(this,N))
  {
    this->SimplifyLabels(N);

    if (!vtkReebGraphIsNodeCleared(this,N))
    {
      //special case for regular point. A node is regular if it has one
      //arc down and one arc up. In this case it can disappear

      if (vtkReebGraphIsRegular(this,n))
      {
        vtkReebGraphVertexCollapse(this,N,n);
      }
    }
  }

}

//----------------------------------------------------------------------------
int vtkReebGraph::AddMeshTetrahedron(vtkIdType vertex0Id, double f0,
                                         vtkIdType vertex1Id, double f1,
                                         vtkIdType vertex2Id, double f2,
                                         vtkIdType vertex3Id, double f3)
{

  vtkIdType vertex0, vertex1, vertex2, vertex3;

  vertex0 = this->VertexStream[vertex0Id];
  vertex1 = this->VertexStream[vertex1Id];
  vertex2 = this->VertexStream[vertex2Id];
  vertex3 = this->VertexStream[vertex3Id];

  int N0 = this->VertexMap[vertex0];
  int N1 = this->VertexMap[vertex1];
  int N2 = this->VertexMap[vertex2];
  int N3 = this->VertexMap[vertex3];

  // Consistency less check
  if (f3 < f2 || (f3==f2 && vertex3 < vertex2))
  {
    vtkReebGraphSwapVars(int,vertex2,vertex3);
    vtkReebGraphSwapVars(int,N2,N3);
    vtkReebGraphSwapVars(double,f2,f3);
  }
  if (f2 < f1 || (f2==f1 && vertex2 < vertex1))
	{
    vtkReebGraphSwapVars(int,vertex1,vertex2);
    vtkReebGraphSwapVars(int,N1,N2);
    vtkReebGraphSwapVars(double,f1,f2);
  }
  if (f1 < f0 || (f1==f0 && vertex1 < vertex0))
  {
    vtkReebGraphSwapVars(int,vertex0,vertex1);
    vtkReebGraphSwapVars(int,N0,N1);
    vtkReebGraphSwapVars(double,f0,f1);
  }
  if (f3 < f2 || (f3==f2 && vertex3 < vertex2))
  {
    vtkReebGraphSwapVars(int,vertex2,vertex3);
    vtkReebGraphSwapVars(int,N2,N3);
    vtkReebGraphSwapVars(double,f2,f3);
  }
  if (f2 < f1 || (f2==f1 && vertex2 < vertex1))
  {
    vtkReebGraphSwapVars(int,vertex1,vertex2);
    vtkReebGraphSwapVars(int,N1,N2);
    vtkReebGraphSwapVars(double,f1,f2);
  }
	if (f3 < f2 || (f3==f2 && vertex3 < vertex2))
  {
    vtkReebGraphSwapVars(int,vertex2,vertex3);
    vtkReebGraphSwapVars(int,N2,N3);
    vtkReebGraphSwapVars(double,f2,f3);
  }

	vtkIdType t0[]={vertex0, vertex1, vertex2},
            t1[]={vertex0, vertex1, vertex3},
            t2[]={vertex0, vertex2, vertex3},
            t3[]={vertex1, vertex2, vertex3};
  vtkIdType *cellIds[4];
  cellIds[0] = t0;
  cellIds[1] = t1;
  cellIds[2] = t2;
  cellIds[3] = t3;

	for(int i=0; i < 3; i++)
	{

    int n0 = this->VertexMap[cellIds[i][0]],
        n1 = this->VertexMap[cellIds[i][1]],
        n2 = this->VertexMap[cellIds[i][2]];

    vtkReebLabelTag Label01 =
      ((vtkReebLabelTag) cellIds[i][0])
      | (((vtkReebLabelTag) cellIds[i][1])<<32);
    vtkReebLabelTag Label12 =
      ((vtkReebLabelTag) cellIds[i][1])
      | (((vtkReebLabelTag) cellIds[i][2])<<32);
    vtkReebLabelTag Label02 =
      ((vtkReebLabelTag) cellIds[i][0])
      | (((vtkReebLabelTag) cellIds[i][2])<<32);

    if (!this->FindUpLabel(n0,Label01))
    {
      vtkIdType N01[] = {n0, n1};
      this->AddPath(2, N01, Label01);
    }
    if (!this->FindUpLabel(n1,Label12))
    {
      vtkIdType N12[] = {n1,n2};
      this->AddPath(2, N12, Label12);
    }
    if (!this->FindUpLabel(n0, Label02))
    {
      vtkIdType N02[] = {n0,n2};
      this->AddPath(2, N02, Label02);
    }

    this->Collapse(n0,n1,Label01,Label02);
	this->Collapse(n1,n2,Label12,Label02);
  }

  if (!(--(this->TriangleVertexMap[vertex0])))
    this->EndVertex(N0);
  if (!(--(this->TriangleVertexMap[vertex1])))
    this->EndVertex(N1);
  if (!(--(this->TriangleVertexMap[vertex2])))
    this->EndVertex(N2);
	if (!(--(this->TriangleVertexMap[vertex3])))
    this->EndVertex(N3);

  return 1;
}

//----------------------------------------------------------------------------
int vtkReebGraph::AddMeshTriangle(vtkIdType vertex0Id, double f0,
																			vtkIdType vertex1Id, double f1,
																			vtkIdType vertex2Id, double f2){

	int vertex0 = this->VertexStream[vertex0Id],
      vertex1 = this->VertexStream[vertex1Id],
      vertex2 = this->VertexStream[vertex2Id];

	int N0 = this->VertexMap[vertex0];
  int N1 = this->VertexMap[vertex1];
  int N2 = this->VertexMap[vertex2];

  // Consistency less check
  if (f2 < f1 || (f2==f1 && vertex2 < vertex1))
	{
    vtkReebGraphSwapVars(int,vertex1,vertex2);
    vtkReebGraphSwapVars(int,N1,N2);
    vtkReebGraphSwapVars(double,f1,f2);
  }
  if (f1 < f0 || (f1==f0 && vertex1 < vertex0))
  {
    vtkReebGraphSwapVars(int,vertex0,vertex1);
    vtkReebGraphSwapVars(int,N0,N1);
    vtkReebGraphSwapVars(double,f0,f1);
  }
  if (f2 < f1 || (f2==f1 && vertex2 < vertex1))
  {
    vtkReebGraphSwapVars(int,vertex1,vertex2);
    vtkReebGraphSwapVars(int,N1,N2);
    vtkReebGraphSwapVars(double,f1,f2);
  }

  vtkReebLabelTag Label01 =
    ((vtkReebLabelTag)vertex0) | (((vtkReebLabelTag)vertex1)<<32);
  vtkReebLabelTag Label12 =
    ((vtkReebLabelTag)vertex1) | (((vtkReebLabelTag)vertex2)<<32);
  vtkReebLabelTag Label02 =
    ((vtkReebLabelTag)vertex0) | (((vtkReebLabelTag)vertex2)<<32);

  if (!this->FindUpLabel(N0,Label01))
  {
    vtkIdType N01[] = {N0, N1};
    this->AddPath(2, N01, Label01);
  }
  if (!this->FindUpLabel(N1,Label12))
  {
    vtkIdType N12[] = {N1,N2};
    this->AddPath(2, N12, Label12);
  }
  if (!this->FindUpLabel(N0, Label02))
  {
    vtkIdType N02[] = {N0,N2};
    this->AddPath(2, N02, Label02);
  }

  this->Collapse(N0,N1,Label01,Label02);
	this->Collapse(N1,N2,Label12,Label02);

  if (!(--(this->TriangleVertexMap[vertex0])))
    this->EndVertex(N0);
  if (!(--(this->TriangleVertexMap[vertex1])))
    this->EndVertex(N1);
  if (!(--(this->TriangleVertexMap[vertex2])))
    this->EndVertex(N2);


	return 1;
}

//----------------------------------------------------------------------------
int vtkReebGraph::StreamTetrahedron( vtkIdType vertex0Id, double scalar0,
																	   vtkIdType vertex1Id, double scalar1,
																	   vtkIdType vertex2Id, double scalar2,
                                     vtkIdType vertex3Id, double scalar3){

	if(!this->VertexMapAllocatedSize){
		// first allocate an arbitrary size
		this->VertexMapAllocatedSize = vtkReebGraphInitialStreamSize;
		this->VertexMap = (vtkIdType *) malloc(
			sizeof(vtkIdType)*this->VertexMapAllocatedSize);
		memset(this->VertexMap, 0, sizeof(vtkIdType)*this->VertexMapAllocatedSize);
    this->VertexStream.clear();
	}
	else if(this->VertexMapSize >= this->VertexMapAllocatedSize - 4){
		int oldSize = this->VertexMapAllocatedSize;
		this->VertexMapAllocatedSize <<= 1;
		this->VertexMap = (vtkIdType *) realloc(this->VertexMap,
			sizeof(vtkIdType)*this->VertexMapAllocatedSize);
		for(int i = oldSize; i < this->VertexMapAllocatedSize - 1; i++)
			this->VertexMap[i] = 0;
	}

	// same thing with the triangle map
	if(!this->TriangleVertexMapAllocatedSize){
		// first allocate an arbitrary size
		this->TriangleVertexMapAllocatedSize = vtkReebGraphInitialStreamSize;
		this->TriangleVertexMap = (int *) malloc(
			sizeof(int)*this->TriangleVertexMapAllocatedSize);
		memset(this->TriangleVertexMap, 0,
			sizeof(int)*this->TriangleVertexMapAllocatedSize);
	}
	else if(this->TriangleVertexMapSize >=
		this->TriangleVertexMapAllocatedSize - 4){

			int oldSize = this->TriangleVertexMapAllocatedSize;
			this->TriangleVertexMapAllocatedSize <<= 1;
			this->TriangleVertexMap = (int *) realloc(this->TriangleVertexMap,
				sizeof(int)*this->TriangleVertexMapAllocatedSize);
			for(int i = oldSize; i < this->TriangleVertexMapAllocatedSize - 1; i++)
				this->TriangleVertexMap[i] = 0;
	}

	// Add the vertices to the stream
	std::map<int, int>::iterator sIter;

	// vertex0
	sIter = this->VertexStream.find(vertex0Id);
	if(sIter == this->VertexStream.end()){
		// this vertex hasn't been streamed yet, let's add it
		this->VertexStream[vertex0Id] = this->VertexMapSize;
		this->VertexMap[this->VertexMapSize]
			= this->AddMeshVertex(vertex0Id, scalar0);
		this->VertexMapSize++;
		this->TriangleVertexMapSize++;
	}

	// vertex1
	sIter =
		this->VertexStream.find(vertex1Id);
	if(sIter == this->VertexStream.end()){
		// this vertex hasn't been streamed yet, let's add it
		this->VertexStream[vertex1Id] = this->VertexMapSize;
		this->VertexMap[this->VertexMapSize]
			= this->AddMeshVertex(vertex1Id, scalar1);
		this->VertexMapSize++;
		this->TriangleVertexMapSize++;
	}

	// vertex2
	sIter =
		this->VertexStream.find(vertex2Id);
	if(sIter == this->VertexStream.end()){
		// this vertex hasn't been streamed yet, let's add it
		this->VertexStream[vertex2Id] = this->VertexMapSize;
		this->VertexMap[this->VertexMapSize]
			= this->AddMeshVertex(vertex2Id, scalar2);
		this->VertexMapSize++;
		this->TriangleVertexMapSize++;
	}

	// vertex3
	sIter =
		this->VertexStream.find(vertex3Id);
	if(sIter == this->VertexStream.end()){
		// this vertex hasn't been streamed yet, let's add it
		this->VertexStream[vertex3Id] = this->VertexMapSize;
		this->VertexMap[this->VertexMapSize]
			= this->AddMeshVertex(vertex3Id, scalar3);
		this->VertexMapSize++;
		this->TriangleVertexMapSize++;
	}

	this->AddMeshTetrahedron(vertex0Id, scalar0, vertex1Id, scalar1,
		vertex2Id, scalar2, vertex3Id, scalar3);

	return 0;
}

//----------------------------------------------------------------------------
int vtkReebGraph::StreamTriangle(	vtkIdType vertex0Id, double scalar0,
																	vtkIdType vertex1Id, double scalar1,
																	vtkIdType vertex2Id, double scalar2){

	if(!this->VertexMapAllocatedSize){
		// first allocate an arbitrary size
		this->VertexMapAllocatedSize = vtkReebGraphInitialStreamSize;
		this->VertexMap = (vtkIdType *) malloc(
			sizeof(vtkIdType)*this->VertexMapAllocatedSize);
		memset(this->VertexMap, 0, sizeof(vtkIdType)*this->VertexMapAllocatedSize);
	}
	else if(this->VertexMapSize >= this->VertexMapAllocatedSize - 3){
		int oldSize = this->VertexMapAllocatedSize;
		this->VertexMapAllocatedSize <<= 1;
		this->VertexMap = (vtkIdType *) realloc(this->VertexMap,
			sizeof(vtkIdType)*this->VertexMapAllocatedSize);
		for(int i = oldSize; i < this->VertexMapAllocatedSize - 1; i++)
			this->VertexMap[i] = 0;
	}

	// same thing with the triangle map
	if(!this->TriangleVertexMapAllocatedSize){
		// first allocate an arbitrary size
		this->TriangleVertexMapAllocatedSize = vtkReebGraphInitialStreamSize;
		this->TriangleVertexMap = (int *) malloc(
			sizeof(int)*this->TriangleVertexMapAllocatedSize);
		memset(this->TriangleVertexMap, 0,
			sizeof(int)*this->TriangleVertexMapAllocatedSize);
	}
	else if(this->TriangleVertexMapSize >=
		this->TriangleVertexMapAllocatedSize - 3){

			int oldSize = this->TriangleVertexMapAllocatedSize;
			this->TriangleVertexMapAllocatedSize <<= 1;
			this->TriangleVertexMap = (int *) realloc(this->TriangleVertexMap,
				sizeof(int)*this->TriangleVertexMapAllocatedSize);
			for(int i = oldSize; i < this->TriangleVertexMapAllocatedSize - 1; i++)
				this->TriangleVertexMap[i] = 0;
	}

	// Add the vertices to the stream
	std::map<int, int>::iterator sIter;

	// vertex0
	sIter = this->VertexStream.find(vertex0Id);
	if(sIter == this->VertexStream.end()){
		// this vertex hasn't been streamed yet, let's add it
		this->VertexStream[vertex0Id] = this->VertexMapSize;
		this->VertexMap[this->VertexMapSize]
			= this->AddMeshVertex(vertex0Id, scalar0);
		this->VertexMapSize++;
		this->TriangleVertexMapSize++;
	}

	// vertex1
	sIter =
		this->VertexStream.find(vertex1Id);
	if(sIter == this->VertexStream.end()){
		// this vertex hasn't been streamed yet, let's add it
		this->VertexStream[vertex1Id] = this->VertexMapSize;
		this->VertexMap[this->VertexMapSize]
			= this->AddMeshVertex(vertex1Id, scalar1);
		this->VertexMapSize++;
		this->TriangleVertexMapSize++;
	}

	// vertex2
	sIter =
		this->VertexStream.find(vertex2Id);
	if(sIter == this->VertexStream.end()){
		// this vertex hasn't been streamed yet, let's add it
		this->VertexStream[vertex2Id] = this->VertexMapSize;
		this->VertexMap[this->VertexMapSize]
			= this->AddMeshVertex(vertex2Id, scalar2);
		this->VertexMapSize++;
		this->TriangleVertexMapSize++;
	}

	this->AddMeshTriangle(vertex0Id, scalar0, vertex1Id, scalar1,
		vertex2Id, scalar2);

	return 0;
}

//----------------------------------------------------------------------------
int vtkReebGraph::Build(vtkPolyData *mesh, vtkDataArray *scalarField)
{

  for(vtkIdType i = 0; i < mesh->GetNumberOfCells(); i++)
    {
    vtkCell *triangle = mesh->GetCell(i);
    vtkIdList *trianglePointList = triangle->GetPointIds();
    if(trianglePointList->GetNumberOfIds() != 3)
      return vtkReebGraph::ERR_NOT_A_SIMPLICIAL_MESH;
    StreamTriangle(trianglePointList->GetId(0),
      scalarField->GetComponent(trianglePointList->GetId(0),0),
      trianglePointList->GetId(1),
      scalarField->GetComponent(trianglePointList->GetId(1),0),
      trianglePointList->GetId(2),
      scalarField->GetComponent(trianglePointList->GetId(2),0));
    }

  this->CloseStream();

  return 0;
}

//----------------------------------------------------------------------------
int vtkReebGraph::Build(vtkUnstructuredGrid *mesh, vtkDataArray *scalarField)
{
  for(vtkIdType i = 0; i < mesh->GetNumberOfCells(); i++)
    {
    vtkCell *tet = mesh->GetCell(i);
    vtkIdList *tetPointList = tet->GetPointIds();
    if(tetPointList->GetNumberOfIds() != 4)
      return vtkReebGraph::ERR_NOT_A_SIMPLICIAL_MESH;
    StreamTetrahedron(tetPointList->GetId(0),
      scalarField->GetComponent(tetPointList->GetId(0),0),
      tetPointList->GetId(1),
      scalarField->GetComponent(tetPointList->GetId(1),0),
      tetPointList->GetId(2),
      scalarField->GetComponent(tetPointList->GetId(2),0),
      tetPointList->GetId(3),
      scalarField->GetComponent(tetPointList->GetId(3),0));
    }

  this->CloseStream();

  return 0;
}

//----------------------------------------------------------------------------
int vtkReebGraph::GetNumberOfArcs()
{
  if(!this->ArcNumber)
    for(vtkIdType arcId = 1; arcId < this->MainArcTable.Size; arcId++)
    {
      if(!vtkReebGraphIsArcCleared(this, arcId)) this->ArcNumber++;
    }

  return this->ArcNumber;
}

//----------------------------------------------------------------------------
int vtkReebGraph::GetNumberOfConnectedComponents()
{
  if(!this->ArcLoopTable) this->FindLoops();
  return this->ConnectedComponentNumber;
}

//----------------------------------------------------------------------------
int vtkReebGraph::GetNumberOfNodes()
{

  if(!this->NodeNumber)
    for(vtkIdType nodeId = 1; nodeId < this->MainNodeTable.Size; nodeId++)
    {
      if(!vtkReebGraphIsNodeCleared(this, nodeId)) this->NodeNumber++;
    }

  return this->NodeNumber;

}

//----------------------------------------------------------------------------
vtkIdType vtkReebGraph::GetNextNodeId()
{

  for(vtkIdType nodeId = this->currentNodeId + 1;
    nodeId < this->MainNodeTable.Size; nodeId++)
  {
    if(!vtkReebGraphIsNodeCleared(this, nodeId))
    {
      this->currentNodeId = nodeId;
      return this->currentNodeId;
    }
  }

  return this->currentNodeId;
}

//----------------------------------------------------------------------------
vtkIdType vtkReebGraph::GetPreviousNodeId()
{
  if(!this->currentNodeId)
  {
    return this->GetNextNodeId();
  }

  for(vtkIdType nodeId = this->currentNodeId - 1; nodeId > 0; nodeId--)
  {
    if(!vtkReebGraphIsNodeCleared(this, nodeId))
    {
      this->currentNodeId = nodeId;
      return this->currentNodeId;
    }
  }

  return this->currentNodeId;
}

//----------------------------------------------------------------------------
vtkIdType vtkReebGraph::GetNextArcId()
{
  for(vtkIdType arcId = this->currentArcId + 1;
    arcId < this->MainArcTable.Size; arcId++)
  {
	  if(!vtkReebGraphIsArcCleared(this, arcId))
    {
      this->currentArcId = arcId;
      return this->currentArcId;
    }
  }

  return this->currentArcId;

}

//----------------------------------------------------------------------------
vtkIdType vtkReebGraph::GetPreviousArcId()
{

  if(!this->currentArcId)
  {
    return this->GetNextArcId();
  }

  for(vtkIdType arcId = this->currentArcId - 1; arcId > 0; arcId--)
  {
    if(!vtkReebGraphIsArcCleared(this, arcId))
    {
      this->currentArcId = arcId;
      return this->currentArcId;
    }
  }

  return this->currentArcId;

}

//----------------------------------------------------------------------------
vtkIdType vtkReebGraph::GetArcDownNodeId(vtkIdType arcId)
{
  return (vtkReebGraphGetArc(this, arcId))->NodeId0;
}

//----------------------------------------------------------------------------
vtkIdType vtkReebGraph::GetArcUpNodeId(vtkIdType arcId)
{
  return (vtkReebGraphGetArc(this, arcId))->NodeId1;
}

//----------------------------------------------------------------------------
double vtkReebGraph::GetNodeScalarValue(vtkIdType nodeId)
{
  return (vtkReebGraphGetNode(this, nodeId))->Value;
}

//----------------------------------------------------------------------------
vtkIdType vtkReebGraph::GetNodeVertexId(vtkIdType nodeId)
{
  return (vtkReebGraphGetNode(this, nodeId))->VertexId;
}

//----------------------------------------------------------------------------
int vtkReebGraph::Build(vtkPolyData *mesh, vtkIdType scalarFieldId)
{

  vtkPointData *pointData = mesh->GetPointData();
  vtkDataArray *scalarField = pointData->GetArray(scalarFieldId);

  if(!scalarField) return vtkReebGraph::ERR_NO_SUCH_FIELD;

  return this->Build(mesh, scalarField);
}

//----------------------------------------------------------------------------
int vtkReebGraph::Build(vtkUnstructuredGrid *mesh, vtkIdType scalarFieldId)
{

  vtkPointData *pointData = mesh->GetPointData();
  vtkDataArray *scalarField = pointData->GetArray(scalarFieldId);

  if(!scalarField) return vtkReebGraph::ERR_NO_SUCH_FIELD;

  return this->Build(mesh, scalarField);
}

//----------------------------------------------------------------------------
int vtkReebGraph::Build(vtkPolyData *mesh, const char* scalarFieldName)
{

  int scalarFieldId = 0;

  vtkPointData *pointData = mesh->GetPointData();
  vtkDataArray *scalarField =
    pointData->GetArray(scalarFieldName, scalarFieldId);

  if(!scalarField) return vtkReebGraph::ERR_NO_SUCH_FIELD;

  return this->Build(mesh, scalarField);
}

//----------------------------------------------------------------------------
int vtkReebGraph::Build(vtkUnstructuredGrid *mesh, const char* scalarFieldName)
{

  int scalarFieldId = 0;

  vtkPointData *pointData = mesh->GetPointData();
  vtkDataArray *scalarField =
    pointData->GetArray(scalarFieldName, scalarFieldId);

  if(!scalarField) return vtkReebGraph::ERR_NO_SUCH_FIELD;

  return this->Build(mesh, scalarField);
}

//----------------------------------------------------------------------------
int vtkReebGraph::GetNumberOfLoops()
{

  if(!this->ArcLoopTable) this->FindLoops();
  return this->LoopNumber - this->RemovedLoopNumber;
}

//----------------------------------------------------------------------------
vtkMutableDirectedGraph* vtkReebGraph::GetVtkGraph()
{
  vtkMutableDirectedGraph* g = vtkMutableDirectedGraph::New();

  vtkVariantArray *vertexProp = vtkVariantArray::New();
  // vertex Ids are for now the only sufficient information.
  vertexProp->SetNumberOfValues(1);

  vtkIdTypeArray *vertexIds = vtkIdTypeArray::New();
  vertexIds->SetName("Vertex Ids");
  g->GetVertexData()->AddArray(vertexIds);

  vtkIdType prevNodeId = -1, nodeId = 0;

  std::map<int, int> vMap;
  int vIt = 0;

  // roll back node list
  while(prevNodeId != nodeId){
    prevNodeId = nodeId;
    nodeId = this->GetPreviousNodeId();
  }
  prevNodeId = -1;

  // copy the nodes.
  while(prevNodeId != nodeId){
    vtkIdType nodeVertexId = this->GetNodeVertexId(nodeId);
    vMap[nodeId] = vIt;
    vertexProp->SetValue(0, nodeVertexId);
    g->AddVertex(vertexProp);

    prevNodeId = nodeId;
    nodeId = this->GetNextNodeId();
    vIt++;
  }

  // roll back arc list
  int arcId = 0, prevArcId = -1;
  while(arcId != prevArcId){
    prevArcId = arcId;
    arcId = this->GetPreviousArcId();
  }
  prevArcId = -1;

  // TODO: map the deg2list to each arc

  // now copy the arcs
  while(prevArcId != arcId){
    std::map<int, int>::iterator downIt, upIt;
    downIt = vMap.find(this->GetArcDownNodeId(arcId));
    upIt = vMap.find(this->GetArcUpNodeId(arcId));
    if((downIt != vMap.end())&&(upIt != vMap.end())){
      g->AddEdge(downIt->second, upIt->second);
    }

    prevArcId = arcId;
    arcId = this->GetNextArcId();
  }

  return g;
}

