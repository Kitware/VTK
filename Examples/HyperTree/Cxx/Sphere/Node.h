/*=========================================================================

Copyright (c) Kitware Inc.
All rights reserved.

=========================================================================*/
// This class was written by Daniel Aguilera and Philippe Pebay
// This work was supported by Commissariat a l'Energie Atomique (CEA/DIF)

#ifndef NODE_H
#define NODE_H

const static char * NODE_H_SCCS_ID = "%Z% DSSI/SNEC/LDDC %M%   %I%     %G%";

#include <vector>
class Cell;

class Node
{
   public:
      Node (int id, double x, double y, double z);
      Node (double x, double y, double z);
      ~Node ();

      void registerCell (Cell * c);
      void unregisterCell (Cell * c);

      void setId(int id) {_id = id;}
      int getId() {return _id;}

      double getX() {return _x;}
      double getY() {return _y;}
      double getZ() {return _z;}

      void replaceBy (Node *);

   protected:
      std::vector<Cell*> _cells;
      int _id;
      double _x;
      double _y;
      double _z;

   private:
      
} ;

#endif /* NODE_H */
