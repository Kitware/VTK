/*=========================================================================

  Module:    V_GaussIntegration.cpp

  Copyright (c) 2006 Sandia Corporation.
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/


/*
 *
 * GaussIntegration.cpp performs Gauss Integrations
 *
 * This file is part of VERDICT
 *
 */

#define VERDICT_EXPORTS

#include "verdict.h"
#include "V_GaussIntegration.hpp"

#include <math.h>

static int numberGaussPoints;
static int numberNodes;
static int numberDims;
static double gaussPointY[maxNumberGaussPoints];
static double gaussWeight[maxNumberGaussPoints];
static double shapeFunction[maxTotalNumberGaussPoints][maxNumberNodes];
static double dndy1GaussPts[maxTotalNumberGaussPoints][maxNumberNodes];
static double dndy2GaussPts[maxTotalNumberGaussPoints][maxNumberNodes];
static double dndy3GaussPts[maxTotalNumberGaussPoints][maxNumberNodes];
static double totalGaussWeight[maxTotalNumberGaussPoints];
static int totalNumberGaussPts;
static double y1Area[maxNumberGaussPointsTri];
static double y2Area[maxNumberGaussPointsTri];
static double y1Volume[maxNumberGaussPointsTet];
static double y2Volume[maxNumberGaussPointsTet];
static double y3Volume[maxNumberGaussPointsTet];
static double y4Volume[maxNumberGaussPointsTet];  

void GaussIntegration::initialize(int n, int m, int dim, int tri)
{
   numberGaussPoints = n;     
   numberNodes = m;
   numberDims = dim;

   if (tri==1)
   //triangular element
   {
       if ( numberDims == 2)
          totalNumberGaussPts = numberGaussPoints;
       else if (numberDims ==3)
          totalNumberGaussPts =numberGaussPoints ;
   }
   else if (tri == 0)
   {
      if ( numberDims == 2)
         totalNumberGaussPts = numberGaussPoints*numberGaussPoints;
      else if (numberDims ==3)
         totalNumberGaussPts = numberGaussPoints*numberGaussPoints*numberGaussPoints;
   }


}


void GaussIntegration::get_shape_func(double shape_function[], double dndy1_at_gauss_pts[], 
                             double dndy2_at_gauss_pts[], double gauss_weight[])
{
   int i, j;
   for (i=0;i<totalNumberGaussPts; i++)
   {
      for ( j =0;j<numberNodes;j++)
      {
         shape_function[i*maxNumberNodes+j] = 
            shapeFunction[i][j];
         dndy1_at_gauss_pts[i*maxNumberNodes+j]  = dndy1GaussPts[i][j];
         dndy2_at_gauss_pts[i*maxNumberNodes+j]  = dndy2GaussPts[i][j];
      }
   }

   for (  i=0;i<totalNumberGaussPts; i++)
      gauss_weight[i] = totalGaussWeight[i];     
}

void GaussIntegration::get_shape_func(double shape_function[], double dndy1_at_gauss_pts[], 
                                      double dndy2_at_gauss_pts[], double dndy3_at_gauss_pts[], 
                                      double gauss_weight[])
{
   int i, j;
   for ( i =0;i<totalNumberGaussPts;i++)
   {
      for ( j=0;j<numberNodes; j++)
      {
         shape_function[i*maxNumberNodes+j] = 
            shapeFunction[i][j];
         dndy1_at_gauss_pts[i*maxNumberNodes+j] = dndy1GaussPts[i][j];
         dndy2_at_gauss_pts[i*maxNumberNodes+j] = dndy2GaussPts[i][j];
         dndy3_at_gauss_pts[i*maxNumberNodes+j] = dndy3GaussPts[i][j];
      }
   }

   for ( i=0;i<totalNumberGaussPts; i++)
      gauss_weight[i] = totalGaussWeight[i];     
}


void GaussIntegration::get_gauss_pts_and_weight()
{

   switch( numberGaussPoints )
   {
      case 1:
         gaussPointY[0]=  0.0;
         gaussWeight[0] = 2.0;
         break;
      case 2:
         gaussPointY[0] = -0.577350269189626;
         gaussPointY[1] =  0.577350269189626;
         gaussWeight[0] = 1.0;
         gaussWeight[1] = 1.0;
         break;
      case 3:
         gaussPointY[0]= -0.774596669241483;
         gaussPointY[1] = 0.0;
         gaussPointY[2] = 0.774596669241483;
         gaussWeight[0] = 0.555555555555555;
         gaussWeight[1] = 0.888888888888889;
         gaussWeight[2] = 0.555555555555555;
         break;
   }
}

