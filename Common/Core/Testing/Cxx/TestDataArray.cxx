#include "vtkIntArray.h"
#include "vtkDoubleArray.h"

int TestDataArray(int,char *[])
{
  double range[2];
  vtkIntArray* array = vtkIntArray::New();
  array->GetRange( range, 0 );
  if ( range[0] != VTK_DOUBLE_MAX || range[1] != VTK_DOUBLE_MIN )
    {
    cerr
      << "Getting range of empty array failed, min: "
      << range[0] << " max: " << range[1] << "\n";
    array->Delete();
    return 1;
    }
  int cc;
  for ( cc = 0; cc < 10; cc ++ )
    {
    array->InsertNextTuple1(cc);
    }
  array->GetRange( range, 0 ); // Range is now 0-9. Used to check MTimes.
  array->RemoveFirstTuple();
  array->RemoveTuple(3);
  array->RemoveTuple(4);
  array->GetRange( range, 0 );
  if ( range[0] != 0 || range[1] != 9 )
    {
    cerr
      << "Getting range (" << range[0] << "-" << range[1]
      << ") of array not marked as modified caused recomputation of range!";
    array->Delete();
    return 1;
    }
  array->Modified(); // Now mark array so range gets recomputed
  array->GetRange( range, 0 );
  if ( range[0] != 1. || range[1] != 9. )
    {
    cerr
      << "Getting range of array {1,2,3,5,7,8,9} failed, min: "
      << range[0] << " max: " << range[1] << "\n";
    array->Delete();
    return 1;
    }
  array->RemoveLastTuple();
  array->Modified();
  array->GetRange( range, 0 );
  if ( range[0] != 1. || range[1] != 8. )
    {
    cerr
      << "Getting range of array {1,2,3,5,7,8} failed, min: "
      << range[0] << " max: " << range[1] << "\n";
    array->Delete();
    return 1;
    }
  int ca[] = { 1, 2, 3, 5, 7, 8 };
  cout << "Array:";
  for ( cc = 0; cc < array->GetNumberOfTuples(); ++cc )
    {
    if ( array->GetTuple1(cc) != ca[cc] )
      {
      cerr
        << "Problem with array: " << array->GetTuple1(cc)
        << " <> " << ca[cc] << endl;
      array->Delete();
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
    farray->InsertNextTuple3( cc + 0.125, cc + 0.250, cc + 0.375);
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
    fc[0] = ca[cc] + .125;
    fc[1] = ca[cc] + .250;
    fc[2] = ca[cc] + .375;
    for (int i = 0; i < 3; i++)
      {
      if (fa[i] != fc[i])
        {
        cerr << "Problem with array: " << fa[i] << " <> " << fc[i] << endl;
        farray->Delete();
        return 1;
        }
      }
    cout << " " << fa[0] << "," << fa[1] << "," << fa[2];
    }
  cout << endl;
  farray->Delete();
  return 0;
}
