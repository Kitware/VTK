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
  service     : Constructeur
  description : 

  parametres  : 
  id (int) :
  x (double) :
  y (double) :
  z (double) :
  -----------------------------------------------------------------------*/
Node::Node (int id, double x, double y, double z)
   : _id(id), _x(x), _y(y), _z(z)
{
}

/*-------------------------------------------------------------------------
  service     : Constructeur
  description : 

  parametres  : 
  x (double) :
  y (double) :
  z (double) :
  -----------------------------------------------------------------------*/
Node::Node (double x, double y, double z)
   : _id(-1), _x(x), _y(y), _z(z)
{
}

/*-------------------------------------------------------------------------
  service     : Destructeur
  description : 

  parametres  : Aucun
  -----------------------------------------------------------------------*/
Node::~Node()
{
}


/*-------------------------------------------------------------------------
  service     : registerCell
  description : 

  retour      : aucun

  parametres  : 
  c (Cell *) :
  -----------------------------------------------------------------------*/
void Node::registerCell (Cell * c)
{
   vector<Cell*>::iterator it = find (_cells.begin(),_cells.end(), c);
   if (it == _cells.end()) _cells.push_back (c);
   else cout << "erreur : la maille " << c->getId() << " est deja enregistree pour le noeud " << _id << endl;
}

/*-------------------------------------------------------------------------
  service     : unregisterCell
  description : 

  retour      : aucun

  parametres  : 
  c (Cell *) :
  -----------------------------------------------------------------------*/
void Node::unregisterCell (Cell * c)
{
   vector<Cell*>::iterator it = find (_cells.begin(),_cells.end(), c);
   if (it != _cells.end()) _cells.erase(it);
   else cout << "erreur : la maille " << c->getId() << " n'est pas enregistree pour le noeud " << _id << endl;
}


/*-------------------------------------------------------------------------
  service     : replaceBy
  description : 

  retour      : aucun

  parametres  : 
  n (Node *) :
  -----------------------------------------------------------------------*/
void Node::replaceBy (Node * n)
{
   vector<Cell*> tmpCells = _cells;
   for (vector<Cell*>::iterator it = tmpCells.begin(); it != tmpCells.end(); it++)
   {
      (*it)->replaceNode (this, n);
   }
}