void GaussIntegration::calculate_shape_function_2d_quad()
{
   int ife=0, i, j;
   double y1,y2;
   get_gauss_pts_and_weight();

    switch( numberNodes ){
      case 4:
         for ( i=0; i<numberGaussPoints; i++)
         {
            for ( j=0;j<numberGaussPoints;j++)
            {
               y1 = gaussPointY[i];
               y2 = gaussPointY[j];
               shapeFunction[ife][0]= 0.25*(1-y1)*(1-y2);
               shapeFunction[ife][1]= 0.25*(1+y1)*(1-y2);
               shapeFunction[ife][2] = 0.25*(1+y1)*(1+y2);
               shapeFunction[ife][3] = 0.25*(1-y1)*(1+y2);

               dndy1GaussPts[ife][0] = -0.25*(1-y2);
               dndy1GaussPts[ife][1] =  0.25*(1-y2);
               dndy1GaussPts[ife][2] =  0.25*(1+y2);
               dndy1GaussPts[ife][3] = -0.25*(1+y2);

               dndy2GaussPts[ife][0] = -0.25*(1-y1);
               dndy2GaussPts[ife][1] = -0.25*(1+y1);
               dndy2GaussPts[ife][2] =  0.25*(1+y1);
               dndy2GaussPts[ife][3] =  0.25*(1-y1);

               totalGaussWeight[ife] = gaussWeight[i]*gaussWeight[j];
               ife++;
            }
         }
         break;
      case 8:
         for ( i=0; i<numberGaussPoints; i++)
         {
            for ( j=0;j<numberGaussPoints;j++)
            {
               y1 = gaussPointY[i];
               y2 = gaussPointY[j];
               shapeFunction[ife][0] = 0.25*(1-y1)*(1-y2)*(-y1-y2-1);
               shapeFunction[ife][1] = 0.25*(1+y1)*(1-y2)*(y1-y2-1);
               shapeFunction[ife][2] = 0.25*(1+y1)*(1+y2)*(y1+y2-1);
               shapeFunction[ife][3] = 0.25*(1-y1)*(1+y2)*(-y1+y2-1);
               shapeFunction[ife][4] = 0.5*(1-y1*y1)*(1-y2);
               shapeFunction[ife][5] = 0.5*(1-y2*y2)*(1+y1);
               shapeFunction[ife][6] = 0.5*(1-y1*y1)*(1+y2);
               shapeFunction[ife][7] = 0.5*(1-y2*y2)*(1-y1);


               dndy1GaussPts[ife][0] =  0.25*(1-y2)*(2.0*y1+y2);
               dndy1GaussPts[ife][1] =  0.25*(1-y2)*(2.0*y1-y2);
               dndy1GaussPts[ife][2] =  0.25*(1+y2)*(2.0*y1+y2);
               dndy1GaussPts[ife][3] =  0.25*(1+y2)*(2.0*y1-y2);

               dndy1GaussPts[ife][4] = -y1*(1-y2);
               dndy1GaussPts[ife][5] =  0.5*(1-y2*y2);
               dndy1GaussPts[ife][6] = -y1*(1+y2);
               dndy1GaussPts[ife][7] = -0.5*(1-y2*y2);

               dndy2GaussPts[ife][0] =  0.25*(1-y1)*(2.0*y2+y1);
               dndy2GaussPts[ife][1] =  0.25*(1+y1)*(2.0*y2-y1);
               dndy2GaussPts[ife][2] =  0.25*(1+y1)*(2.0*y2+y1);
               dndy2GaussPts[ife][3] =  0.25*(1-y1)*(2.0*y2-y1);

               dndy2GaussPts[ife][4] = -0.5*(1-y1*y1);
               dndy2GaussPts[ife][5] = -y2*(1+y1);
               dndy2GaussPts[ife][6] =  0.5*(1-y1*y1);
               dndy2GaussPts[ife][7] = -y2*(1-y1);

               totalGaussWeight[ife] = gaussWeight[i]*gaussWeight[j];
               ife++;
            }
         }
         break;
   }
}

