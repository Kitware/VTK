#include "vtkIntArray.h"
#include "vtkDoubleArray.h"

int TestDataArrayComponentNames(int,char *[])
{
  vtkIntArray* array = vtkIntArray::New();
  array->SetNumberOfComponents( 2 );
  array->SetComponentName(0, "x" );
  array->SetComponentName(1, "y" );

  if ( strcmp(array->GetComponentName(0),"x") != 0 ||  strcmp(array->GetComponentName(1),"y") != 0 )
  {
    cerr
      << "Unable to store component names correctly";
    array->Delete();
    return 1;
  }

  if ( array->GetComponentName(-1) != NULL || array->GetComponentName(3) != NULL )
  {
    cerr
      << " Unable to return NULL when asking for a component name outside the the valid range";
    array->Delete();
    return 1;
  }

  //rename to a different string length
  array->SetComponentName(0, "a really long component name" );
  if ( strcmp(array->GetComponentName(0),"a really long component name") != 0  )
  {
    cerr
      << "Unable to rename the component name to a string of a different length";
    array->Delete();
    return 1;
  }

  array->Delete();

  vtkDoubleArray* farray = vtkDoubleArray::New();
  farray->SetComponentName(2,"z");
  farray->SetComponentName(1,"y");
  farray->SetComponentName(0,"x");
  farray->SetNumberOfComponents(3);
  for ( int i = 0; i < 10; i ++ )
  {
    farray->InsertNextTuple3( i + 0.1, i + 0.2, i + 0.3);
  }

  if ( strcmp(farray->GetComponentName(0),"x") != 0 ||
       strcmp(farray->GetComponentName(1),"y") != 0 ||
       strcmp(farray->GetComponentName(2),"z") != 0 )
  {
    cerr
      << "Unable to store component names correctly";
    farray->Delete();
    return 1;
  }

  //rename the components
  farray->SetComponentName(0,"a");
  farray->SetComponentName(1,"b");
  farray->SetComponentName(2,"c");
  if ( strcmp(farray->GetComponentName(0),"a") != 0  ||
        strcmp(farray->GetComponentName(1),"b") != 0  ||
        strcmp(farray->GetComponentName(2),"c") != 0  )
  {
    cerr
      << "Unable to rename component names correctly";
    farray->Delete();
    return 1;
  }

  //test edge cases
  farray->SetComponentName(-1, "invalid" );


  //Test Down cast
  vtkDoubleArray *downcast = vtkArrayDownCast<vtkDoubleArray>( farray );
  if (  strcmp(downcast->GetComponentName(0),"a") != 0||
        strcmp(downcast->GetComponentName(1),"b") != 0||
        strcmp(downcast->GetComponentName(2),"c") != 0)
  {
    cerr
      << "Unable to safe down cast";
    farray->Delete();
    return 1;
  }

  //DeepCopy
  vtkDoubleArray *deepCopy = vtkDoubleArray::New();
  deepCopy->SetComponentName(3,"s1");
  deepCopy->SetComponentName(2,"z");
  deepCopy->SetComponentName(1,"y");
  deepCopy->SetComponentName(0,"x");
  deepCopy->DeepCopy( farray );
  if ( strcmp(deepCopy->GetComponentName(0),"a") != 0||
        strcmp(deepCopy->GetComponentName(1),"b") != 0||
        strcmp(deepCopy->GetComponentName(2),"c") != 0)
  {
    cerr
      << "Deep Copy failed to overwrite old component names";
    deepCopy->Delete();
    downcast->Delete();
    return 1;
  }
  if ( deepCopy->GetComponentName(3) != NULL )
  {
    cerr
      << "Deep Copy failed to return null for a invalid component name";
    deepCopy->Delete();
    downcast->Delete();
    return 1;
  }

  downcast->Delete();

  //test to make sure that the deep copy still has valid strings
  if (  strcmp(deepCopy->GetComponentName(0),"a") != 0||
        strcmp(deepCopy->GetComponentName(1),"b") != 0||
        strcmp(deepCopy->GetComponentName(2),"c") != 0)
  {
    cerr
      << "Deep copy failed to copy component names, instead it made shallow copies";
    deepCopy->Delete();
    return 1;
  }

  deepCopy->Delete();

  //DeepCopy test 2 with NULL values
  farray = vtkDoubleArray::New();
  farray->SetNumberOfComponents(20);

  deepCopy = vtkDoubleArray::New();
  deepCopy->DeepCopy( farray );

  if ( deepCopy->GetComponentName(0) != NULL )
  {
    cerr
      << "Failed to deep copy with no names on the components";
    return 1;
  }

  deepCopy->Delete();
  farray->Delete();


  //test the copy components command
  vtkIntArray *source = vtkIntArray::New();
  source->SetComponentName(0,"x");
  source->SetComponentName(1,"y");
  source->SetComponentName(2,"z");

  vtkIntArray *dest = vtkIntArray::New();
  dest->SetComponentName(0,"a");
  dest->SetComponentName(1,"b");
  dest->SetComponentName(2,"c");

  dest->CopyComponentNames( source );
  if (  strcmp(dest->GetComponentName(0),"x") != 0||
        strcmp(dest->GetComponentName(1),"y") != 0||
        strcmp(dest->GetComponentName(2),"z") != 0)
  {
    cerr
      << "Copy Component Names failed to work correctly";
    dest->Delete();
    source->Delete();
    return 1;
  }

  dest->Delete();

  //test copy when dest has a null pointer
  dest = vtkIntArray::New();
  dest->CopyComponentNames( source );
  if (  strcmp(dest->GetComponentName(0),"x") != 0||
        strcmp(dest->GetComponentName(1),"y") != 0||
        strcmp(dest->GetComponentName(2),"z") != 0)
  {
    cerr
      << "Copy Component Names failed to work correctly";
    dest->Delete();
    source->Delete();
    return 1;
  }

  dest->Delete();

  //test copy when dest had more components
  dest = vtkIntArray::New();
  dest->SetComponentName(10,"10");
  dest->CopyComponentNames( source );
  if (  strcmp(dest->GetComponentName(0),"x") != 0||
        strcmp(dest->GetComponentName(1),"y") != 0||
        strcmp(dest->GetComponentName(2),"z") != 0)
  {
    cerr
      << "Copy Component Names failed to work correctly";
    dest->Delete();
    source->Delete();
    return 1;
  }



  dest->Delete();
  source->Delete();


  return 0;
}
