/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCoincidentPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "vtkCoincidentPoints.h"

#include "vtkIdList.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"

#include <vtkstd/set>
#include <vtkstd/vector>
#include <vtkstd/map>



//----------------------------------------------------------------------------
// vtkCoincidentPoints::implementation

class vtkCoincidentPoints::implementation
{
public:
  implementation()
    {
    this->TraversalIterator = this->CoordMap.end();
    }

  ~implementation()
    {
    this->CoordMap.clear();
    }


  struct Coord
  {
    double coord[3];

    Coord()
      {
      this->coord[0] = -1.0;
      this->coord[1] = -1.0;
      this->coord[2] = -1.0;
      }
    Coord( const Coord & src )
      {
      this->coord[0] = src.coord[0];
      this->coord[1] = src.coord[1];
      this->coord[2] = src.coord[2];
      }
    Coord( const double src[3] )
      {
      this->coord[0] = src[0];
      this->coord[1] = src[1];
      this->coord[2] = src[2];
      }
    Coord( const double x, const double y, const double z )
      {
      this->coord[0] = x;
      this->coord[1] = y;
      this->coord[2] = z;
      }
    ~Coord() {}

    inline bool operator < (const Coord & other) const
      {
      return this->coord[0] < other.coord[0] ||
        (this->coord[0] == other.coord[0] && (this->coord[1] < other.coord[1] ||
        (this->coord[1] == other.coord[1] && this->coord[2] < other.coord[2])));
      }
  };

  typedef vtkstd::map<Coord, vtkSmartPointer<vtkIdList> >::iterator MapCoordIter;

  vtkCoincidentPoints* Self;

  vtkstd::map<Coord, vtkSmartPointer<vtkIdList> > CoordMap;
  vtkstd::map<vtkIdType, vtkIdType> CoincidenceMap;
  MapCoordIter TraversalIterator;
};

//----------------------------------------------------------------------------
// vtkCoincidentPoints

vtkStandardNewMacro(vtkCoincidentPoints);

vtkCoincidentPoints::vtkCoincidentPoints()
{
  this->Implementation = new implementation();
  this->Implementation->Self = this;
}

vtkCoincidentPoints::~vtkCoincidentPoints()
{
  delete this->Implementation;
}

void vtkCoincidentPoints::Clear()
{
 this->Implementation->CoordMap.clear();
 this->Implementation->CoincidenceMap.clear();
}

void vtkCoincidentPoints::PrintSelf( ostream& os, vtkIndent indent )
{
  this->Superclass::PrintSelf( os, indent );
  //os << indent << "MaximumDepth: " << this->MaximumDepth << "\n";
}

void vtkCoincidentPoints::AddPoint(vtkIdType Id, const double point[3])
{
  implementation::Coord coord(point);
  implementation::MapCoordIter mapIter = this->Implementation->CoordMap.find(coord);
  if(mapIter == this->Implementation->CoordMap.end())
    {
    vtkSmartPointer<vtkIdList> idSet = vtkSmartPointer<vtkIdList>::New();
    idSet->InsertNextId(Id);
    this->Implementation->CoordMap[coord] = idSet;
    }
  else
    {
    (*mapIter).second->InsertNextId(Id);
    }
}

vtkIdList * vtkCoincidentPoints::GetCoincidentPointIds(const double point[3])
{
  implementation::Coord coord(point);
  implementation::MapCoordIter mapIter = this->Implementation->CoordMap.find(coord);
  if(mapIter == this->Implementation->CoordMap.end())
    {
    return NULL;
    }

  if((*mapIter).second->GetNumberOfIds() > 1)
    {
    return (*mapIter).second;
    }
  else
    {
    // No Coincident Points
    return NULL;
    }
}

void vtkCoincidentPoints::RemoveNonCoincidentPoints()
{
  implementation::MapCoordIter mapIter = this->Implementation->CoordMap.begin();
  while(mapIter != this->Implementation->CoordMap.end())
    {
    if( (*mapIter).second->GetNumberOfIds() <= 1 )
      {
      this->Implementation->CoordMap.erase(mapIter++);
      }
    else
      {
      ++mapIter;
      }
    }
}

vtkIdList * vtkCoincidentPoints::GetNextCoincidentPointIds()
{
  vtkIdList * rvalue = NULL;
  if(this->Implementation->TraversalIterator !=
    this->Implementation->CoordMap.end())
    {
    rvalue = (*this->Implementation->TraversalIterator).second;
    ++this->Implementation->TraversalIterator;
    return rvalue;
    }

  return rvalue;
}

void vtkCoincidentPoints::InitTraversal()
{
  this->Implementation->TraversalIterator = this->Implementation->CoordMap.begin();
}

//----------------------------------------------------------------------------
// vtkSpiralkVertices - calculate points at a regular interval along a parametric
// spiral.
void vtkCoincidentPoints::SpiralPoints(vtkIdType num, vtkPoints * offsets)
{
  int maxIter = 10;
  double pi = vtkMath::Pi();
  double a = 1/(4*pi*pi);
  offsets->Initialize();
  offsets->SetNumberOfPoints(num);

  for (vtkIdType i = 0; i < num; i++)
    {
    double d = 2.0*i/sqrt(3.0);
    // We are looking for points at regular intervals along the parametric spiral
    // x = t*cos(2*pi*t)
    // y = t*sin(2*pi*t)
    // We cannot solve this equation exactly, so we use newton's method.
    // Using an Excel trendline, we find that
    // t = 0.553*d^0.502
    // is an excellent starting point./g
    double t = 0.553*pow(d, 0.502);
    for (int iter = 0; iter < maxIter; iter++)
      {
      double r = sqrt(t*t+a*a);
      double f = pi*(t*r+a*a*log(t+r)) - d;
      double df = 2*pi*r;
      t = t - f/df;
      }
    double x = t*cos(2*pi*t);
    double y = t*sin(2*pi*t);
    offsets->SetPoint(i, x, y, 0);
    }
}