void GaussIntegration::calculate_shape_function_3d_hex()
{
   int ife=0, i, j, k, node_id;
   double y1,y2, y3, sign_node_y1, sign_node_y2, sign_node_y3;
   double y1_term, y2_term, y3_term, y123_temp;

   get_gauss_pts_and_weight();

    switch( numberNodes )
    {
    case 8:
       for ( i=0; i<numberGaussPoints; i++)
       {
          for ( j=0;j<numberGaussPoints;j++)
          {
             for ( k=0;k<numberGaussPoints;k++)
             {
                y1 = gaussPointY[i];
                y2 = gaussPointY[j];
                y3 = gaussPointY[k];

                for ( node_id =0; node_id <numberNodes; node_id++)
                {
                   get_signs_for_node_local_coord_hex(node_id, sign_node_y1, 
                      sign_node_y2, sign_node_y3);
   
                   y1_term = 1+sign_node_y1*y1;
                   y2_term = 1+sign_node_y2*y2;
                   y3_term = 1+sign_node_y3*y3;

                   shapeFunction[ife][node_id] = 0.125*y1_term
                      *y2_term*y3_term;
                   dndy1GaussPts[ife][node_id] = 0.125*sign_node_y1
                      *y2_term*y3_term;
                   dndy2GaussPts[ife][node_id] = 0.125*sign_node_y2
                      *y1_term*y3_term;
                   dndy3GaussPts[ife][node_id] = 0.125*sign_node_y3
                      *y1_term*y2_term;
                }
                totalGaussWeight[ife] = gaussWeight[i]*gaussWeight[j]*gaussWeight[k];
                ife++;
             }
          }
       }
       break;
    case 20:
       for ( i=0; i<numberGaussPoints; i++)
       {
          for ( j=0;j<numberGaussPoints;j++)
          {
             for ( k=0;k<numberGaussPoints;k++)
             {
                y1 = gaussPointY[i];
                y2 = gaussPointY[j];
                y3 = gaussPointY[k];

                for ( node_id =0; node_id <numberNodes; node_id++)
                {
                   get_signs_for_node_local_coord_hex(node_id, sign_node_y1, 
                      sign_node_y2, sign_node_y3);
                   
                   y1_term = 1+sign_node_y1*y1;
                   y2_term = 1+sign_node_y2*y2;
                   y3_term = 1+sign_node_y3*y3;
                   y123_temp = sign_node_y1*y1+sign_node_y2*y2+sign_node_y3*y3-2.;

                   switch (node_id)
                   {
                   case 0: case 1: case 2: case 3:
                   case 4: case 5: case 6: case 7:
                      {
                         shapeFunction[ife][node_id] = 0.125*y1_term
                            *y2_term*y3_term
                            *y123_temp;
                         dndy1GaussPts[ife][node_id] = 0.125*sign_node_y1
                            *y123_temp
                            *y2_term*y3_term
                            +0.125*y1_term
                            *y2_term*y3_term*sign_node_y1;
                         dndy2GaussPts[ife][node_id] = 0.125*sign_node_y2
                            *y1_term*y3_term
                            *y123_temp
                            +0.125*y1_term
                            *y2_term*y3_term*sign_node_y2;
                         dndy3GaussPts[ife][node_id] = 0.125*sign_node_y3
                            *y1_term*y2_term
                            *y123_temp
                            +0.125*y1_term
                            *y2_term*y3_term*sign_node_y3;
                         break;
                      }
                   case 8: case 10: case 16: case 18:
                      {
                         shapeFunction[ife][node_id] = 0.25*(1-y1*y1)
                            *y2_term*y3_term;
                         dndy1GaussPts[ife][node_id] = -0.5*y1
                            *y2_term*y3_term;
                         dndy2GaussPts[ife][node_id] = 0.25*(1-y1*y1)
                            *sign_node_y2*y3_term;
                         dndy3GaussPts[ife][node_id] = 0.25*(1-y1*y1)
                            *y2_term*sign_node_y3;
                         break;
                      }
                   case 9: case 11: case 17: case 19:
                      {
                         shapeFunction[ife][node_id] = 0.25*(1-y2*y2)
                            *y1_term*y3_term;
                         dndy1GaussPts[ife][node_id] = 0.25*(1-y2*y2)
                            *sign_node_y1*y3_term;
                         dndy2GaussPts[ife][node_id] = -0.5*y2
                            *y1_term*y3_term;
                         dndy3GaussPts[ife][node_id] = 0.25*(1-y2*y2)
                            *y1_term*sign_node_y3;
                         break;
                      }
                   case 12: case 13: case 14: case 15:
                      {
                         shapeFunction[ife][node_id] = 0.25*(1-y3*y3)
                            *y1_term*y2_term;
                         dndy1GaussPts[ife][node_id] = 0.25*(1-y3*y3)
                            *sign_node_y1*y2_term;
                         dndy2GaussPts[ife][node_id] = 0.25*(1-y3*y3)
                            *y1_term*sign_node_y2;
                         dndy3GaussPts[ife][node_id] = -0.5*y3
                            *y1_term*y2_term;
                         break;
                      }
                   }
                   
                }
                totalGaussWeight[ife] = gaussWeight[i]*gaussWeight[j]*gaussWeight[k];
                ife++;
             }
          }
       }
       break;

   }
}

