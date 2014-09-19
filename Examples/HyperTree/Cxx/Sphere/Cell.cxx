/*=========================================================================

Copyright (c) Kitware Inc.
All rights reserved.

=========================================================================*/
// This class was written by Daniel Aguilera and Philippe Pebay
// This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)

#include "Mesh.h"
#include "Cell.h"
#include "Node.h"

#include <math.h>
#include <iostream>
#include <algorithm>

using namespace std;

int Cell::_count = 0;
int Cell::_refinedCount = 0;
int Cell::_refineNumber = 0;
double Cell::_R = 10.0;

/*-------------------------------------------------------------------------
  -----------------------------------------------------------------------*/
Cell::Cell (int id, vector<Node*> nodes)
{
  _refined = false;
  _id = id;
  _nodes = nodes;
  _nodeIds = 0;
  _count++;

  for (vector<Node*>::iterator it = _nodes.begin(); it != _nodes.end(); it++)
    {
    (*it)->registerCell (this);
    }

}

/*-------------------------------------------------------------------------
  -----------------------------------------------------------------------*/
Cell::~Cell()
{
  for (vector<Node*>::iterator it = _nodes.begin(); it != _nodes.end(); it++)
    {
    (*it)->unregisterCell (this);
    }
  delete [] _nodeIds;
}

/*-------------------------------------------------------------------------
  -----------------------------------------------------------------------*/
void Cell::refineIfNeeded()
{
  if (_refined) return;

  double v1 = computeValue (_nodes[0]);
  for (int i = 1; i < 8; i++)
    {
    double v2 = computeValue (_nodes[i]);
    if (v1*v2 < 0)
      {
      this->refine();
      return;
      }
    }
}

/*-------------------------------------------------------------------------
  -----------------------------------------------------------------------*/
void Cell::refine ()
{
  _refinedCount++;
  // attention : petite inversion entre X et Z par rapport a la creation du maillage initial
  _cells = Mesh::instance()->createCells (_refineNumber + 1, _refineNumber + 1, _refineNumber + 1,
                                          _nodes[0], _nodes[3], _nodes[2], _nodes[1],
                                          _nodes[4], _nodes[7], _nodes[6], _nodes[5]);

  // desenregistrement de la maille aupres des noeuds
  for (vector<Node*>::iterator it = _nodes.begin(); it != _nodes.end(); it++)
    {
    (*it)->unregisterCell (this);
    }
  delete [] _nodeIds;

  _count--;
  _refined = true;
}

/*-------------------------------------------------------------------------
  -----------------------------------------------------------------------*/
double Cell::computeValue (Node * n)
{
  // sphere de rayon _R
  double x = n->getX();
  double y = n->getY();
  double z = n->getZ();

  return (sqrt (x*x + y*y + z*z) - _R);
}


/*-------------------------------------------------------------------------
  -----------------------------------------------------------------------*/
vtkIdType * Cell::getNodeIds()
{
  delete [] _nodeIds;
  _nodeIds = new vtkIdType[_nodes.size()+1];
  int index = 0;
  for (vector<Node*>::iterator it = _nodes.begin(); it != _nodes.end(); it++)
    {
    _nodeIds[index++] = (*it)->getId();
    }
  _nodeIds[index++] = -1;
  return _nodeIds;
}

/*-------------------------------------------------------------------------
  -----------------------------------------------------------------------*/
void Cell::replaceNode (Node* oldNode, Node* newNode)
{
  vector<Node*>::iterator it = std::find (_nodes.begin(), _nodes.end(), oldNode);
  if (it != _nodes.end())
    {
    _nodes.insert (it, newNode);
    _nodes.erase (it);
    oldNode->unregisterCell (this);
    newNode->registerCell (this);
    }
}
