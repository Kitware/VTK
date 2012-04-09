/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkExtentSplitter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "vtkExtentSplitter.h"

#include "vtkObjectFactory.h"

#include <map>
#include <queue>
#include <vector>

vtkStandardNewMacro(vtkExtentSplitter);

//----------------------------------------------------------------------------
struct vtkExtentSplitterExtent
{
  int extent[6];
};

struct vtkExtentSplitterSource
{
  int extent[6];
  int priority;
};

struct vtkExtentSplitterSubExtent
{
  int extent[6];
  int source;
};

class vtkExtentSplitterInternals
{
public:
  typedef std::queue<vtkExtentSplitterExtent> QueueType;
  typedef std::map<int, vtkExtentSplitterSource> SourcesType;
  typedef std::vector<vtkExtentSplitterSubExtent> SubExtentsType;
  SourcesType Sources;
  QueueType Queue;
  SubExtentsType SubExtents;
};

//----------------------------------------------------------------------------
vtkExtentSplitter::vtkExtentSplitter()
{
  this->Internal = new vtkExtentSplitterInternals;
  this->PointMode = 0;
}

//----------------------------------------------------------------------------
vtkExtentSplitter::~vtkExtentSplitter()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void vtkExtentSplitter::PrintSelf(ostream& os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  vtkIndent nextIndent = indent.GetNextIndent();
  os << indent << "PointMode: " << this->PointMode << "\n";
  if(this->Internal->Sources.empty())
    {
    os << indent << "Extent Sources: (none)\n";
    }
  else
    {
    os << indent << "Extent Sources: (format = \"id priority: extent\")\n";
    for(vtkExtentSplitterInternals::SourcesType::const_iterator src =
          this->Internal->Sources.begin();
        src != this->Internal->Sources.end(); ++src)
      {
      const int* extent = src->second.extent;
      os << nextIndent << src->first
         << " " << src->second.priority << ": "
         << extent[0] << " " << extent[1] << "  "
         << extent[2] << " " << extent[3] << "  "
         << extent[4] << " " << extent[5] << "\n";
      }
    }
  os << indent << "Number of Extents in Queue: " << 
    static_cast<int>(this->Internal->Queue.size()) << "\n";
  if(this->Internal->SubExtents.empty())
    {
    os << indent << "SubExtents: (none)\n";
    }
  else
    {
    os << indent << "SubExtents: (format = \"id: extent\")\n";
    for(vtkExtentSplitterInternals::SubExtentsType::const_iterator i =
          this->Internal->SubExtents.begin();
        i != this->Internal->SubExtents.end(); ++i)
      {
      const int* extent = i->extent;
      os << nextIndent << i->source << ": "
         << extent[0] << " " << extent[1] << "  "
         << extent[2] << " " << extent[3] << "  "
         << extent[4] << " " << extent[5] << "\n";        
      }
    }
}

//----------------------------------------------------------------------------
void vtkExtentSplitter::AddExtentSource(int id, int priority, int x0, int x1,
                                        int y0, int y1, int z0, int z1)
{
  // Add the source.
  vtkExtentSplitterSource& source = this->Internal->Sources[id];
  source.extent[0] = x0;
  source.extent[1] = x1;
  source.extent[2] = y0;
  source.extent[3] = y1;
  source.extent[4] = z0;
  source.extent[5] = z1;
  source.priority = priority;
  
  // Previously calculated sub-extents are now invalid.
  this->Internal->SubExtents.clear();
}

//----------------------------------------------------------------------------
void vtkExtentSplitter::AddExtentSource(int id, int priority, int* extent)
{
  this->AddExtentSource(id, priority, extent[0], extent[1], extent[2],
                        extent[3], extent[4], extent[5]);
}

//----------------------------------------------------------------------------
void vtkExtentSplitter::RemoveExtentSource(int id)
{
  // Remove the source.
  this->Internal->Sources.erase(id);

  // Previously calculated sub-extents are now invalid.
  this->Internal->SubExtents.clear();
}

//----------------------------------------------------------------------------
void vtkExtentSplitter::RemoveAllExtentSources()
{
  // Clear the set of sources.
  this->Internal->Sources.clear();
  
  // Previously calculated sub-extents are now invalid.
  this->Internal->SubExtents.clear();
}

//----------------------------------------------------------------------------
void vtkExtentSplitter::AddExtent(int x0, int x1, int y0, int y1,
                                  int z0, int z1)
{
  // Queue the extent.
  vtkExtentSplitterExtent e;
  e.extent[0] = x0;
  e.extent[1] = x1;
  e.extent[2] = y0;
  e.extent[3] = y1;
  e.extent[4] = z0;
  e.extent[5] = z1;
  this->Internal->Queue.push(e);
  
  // Previously calculated sub-extents are now invalid.
  this->Internal->SubExtents.clear();
}