void GaussIntegration::calculate_derivative_at_nodes(double dndy1_at_nodes[][maxNumberNodes], 
                                                     double dndy2_at_nodes[][maxNumberNodes])
{
   double y1=0., y2=0.;
   int i;
   for(i=0;i<numberNodes; i++) 
   {
      switch( i )
      {
         case 0:
            y1 = -1.;
            y2 = -1.;
            break;
         case 1:
            y1 = 1.;
            y2 = -1.;
            break;
         case 2:
            y1 = 1.;
            y2 = 1.;
            break;
         case 3:
            y1 = -1.;
            y2 = 1.;
            break;

         // midside nodes if there is any

         case 4:
            y1 = 0. ;
            y2 = -1. ;
            break;

         case 5:
            y1 = 1. ;
            y2 = 0. ;
            break;

         case 6:
            y1 = 0. ;
            y2 = 1. ;
            break;

         case 7:
            y1 = -1. ;
            y2 = 0. ;
            break;

      }

      switch( numberNodes )
      {
         case 4:
            //dn_i/dy1 evaluated at node i
            dndy1_at_nodes[i][0] = -0.25*(1-y2);
            dndy1_at_nodes[i][1] = 0.25*(1-y2);
            dndy1_at_nodes[i][2] = 0.25*(1+y2);
            dndy1_at_nodes[i][3] = -0.25*(1+y2);

            // dn_i/dy2 evaluated at node i
            dndy2_at_nodes[i][0] = -0.25*(1-y1);
            dndy2_at_nodes[i][1] = -0.25*(1+y1);
            dndy2_at_nodes[i][2] = 0.25*(1+y1);
            dndy2_at_nodes[i][3] = 0.25*(1-y1);
            break;

         case 8:

            dndy1_at_nodes[i][0]  =  0.25*(1-y2)*(2.0*y1+y2);
            dndy1_at_nodes[i][1]  =  0.25*(1-y2)*(2.0*y1-y2);
            dndy1_at_nodes[i][2]  =  0.25*(1+y2)*(2.0*y1+y2);
            dndy1_at_nodes[i][3]  =  0.25*(1+y2)*(2.0*y1-y2);

            dndy1_at_nodes[i][4]  = -y1*(1-y2);
            dndy1_at_nodes[i][5]  =  0.5*(1-y2*y2);
            dndy1_at_nodes[i][6]  = -y1*(1+y2);
            dndy1_at_nodes[i][7]  = -0.5*(1-y2*y2);

            dndy2_at_nodes[i][0]  =  0.25*(1-y1)*(2.0*y2+y1);
            dndy2_at_nodes[i][1]  =  0.25*(1+y1)*(2.0*y2-y1);
            dndy2_at_nodes[i][2]  =  0.25*(1+y1)*(2.0*y2+y1);
            dndy2_at_nodes[i][3]  =  0.25*(1-y1)*(2.0*y2-y1);

            dndy2_at_nodes[i][4] = -0.5*(1-y1*y1);
            dndy2_at_nodes[i][5] = -y2*(1+y1);
            dndy2_at_nodes[i][6] =  0.5*(1-y1*y1);
            dndy2_at_nodes[i][7] = -y2*(1-y1);
            break;
         }
   }
}

void GaussIntegration::calculate_derivative_at_nodes_3d(double dndy1_at_nodes[][maxNumberNodes],
                                                        double dndy2_at_nodes[][maxNumberNodes],
                                                        double dndy3_at_nodes[][maxNumberNodes])
{
   double y1, y2, y3,sign_node_y1,sign_node_y2,sign_node_y3 ;
   double y1_term, y2_term, y3_term, y123_temp;
   int node_id, node_id_2;
   for(node_id=0;node_id<numberNodes; node_id++) 
   {
      get_signs_for_node_local_coord_hex(node_id, y1,y2,y3);
      

      switch( numberNodes )
      {
      case 8:
         for ( node_id_2 =0; node_id_2 <numberNodes; node_id_2++)
         {
            get_signs_for_node_local_coord_hex(node_id_2, sign_node_y1,
               sign_node_y2,sign_node_y3);
            y1_term = 1+sign_node_y1*y1;
            y2_term = 1+sign_node_y2*y2;
            y3_term = 1+sign_node_y3*y3;

            dndy1_at_nodes[node_id][node_id_2] = 0.125*sign_node_y1
                                                 *y2_term*y3_term;

            dndy2_at_nodes[node_id][node_id_2] = 0.125*sign_node_y2
                                                 *y1_term*y3_term;

            dndy3_at_nodes[node_id][node_id_2] = 0.125*sign_node_y3
                                                 *y1_term*y2_term;
         }
         break;
      case 20:
         for ( node_id_2 =0; node_id_2 <numberNodes; node_id_2++)
         {
            get_signs_for_node_local_coord_hex(node_id_2, sign_node_y1,
               sign_node_y2,sign_node_y3);

            y1_term = 1+sign_node_y1*y1;
            y2_term = 1+sign_node_y2*y2;
            y3_term = 1+sign_node_y3*y3;
            y123_temp = sign_node_y1*y1+sign_node_y2*y2+sign_node_y3*y3-2.;
            switch (node_id_2)
            {
               case 0: case 1: case 2: case 3:
               case 4: case 5: case 6: case 7:
                  {
                     dndy1_at_nodes[node_id][node_id_2] = 0.125*sign_node_y1
                        *y2_term*y3_term
                        *y123_temp
                        +0.125*y1_term
                        *y2_term*y3_term*sign_node_y1;
                     dndy2_at_nodes[node_id][node_id_2] = 0.125*sign_node_y2
                        *y1_term*y3_term
                        *y123_temp
                        +0.125*y1_term
                        *y2_term*y3_term*sign_node_y2;
                     dndy3_at_nodes[node_id][node_id_2] = 0.125*sign_node_y3
                        *y1_term*y2_term
                        *y123_temp
                        +0.125*y1_term
                        *y2_term*y3_term*sign_node_y3;
                     break;
                  }
               case 8: case 10: case 16: case 18:
                  {
                     dndy1_at_nodes[node_id][node_id_2] = -0.5*y1
                        *y2_term*y3_term;
                     dndy2_at_nodes[node_id][node_id_2] = 0.25*(1-y1*y1)
                        *sign_node_y2*y3_term;
                     dndy3_at_nodes[node_id][node_id_2] = 0.25*(1-y1*y1)
                        *y2_term*sign_node_y3;
                     break;
                  }
               case 9: case 11: case 17: case 19:
                  {
                     dndy1_at_nodes[node_id][node_id_2] = 0.25*(1-y2*y2)
                        *sign_node_y1*y3_term;
                     dndy2_at_nodes[node_id][node_id_2] = -0.5*y2
                        *y1_term*y3_term;
                     dndy3_at_nodes[node_id][node_id_2] = 0.25*(1-y2*y2)
                        *y1_term*sign_node_y3;
                     break;
                  }
               case 12: case 13: case 14: case 15:
                  {
                     dndy1_at_nodes[node_id][node_id_2] = 0.25*(1-y3*y3)
                        *sign_node_y1*y2_term;
                     dndy2_at_nodes[node_id][node_id_2] = 0.25*(1-y3*y3)
                        *y1_term*sign_node_y2;
                     dndy3_at_nodes[node_id][node_id_2] = -0.5*y3
                        *y1_term*y2_term;
                     break;
                  }
            }
         }
         break;
         
      }
   }
}



