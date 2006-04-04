#include "vtkIntArray.h"
#include "vtkDoubleArray.h"

int TestDataArray(int,char *[])
{
  vtkIntArray* array = vtkIntArray::New();
  int cc;
  for ( cc = 0; cc < 10; cc ++ )
    {
    array->InsertNextTuple1(cc);
    }
  array->RemoveFirstTuple();
  array->RemoveTuple(3);
  array->RemoveTuple(4);
  array->RemoveLastTuple();
  int ca[] = { 1, 2, 3, 5, 7, 8 };
  cout << "Array:";
  for ( cc = 0; cc < array->GetNumberOfTuples(); ++cc )
    {
    if ( array->GetTuple1(cc) != ca[cc] )
      {
      cerr << "Problem with array: " << array->GetTuple1(cc) << " <> " << ca[cc] << endl;
      return 1;
      }
    cout << " " << array->GetTuple1(cc);
    }
  cout << endl;
  array->Delete();

  vtkDoubleArray* farray = vtkDoubleArray::New();
  farray->SetNumberOfComponents(3);
  for ( cc = 0; cc < 10; cc ++ )
    {
    farray->InsertNextTuple3( cc + 0.1, cc + 0.2, cc + 0.3);
    }
  farray->RemoveFirstTuple();
  farray->RemoveTuple(3);
  farray->RemoveTuple(4);
  farray->RemoveLastTuple();
  cout << "Array:";
  for ( cc = 0; cc < farray->GetNumberOfTuples(); ++cc )
    {
    double* fa = farray->GetTuple3(cc);
    double fc[3];
    fc[0] = ca[cc] + .1;
    fc[1] = ca[cc] + .2;
    fc[2] = ca[cc] + .3;
    for (int i = 0; i < 3; i++)
      {
      if (fa[i] != fc[i])
        {
        cerr << "Problem with array: " << fa[i] << " <> " << fc[i] << endl;
        return 1;
        }
      }
    cout << " " << fa[0] << "," << fa[1] << "," << fa[2];
    }
  cout << endl;
  farray->Delete();
  return 0;
}