//----------------------------------------------------------------------------
void vtkExtentSplitter::AddExtent(int* extent)
{
  this->AddExtent(extent[0], extent[1], extent[2],
                  extent[3], extent[4], extent[5]);
}

//----------------------------------------------------------------------------
int vtkExtentSplitter::GetNumberOfSubExtents()
{
  return static_cast<int>(this->Internal->SubExtents.size());
}

//----------------------------------------------------------------------------
int* vtkExtentSplitter::GetSubExtent(int index)
{
  if(index < 0 || index >= this->GetNumberOfSubExtents())
    {
    static int dummy[6] = {0, -1, 0, -1, 0, -1};
    vtkErrorMacro("SubExtent index " << index << " is out of range [0,"
                  << this->GetNumberOfSubExtents()-1 << "]");
    return dummy;
    }
  return this->Internal->SubExtents[index].extent;
}

//----------------------------------------------------------------------------
void vtkExtentSplitter::GetSubExtent(int index, int* extent)
{
  if(index < 0 || index >= this->GetNumberOfSubExtents())
    {
    vtkErrorMacro("SubExtent index " << index << " is out of range [0,"
                  << this->GetNumberOfSubExtents()-1 << "]");
    extent[0] = 0;
    extent[1] = -1;
    extent[2] = 0;
    extent[3] = -1;
    extent[4] = 0;
    extent[5] = -1;
    }
  else
    {
    int i;
    int* e = this->Internal->SubExtents[index].extent;
    for(i=0; i < 6; ++i)
      {
      extent[i] = e[i];
      }
    }
}

//----------------------------------------------------------------------------
int vtkExtentSplitter::GetSubExtentSource(int index)
{
  if(index < 0 || index >= this->GetNumberOfSubExtents())
    {
    vtkErrorMacro("SubExtent index " << index << " is out of range [0,"
                  << this->GetNumberOfSubExtents()-1 << "]");
    return -1;
    }
  return this->Internal->SubExtents[index].source;  
}

//----------------------------------------------------------------------------
int vtkExtentSplitter::ComputeSubExtents()
{
  // Assume success.
  int result = 1;
  
  vtkExtentSplitterInternals::SubExtentsType subExtents;
  int bestPriority;
  int dimensionality = 0;
  
  while(!this->Internal->Queue.empty())
    {
    // Pop the next extent off the queue.
    vtkExtentSplitterExtent e = this->Internal->Queue.front();
    this->Internal->Queue.pop();
    
    // In non-PointMode, intersections must have the same topological
    // dimension as the original extent.  This will prevent
    // high-priority source extents from repeatedly producing
    // single-point-wide intersections.
    if(!this->PointMode)
      {
      dimensionality = (((e.extent[1]-e.extent[0] > 0)?1:0)+
                        ((e.extent[3]-e.extent[2] > 0)?1:0)+
                        ((e.extent[5]-e.extent[4] > 0)?1:0));
      }
    
    // Intersect the extent with each extent source.
    subExtents.clear();
    bestPriority = -1;
    vtkExtentSplitterSubExtent se;
    for(vtkExtentSplitterInternals::SourcesType::const_iterator src =
          this->Internal->Sources.begin();
        src != this->Internal->Sources.end(); ++src)
      {
      se.source = src->first;
      if(this->IntersectExtents(e.extent, src->second.extent, se.extent) &&
         (this->PointMode ||
          (dimensionality == (((se.extent[1]-se.extent[0] > 0)?1:0)+
                              ((se.extent[3]-se.extent[2] > 0)?1:0)+
                              ((se.extent[5]-se.extent[4] > 0)?1:0)))))
        {
        // Non-zero intersection volume.  Add the extent as a
        // candidate for best extent.
        if(src->second.priority > bestPriority)
          {
          // New highest priority.  Clear previous intersections with
          // lower priority.
          subExtents.clear();
          subExtents.push_back(se);
          bestPriority = src->second.priority;
          }
        else if(src->second.priority == bestPriority)
          {
          // Matching priority.  Add this intersection to the list.
          subExtents.push_back(se);
          }
        }
      }
    
    // Check whether any extent sources intersected the extent.
    if(subExtents.empty())
      {
      // No extent source intersected the extent.  Add the extent as
      // an error.
      int i;
      result = 0;
      se.source = -1;
      for(i=0; i < 6; ++i)
        {
        se.extent[i] = e.extent[i];
        }
      this->Internal->SubExtents.push_back(se);
      }
    else
      {
      // Choose the extent intersection with the largest volume.
      int bestVolume = 0;
      int bestIndex = 0;
      int i;
      for(i=0; i < static_cast<int>(subExtents.size()); ++i)
        {
        int* extent = subExtents[i].extent;
        int volume = ((extent[1]-extent[0]+1)*
                      (extent[3]-extent[2]+1)*
                      (extent[5]-extent[4]+1));
        if(volume > bestVolume)
          {
          bestVolume = volume;
          bestIndex = i;
          }
        }
      
      // Add this extent source with its sub-extent.
      se.source = subExtents[bestIndex].source;
      for(i=0; i < 6; ++i)
        {
        se.extent[i] = subExtents[bestIndex].extent[i];
        }
      this->Internal->SubExtents.push_back(se);
      
      // Subtract the sub-extent from the extent and split the rest of
      // the volume into more sub-extents.
      this->SplitExtent(e.extent, se.extent);
      }
    }
  
  return result;
}