void GaussIntegration::get_signs_for_node_local_coord_hex(int node_id, double &sign_node_y1, double &sign_node_y2, double &sign_node_y3)
{
   switch (node_id)
   {
   case 0:
      sign_node_y1 = -1.;
      sign_node_y2 = -1.;
      sign_node_y3 = -1.;
      break;
   case 1:
      sign_node_y1 = 1.;
      sign_node_y2 = -1.;
      sign_node_y3 = -1.;
      break;
   case 2:
      sign_node_y1 = 1.;
      sign_node_y2 = 1.;
      sign_node_y3 = -1.;
      break;
   case 3:
      sign_node_y1 = -1.;
      sign_node_y2 = 1.;
      sign_node_y3 = -1.;
      break;
   case 4:
      sign_node_y1 = -1.;
      sign_node_y2 = -1.;
      sign_node_y3 = 1.;
      break;
   case 5:
      sign_node_y1 = 1.;
      sign_node_y2 = -1.;
      sign_node_y3 = 1.;
      break;
   case 6:
      sign_node_y1 = 1.;
      sign_node_y2 = 1.;
      sign_node_y3 = 1.;
      break;
   case 7:
      sign_node_y1 = -1.;
      sign_node_y2 = 1.;
      sign_node_y3 = 1.;
      break;
   case 8:
      sign_node_y1 = 0;
      sign_node_y2 = -1.;
      sign_node_y3 = -1.;
      break;
   case 9:
      sign_node_y1 = 1.;
      sign_node_y2 = 0;
      sign_node_y3 = -1.;
      break;
   case 10:
      sign_node_y1 = 0;
      sign_node_y2 = 1.;
      sign_node_y3 = -1.;
      break;
   case 11:
      sign_node_y1 = -1.;
      sign_node_y2 = 0.;
      sign_node_y3 = -1.;
      break;
   case 12:
      sign_node_y1 = -1.;
      sign_node_y2 = -1.;
      sign_node_y3 = 0.;
      break;
   case 13:
      sign_node_y1 = 1.;
      sign_node_y2 = -1.;
      sign_node_y3 = 0.;
      break;
   case 14:
      sign_node_y1 = 1.;
      sign_node_y2 = 1.;
      sign_node_y3 = 0.;
      break;
   case 15:
      sign_node_y1 = -1.;
      sign_node_y2 = 1.;
      sign_node_y3 = 0.;
      break;
   case 16:
      sign_node_y1 = 0;
      sign_node_y2 = -1.;
      sign_node_y3 = 1.;
      break;
   case 17:
      sign_node_y1 = 1.;
      sign_node_y2 = 0;
      sign_node_y3 = 1.;
      break;
   case 18:
      sign_node_y1 = 0;
      sign_node_y2 = 1.;
      sign_node_y3 = 1.;
      break;
   case 19:
      sign_node_y1 = -1.;
      sign_node_y2 = 0.;
      sign_node_y3 = 1.;
      break;
   default:
      // Should not be possible to get here, but if we do, at least results will be consistent, not random
      sign_node_y1 = 0.;
      sign_node_y2 = 0.;
      sign_node_y3 = 0.;
      break;
   }
}

