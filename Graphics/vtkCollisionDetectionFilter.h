/*=========================================================================

  Program:   Visualization Toolkit
  Module:    vtkCollisionDetectionFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME vtkCollisionDetectionFilter - performs collision determination between two polyhedral surfaces
// .SECTION Description
// vtkCollisionDetectionFilter performs collision determination between two polyhedral surfaces using
// two instances of vtkOBBTree. Set the polydata inputs, the tolerance and transforms or matrices. If 
// CollisionMode is set to AllContacts, the Contacts output will be lines of contact.
// If CollisionMode is FirstContact or HalfContacts then the Contacts output will be vertices.  
// See below for an explanation of these options.
//
// This class can be used to clip one polydata surface with another, using the Contacts output as a loop
// set in vtkSelectPolyData

// .SECTION Caveats
// Currently only triangles are processed. Use vtkTriangleFilter to
// convert any strips or polygons to triangles.

// .SECTION Thanks
// Goodwin Lawlor <goodwin.lawlor@ucd.ie>, University College Dublin, who wrote this class.
// Thanks to Peter C. Everett <pce@world.std.com> for vtkOBBTree::IntersectWithOBBTree() in particular,
// and all those who contributed to vtkOBBTree in general.


// .SECTION See Also
// vtkTriangleFilter, vtkSelectPolyData, vtkOBBTree

#ifndef __vtkCollisionDetectionFilter_h
#define __vtkCollisionDetectionFilter_h

#include "vtkPolyDataAlgorithm.h"
#include "vtkLinearTransform.h"
#include "vtkIdTypeArray.h"
#include "vtkFieldData.h"

// Forward declarations
class vtkOBBTree;
class vtkPolyData;
class vtkPoints;
class vtkMatrix4x4;


class VTK_GRAPHICS_EXPORT vtkCollisionDetectionFilter : public vtkPolyDataAlgorithm
{
public:
  vtkTypeMacro(vtkCollisionDetectionFilter, vtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, vtkIndent indent);

//BTX
  enum CollisionModes
  {
    VTK_ALL_CONTACTS = 0,
    VTK_FIRST_CONTACT = 1,
    VTK_HALF_CONTACTS = 2 
  };
//ETX

  // Description:
  // Set the collision mode to VTK_ALL_CONTACTS to find all the contacting cell pairs with
  // two points per collision, or VTK_HALF_CONTACTS to find all the contacting cell pairs
  // with one point per collision, or VTK_FIRST_CONTACT to quickly find the first contact
  // point.
  vtkSetClampMacro(CollisionMode,int,VTK_ALL_CONTACTS,VTK_HALF_CONTACTS);
  vtkGetMacro(CollisionMode,int);
  void SetCollisionModeToAllContacts() {this->SetCollisionMode(VTK_ALL_CONTACTS);};
  void SetCollisionModeToFirstContact() {this->SetCollisionMode(VTK_FIRST_CONTACT);};
  void SetCollisionModeToHalfContacts() {this->SetCollisionMode(VTK_HALF_CONTACTS);};
  const char *GetCollisionModeAsString();

  // Description:
  // Constructs with initial values.
  static vtkCollisionDetectionFilter *New();

  // Description:
  // Intersect two polygons, return x1 and x2 as the twp points of intersection. If
  // CollisionMode = VTK_ALL_CONTACTS, both contact points are found. If 
  // CollisionMode = VTK_FIRST_CONTACT or VTK_HALF_CONTACTS, only
  // one contact point is found.
  int IntersectPolygonWithPolygon(int npts, double *pts, double bounds[6],
                                            int npts2, double *pts2, 
                                            double bounds2[6], double tol2,
                                            double x1[2], double x2[3],
                                            int CollisionMode);
  
  // Description:
  // Set and Get the input vtk polydata models
    void SetInput(int i, vtkPolyData *model);
    vtkPolyData *GetInput(int i);

      
  // Description:
  // Get an array of the contacting cells. This is a convenience method to access 
  // the "ContactCells" field array in outputs 0 and 1. These arrays index contacting
  // cells (eg) index 50 of array 0 points to a cell (triangle) which contacts/intersects
  // a cell at index 50 of array 1. This method is equivalent to 
  // GetOutput(i)->GetFieldData()->GetArray("ContactCells")
    vtkIdTypeArray *GetContactCells(int i);

  // Description:
  // Get the output with the points where the contacting cells intersect. This method is
  // is equivalent to GetOutputPort(2)/GetOutput(2) 
  vtkAlgorithmOutput *GetContactsOutputPort() {return this->GetOutputPort(2);}
  vtkPolyData *GetContactsOutput() {return this->GetOutput(2);}

  // Description:
  // Specify the transform object used to transform models. Alternatively, matrices
  // can be set instead.
  void SetTransform(int i, vtkLinearTransform *transform);
  vtkLinearTransform *GetTransform(int i) {return this->Transform[i];}
  
  // Description:
  // Specify the matrix object used to transform models.
  void SetMatrix(int i, vtkMatrix4x4 *matrix);
  vtkMatrix4x4 *GetMatrix(int i);
  
  //Description:
  // Set and Get the obb tolerance (absolute value, in world coords). Default is 0.001
  vtkSetMacro(BoxTolerance, float);
  vtkGetMacro(BoxTolerance, float);

  //Description:
  // Set and Get the cell tolerance (squared value). Default is 0.0
  vtkSetMacro(CellTolerance, double);
  vtkGetMacro(CellTolerance, double);
  
  //Description:
  // Set and Get the the flag to visualize the contact cells. If set the contacting cells
  // will be coloured from red through to blue, with collisions first determined coloured red.
  vtkSetMacro(GenerateScalars, int);
  vtkGetMacro(GenerateScalars, int);
  vtkBooleanMacro(GenerateScalars,int);
  
  //Description:
  // Get the number of contacting cell pairs
  int GetNumberOfContacts() 
    {return this->GetOutput(0)->GetFieldData()->GetArray("ContactCells")->GetNumberOfTuples();}
  
  //Description:
  // Get the number of box tests
  vtkGetMacro(NumberOfBoxTests, int); 
    
  //Description:
  // Set and Get the number of cells in each OBB. Default is 2
  vtkSetMacro(NumberOfCellsPerNode, int);
  vtkGetMacro(NumberOfCellsPerNode, int);
  
  //Description:
  // Set and Get the opacity of the polydata output when a collision takes place.
  // Default is 1.0
  vtkSetClampMacro(Opacity, float, 0.0, 1.0);
  vtkGetMacro(Opacity, float);

  // Description:
  // Return the MTime also considering the transform.
  unsigned long GetMTime();

protected:
  vtkCollisionDetectionFilter();
  ~vtkCollisionDetectionFilter();

  // Usual data generation method
  virtual int RequestData(vtkInformation *, vtkInformationVector **, vtkInformationVector *);
  
  vtkOBBTree *tree0;
  vtkOBBTree *tree1;

  vtkLinearTransform *Transform[2];
  vtkMatrix4x4 *Matrix[2];
  
  int NumberOfBoxTests;
  
  int NumberOfCellsPerNode;
  
  int GenerateScalars;
  
  float BoxTolerance;
  float CellTolerance;
  float Opacity;
  
  int CollisionMode;

private:  

  vtkCollisionDetectionFilter(const vtkCollisionDetectionFilter&);  // Not implemented.
  void operator=(const vtkCollisionDetectionFilter&);  // Not implemented.
};

//BTX

inline const char *vtkCollisionDetectionFilter::GetCollisionModeAsString(void)
{
  if ( this->CollisionMode == VTK_ALL_CONTACTS )
    {
    return (char *)"AllContacts";
    }
  else if (this->CollisionMode == VTK_FIRST_CONTACT)
    {
    return (char *)"FirstContact";
    }
  else
    {
    return (char *)"HalfContacts";
    }
}

//ETX
#endif
