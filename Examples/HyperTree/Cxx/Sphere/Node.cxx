/*=========================================================================

Copyright (c) Kitware Inc.
All rights reserved.

=========================================================================*/
// This class was written by Daniel Aguilera and Philippe Pebay
// This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)

const static char * NODE_CPP_SCCS_ID = "%Z% DSSI/SNEC/LDDC %M%   %I%     %G%";

#include "Node.h"
#include "Cell.h"

#include <algorithm>
#include <iostream>
using namespace std;

/*-------------------------------------------------------------------------
  -----------------------------------------------------------------------*/
Node::Node (int id, double x, double y, double z)
  : _id(id), _x(x), _y(y), _z(z)
{
}

/*-------------------------------------------------------------------------
  -----------------------------------------------------------------------*/
Node::Node (double x, double y, double z)
  : _id(-1), _x(x), _y(y), _z(z)
{
}

/*-------------------------------------------------------------------------
  -----------------------------------------------------------------------*/
Node::~Node()
{
}


/*-------------------------------------------------------------------------
  -----------------------------------------------------------------------*/
void Node::registerCell (Cell * c)
{
  vector<Cell*>::iterator it = find (_cells.begin(),_cells.end(), c);
  if (it == _cells.end()) _cells.push_back (c);
  else cout << "erreur : la maille " << c->getId() << " est deja enregistree pour le noeud " << _id << endl;
}

/*-------------------------------------------------------------------------
  -----------------------------------------------------------------------*/
void Node::unregisterCell (Cell * c)
{
  vector<Cell*>::iterator it = find (_cells.begin(),_cells.end(), c);
  if (it != _cells.end()) _cells.erase(it);
  else cout << "erreur : la maille " << c->getId() << " n'est pas enregistree pour le noeud " << _id << endl;
}


/*-------------------------------------------------------------------------
  -----------------------------------------------------------------------*/
void Node::replaceBy (Node * n)
{
  vector<Cell*> tmpCells = _cells;
  for (vector<Cell*>::iterator it = tmpCells.begin(); it != tmpCells.end(); it++)
  {
    (*it)->replaceNode (this, n);
  }
}