void GaussIntegration::get_tri_rule_pts_and_weight()
{
   // get triangular rule integration points and weight

   switch( numberGaussPoints )
   {
      case 6:
         y1Area[0] = 0.09157621;
         y2Area[0] = 0.09157621;

         y1Area[1] = 0.09157621;
         y2Area[1] = 0.8168476;

         y1Area[2] = 0.8168476;
         y2Area[2] = 0.09157621;

         y1Area[3] = 0.4459485;
         y2Area[3] = 0.4459485;

         y1Area[4] = 0.4459485;
         y2Area[4] = 0.1081030;

         y1Area[5] = 0.1081030;
         y2Area[5] = 0.4459485;

         int i;
         for (i=0;i<3;i++)
         {
            totalGaussWeight[i] = 0.06348067;
         }
         for (i=3;i<6;i++)
         {
            totalGaussWeight[i] = 0.1289694;
         }   
         break;
   }
}

void GaussIntegration::calculate_shape_function_2d_tri()
{
  int ife;
  double y1,y2, y3;
  get_tri_rule_pts_and_weight();

  for (ife=0; ife<totalNumberGaussPts; ife++)
    {
      y1 =  y1Area[ife];
      y2 =  y2Area[ife];
      y3 = 1.0 -y1 -y2;

      shapeFunction[ife][0] = y1*(2.*y1-1.);
      shapeFunction[ife][1] = y2*(2.*y2-1.);
      shapeFunction[ife][2] = y3*(2.*y3-1.);

      shapeFunction[ife][3] = 4.*y1*y2;
      shapeFunction[ife][4] = 4.*y2*y3;
      shapeFunction[ife][5] = 4.*y1*y3;
      

      dndy1GaussPts[ife][0] = 4*y1-1.;
      dndy1GaussPts[ife][1] = 0;
      dndy1GaussPts[ife][2] = 1-4.*y3;
      
      dndy1GaussPts[ife][3] = 4.*y2;
      dndy1GaussPts[ife][4] = -4.*y2;
      dndy1GaussPts[ife][5] = 4.*(1-2*y1-y2);

      dndy2GaussPts[ife][0] = 0.0;
      dndy2GaussPts[ife][1] = 4.*y2-1.;
      dndy2GaussPts[ife][2] = 1-4.*y3;
      
      dndy2GaussPts[ife][3] = 4.*y1;
      dndy2GaussPts[ife][4] = 4.*(1-y1-2.*y2);
      dndy2GaussPts[ife][5] = -4.*y1;
    }
}


void GaussIntegration::calculate_derivative_at_nodes_2d_tri(double dndy1_at_nodes[][maxNumberNodes],
                                                            double dndy2_at_nodes[][maxNumberNodes])
{
   double y1=0., y2=0., y3;
   int i;
   for(i=0;i<numberNodes; i++) 
   {
      switch( i )
      {
      case 0:
         y1 = 1.;
         y2 = 0.;
         break;
      case 1:
         y1 = 0.;
         y2 = 1.;
         break;
      case 2:
         y1 = 0.;
         y2 = 0.;
         break;
      case 3:
         y1 = 0.5;
         y2 = 0.5;
         break;
      case 4:
         y1 = 0.;
         y2 = 0.5;
         break;
      case 5:
         y1 = 0.5;
         y2 = 0.0;
         break;
      }

      y3 = 1. -y1-y2;

      dndy1_at_nodes[i][0] = 4*y1-1.;
      dndy1_at_nodes[i][1]= 0;
      dndy1_at_nodes[i][2] = 1-4.*y3;
      
      dndy1_at_nodes[i][3] = 4.*y2;
      dndy1_at_nodes[i][4] = -4.*y2;
      dndy1_at_nodes[i][5] = 4.*(1-2*y1-y2);

      dndy2_at_nodes[i][0] = 0.0;
      dndy2_at_nodes[i][1] = 4.*y2-1.;
      dndy2_at_nodes[i][2] = 1-4.*y3;
      
      dndy2_at_nodes[i][3] = 4.*y1;
      dndy2_at_nodes[i][4] = 4.*(1-y1-2.*y2);
      dndy2_at_nodes[i][5] = -4.*y1;
   }
}
void GaussIntegration::get_tet_rule_pts_and_weight()
{
   // get tetrahedron rule integration points and weight

   double a, b;
   switch( numberGaussPoints )
   {
   case 1:
      // 1 integration point formula, degree of precision 1
      y1Volume[0] = 0.25;
      y2Volume[0] = 0.25;
      y3Volume[0] = 0.25;
      y4Volume[0] = 0.25;
      totalGaussWeight[0] = 1.;
      break;
   case 4:
      // 4 integration points formula, degree of precision 2
      a = 0.58541020;
      b = 0.13819660;

      y1Volume[0] = a;
      y2Volume[0] = b;
      y3Volume[0] = b;
      y4Volume[0] = b;

      y1Volume[1] = b;
      y2Volume[1] = a;
      y3Volume[1] = b;
      y4Volume[1] = b;

      y1Volume[2] = b;
      y2Volume[2] = b;
      y3Volume[2] = a;
      y4Volume[2] = b;

      y1Volume[3] = b;
      y2Volume[3] = b;
      y3Volume[3] = b;
      y4Volume[3] = a;

      int i;
      for (i=0;i<4;i++)
      {
         totalGaussWeight[i] = 0.25;
       }
      break;
   }
}