//----------------------------------------------------------------------------
void vtkExtentSplitter::SplitExtent(int* extent, int* subextent)
{
  // Subtract the volume described by subextent from that described by
  // extent.  Split the remaining region into rectangular solids and
  // queue them as additional extents.  We may assume that subextent
  // is completely contained by extent.
  vtkExtentSplitterExtent e;
  
  // In PointMode, there are no cell data, so we can skip over cells.
  int pointMode = this->PointMode?1:0;
  
  // Split with xy-planes.
  if(extent[4] < subextent[4])
    {
    e.extent[0] = extent[0];
    e.extent[1] = extent[1];
    e.extent[2] = extent[2];
    e.extent[3] = extent[3];
    e.extent[4] = extent[4];
    e.extent[5] = subextent[4]-pointMode;
    this->Internal->Queue.push(e);
    
    extent[4] = subextent[4];
    }
  if(extent[5] > subextent[5])
    {
    e.extent[0] = extent[0];
    e.extent[1] = extent[1];
    e.extent[2] = extent[2];
    e.extent[3] = extent[3];
    e.extent[4] = subextent[5]+pointMode;
    e.extent[5] = extent[5];
    this->Internal->Queue.push(e);
    
    extent[5] = subextent[5];
    }
  
  // Split with xz-planes.
  if(extent[2] < subextent[2])
    {
    e.extent[0] = extent[0];
    e.extent[1] = extent[1];
    e.extent[2] = extent[2];
    e.extent[3] = subextent[2]-pointMode;
    e.extent[4] = extent[4];
    e.extent[5] = extent[5];
    this->Internal->Queue.push(e);
    
    extent[2] = subextent[2];
    }
  if(extent[3] > subextent[3])
    {
    e.extent[0] = extent[0];
    e.extent[1] = extent[1];
    e.extent[2] = subextent[3]+pointMode;
    e.extent[3] = extent[3];
    e.extent[4] = extent[4];
    e.extent[5] = extent[5];
    this->Internal->Queue.push(e);
    
    extent[3] = subextent[3];
    }
  
  // Split with yz-planes.
  if(extent[0] < subextent[0])
    {
    e.extent[0] = extent[0];
    e.extent[1] = subextent[0]-pointMode;
    e.extent[2] = extent[2];
    e.extent[3] = extent[3];
    e.extent[4] = extent[4];
    e.extent[5] = extent[5];
    this->Internal->Queue.push(e);
    
    extent[0] = subextent[0];
    }
  if(extent[1] > subextent[1])
    {
    e.extent[0] = subextent[1]+pointMode;
    e.extent[1] = extent[1];
    e.extent[2] = extent[2];
    e.extent[3] = extent[3];
    e.extent[4] = extent[4];
    e.extent[5] = extent[5];
    this->Internal->Queue.push(e);
    
    // Leave this line out because the value will not be used:
    //extent[1] = subextent[1];
    }
  
  // At this point, we should have extent[i] == subextent[i] for 0 <= i < 6.
  // No more volume remains.
}

//----------------------------------------------------------------------------
int vtkExtentSplitter::IntersectExtents(const int* extent1, const int* extent2,
                                        int* result)
{
  if((extent1[0] > extent2[1]) || (extent1[2] > extent2[3]) ||
     (extent1[4] > extent2[5]) || (extent1[1] < extent2[0]) ||
     (extent1[3] < extent2[2]) || (extent1[5] < extent2[4]))
    {
    // No intersection of extents.
    return 0;
    }
  
  // Get the intersection of the extents.
  result[0] = this->Max(extent1[0], extent2[0]);
  result[1] = this->Min(extent1[1], extent2[1]);
  result[2] = this->Max(extent1[2], extent2[2]);
  result[3] = this->Min(extent1[3], extent2[3]);
  result[4] = this->Max(extent1[4], extent2[4]);
  result[5] = this->Min(extent1[5], extent2[5]);
  
  return 1;
}

//----------------------------------------------------------------------------
int vtkExtentSplitter::Min(int a, int b)
{
  return (a < b)? a : b;
}

//----------------------------------------------------------------------------
int vtkExtentSplitter::Max(int a, int b)
{
  return (a > b)? a : b;
}