void GaussIntegration::calculate_shape_function_3d_tet()
{
   int ife;
   double y1,y2, y3, y4;
   get_tet_rule_pts_and_weight();

   switch (numberNodes)
   {
   case 10: // 10 nodes quadratic tet
      {
         for (ife=0; ife<totalNumberGaussPts; ife++)
         {
            // y1,y2,y3,y4 are the volume coordinates
            y1 =  y1Volume[ife];
            y2 =  y2Volume[ife];
            y3 =  y3Volume[ife];
            y4 =  y4Volume[ife];

            // shape function is the same as in ABAQUS
            // it is different from that in all the FEA book
            // in which node is the first node
            // here at node 1 y4=1
            shapeFunction[ife][0] = y4*(2.*y4-1.);
            shapeFunction[ife][1] = y1*(2.*y1-1.);
            shapeFunction[ife][2] = y2*(2.*y2-1.);
            shapeFunction[ife][3] = y3*(2.*y3-1.);

            shapeFunction[ife][4] = 4.*y1*y4;
            shapeFunction[ife][5] = 4.*y1*y2;            
            shapeFunction[ife][6] = 4.*y2*y4;
            shapeFunction[ife][7] = 4.*y3*y4;
            shapeFunction[ife][8] = 4.*y1*y3;
            shapeFunction[ife][9] = 4.*y2*y3;

            dndy1GaussPts[ife][0] = 1-4*y4;
            dndy1GaussPts[ife][1] = 4*y1-1.;
            dndy1GaussPts[ife][2] = 0;
            dndy1GaussPts[ife][3] = 0;
            
            dndy1GaussPts[ife][4] = 4.*(y4-y1);
            dndy1GaussPts[ife][5] = 4.*y2;
            dndy1GaussPts[ife][6] = -4.*y2;
            dndy1GaussPts[ife][7] = -4.*y3;    
            dndy1GaussPts[ife][8] = 4.*y3;
            dndy1GaussPts[ife][9] = 0;
            
            dndy2GaussPts[ife][0] = 1-4*y4;
            dndy2GaussPts[ife][1] = 0;
            dndy2GaussPts[ife][2] = 4.*y2-1.;
            dndy2GaussPts[ife][3] = 0;

            dndy2GaussPts[ife][4] = -4.*y1; 
            dndy2GaussPts[ife][5] = 4.*y1;       
            dndy2GaussPts[ife][6] = 4.*(y4-y2);
            dndy2GaussPts[ife][7] = -4.*y3;
            dndy2GaussPts[ife][8] = 0.;
            dndy2GaussPts[ife][9] = 4.*y3;

            dndy3GaussPts[ife][0] = 1-4*y4;
            dndy3GaussPts[ife][1] = 0;
            dndy3GaussPts[ife][2] = 0;
            dndy3GaussPts[ife][3] = 4.*y3-1.;
            
            dndy3GaussPts[ife][4] = -4.*y1;
            dndy3GaussPts[ife][5] = 0;     
            dndy3GaussPts[ife][6] = -4.*y2;
            dndy3GaussPts[ife][7] = 4.*(y4-y3);
            dndy3GaussPts[ife][8] = 4.*y1;
            dndy3GaussPts[ife][9] = 4.*y2;
         }
         break;
      }
   case 4: // four node linear tet for debug purpose
      {
         for (ife=0; ife<totalNumberGaussPts; ife++)
         {
            y1 =  y1Volume[ife];
            y2 =  y2Volume[ife];
            y3 =  y3Volume[ife];
            y4 =  y4Volume[ife];

            shapeFunction[ife][0] = y4;
            shapeFunction[ife][1] = y1;
            shapeFunction[ife][2] = y2;
            shapeFunction[ife][3] = y3;

            dndy1GaussPts[ife][0] = -1.;
            dndy1GaussPts[ife][1] = 1;
            dndy1GaussPts[ife][2] = 0;
            dndy1GaussPts[ife][3] = 0;

            dndy2GaussPts[ife][0] = -1.;
            dndy2GaussPts[ife][1] = 0;
            dndy2GaussPts[ife][2] = 1;
            dndy2GaussPts[ife][3] = 0;

            dndy3GaussPts[ife][0] = -1.;
            dndy3GaussPts[ife][1] = 0;
            dndy3GaussPts[ife][2] = 0;
            dndy3GaussPts[ife][3] = 1;
            
         }
         break;
      }
   }

}

void GaussIntegration::calculate_derivative_at_nodes_3d_tet(double dndy1_at_nodes[][maxNumberNodes],
                                                            double dndy2_at_nodes[][maxNumberNodes],
                                                            double dndy3_at_nodes[][maxNumberNodes])
{
   double y1, y2, y3, y4;
   int i;

   switch (numberNodes)
   {
   case 10:
      {
         for(i=0;i<numberNodes; i++) 
         {
            get_node_local_coord_tet(i, y1, y2, y3, y4);

            dndy1_at_nodes[i][0] = 1-4*y4;
            dndy1_at_nodes[i][1] = 4*y1-1.;
            dndy1_at_nodes[i][2] = 0;
            dndy1_at_nodes[i][3] = 0;

            dndy1_at_nodes[i][4] = 4.*(y4-y1);
            dndy1_at_nodes[i][5] = 4.*y2;
            dndy1_at_nodes[i][6] = -4.*y2;
            dndy1_at_nodes[i][7] = -4.*y3;
            dndy1_at_nodes[i][8] = 4.*y3;
            dndy1_at_nodes[i][9] = 0;

            dndy2_at_nodes[i][0] = 1-4*y4;
            dndy2_at_nodes[i][1] = 0;
            dndy2_at_nodes[i][2] = 4.*y2-1.;
            dndy2_at_nodes[i][3] = 0;
            dndy2_at_nodes[i][4] = -4.*y1;
            dndy2_at_nodes[i][5] = 4.*y1;
            dndy2_at_nodes[i][6] = 4.*(y4-y2);
            dndy2_at_nodes[i][7] = -4.*y3;
            dndy2_at_nodes[i][8] = 0.;
            dndy2_at_nodes[i][9] = 4.*y3;

            dndy3_at_nodes[i][0] = 1-4*y4;
            dndy3_at_nodes[i][1] = 0;
            dndy3_at_nodes[i][2] = 0;
            dndy3_at_nodes[i][3] = 4.*y3-1.;

            dndy3_at_nodes[i][4] = -4.*y1;
            dndy3_at_nodes[i][5] = 0;
            dndy3_at_nodes[i][6] = -4.*y2;
            dndy3_at_nodes[i][7] = 4.*(y4-y3);
            dndy3_at_nodes[i][8] = 4.*y1;
            dndy3_at_nodes[i][9] = 4.*y2;
         }
         break;
      }
   case 4:
      {
         for(i=0;i<numberNodes; i++)
         {
            get_node_local_coord_tet(i, y1, y2, y3, y4);
            dndy1_at_nodes[i][0] = -1.;
            dndy1_at_nodes[i][1] = 1;
            dndy1_at_nodes[i][2] = 0;
            dndy1_at_nodes[i][3] = 0;

            dndy2_at_nodes[i][0] = -1.;
            dndy2_at_nodes[i][1] = 0;
            dndy2_at_nodes[i][2] = 1;
            dndy2_at_nodes[i][3] = 0;
 
            dndy3_at_nodes[i][0] = -1.;
            dndy3_at_nodes[i][1] = 0;
            dndy3_at_nodes[i][2] = 0;
            dndy3_at_nodes[i][3] = 1;
            
         }
         break;
      }
   }
}
      

void GaussIntegration::get_node_local_coord_tet(int node_id, double &y1, double &y2, 
                                                double &y3, double &y4)
{
   switch( node_id )
   {
   case 0:
      y1 = 0.;
      y2 = 0.;
      y3 = 0.;
      y4 = 1.;
      break;
   case 1:
      y1 = 1.;
      y2 = 0.;
      y3 = 0.;
      y4 = 0.;
      break;
   case 2:
      y1 = 0.;
      y2 = 1.;
      y3 = 0.;
      y4 = 0.;
      break;
   case 3:
      y1 = 0.;
      y2 = 0.;
      y3 = 1.;
      y4 = 0.;
      break;
   case 4:
      y1 = 0.5;
      y2 = 0.;
      y3 = 0.;
      y4 = 0.5;
      break;
   case 5:
      y1 = 0.5;
      y2 = 0.5;
      y3 = 0.;
      y4 = 0.;
      break;
   case 6:
      y1 = 0.;
      y2 = 0.5;
      y3 = 0.;
      y4 = 0.5;
      break;
   case 7:
      y1 = 0.;
      y2 = 0.0;
      y3 = 0.5;
      y4 = 0.5;
      break;
   case 8:
      y1 = 0.5;
      y2 = 0.;
      y3 = 0.5;
      y4 = 0.0;
      break;
   case 9:
      y1 = 0.;
      y2 = 0.5;
      y3 = 0.5;
      y4 = 0.;
      break;
   default:
      // Should not be possible to get here, but if we do, at least results will be consistent, not random
      y1 = 0.;
      y2 = 0.;
      y3 = 0.;
      y4 = 0.;
      break;
   }
}
