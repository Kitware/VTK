#include "vtkString.h"
#include "vtkObject.h"
#include "vtkArrayMap.txx"

int main()
{
  int error = 0;
  int cc;  
  char names[][10] = {
    "Andy",
    "Amy",
    "Berk",
    "Bill",
    "Brad",
    "Charles",
    "JoAnne",
    "Ken",
    "Lisa",
    "Sebastien",
    "Will"
  };
  
  vtkArrayMap<int,char*> *am 
    = vtkArrayMap<int,char*>::New();

  for ( cc = 9; cc >= 0; cc -- )
    {
    if ( am->SetItem(cc, names[cc]) != VTK_OK )
      {
      cout << "Problem adding item to the array map" << endl;
      error = 1;
      }
    }
  for ( cc = 0; cc < 10; cc ++ )
    {
    char *buffer = 0;
    if ( am->GetItem(cc, buffer) != VTK_OK)
      {
      cout << "Problem retrieving item from the array map" << endl;
      error = 1;
      }
    if ( !buffer || !vtkString::Equals(buffer, names[cc]) )
      {
      cout << "Retrieved string: " << (buffer?buffer:"") 
           << " is not the same as the"
        " one inserted" << endl;
      error = 1;
      }
    }

  // Try the iterator
  vtkArrayMap<int,char*>::IteratorType *nit = am->NewIterator();
  //cout << "Try iterator" << endl;
  nit->GoToFirstItem();
  while ( nit->IsDoneWithTraversal() != VTK_OK )
    {
    char* str = 0;
    int idx = 0;
    if ( nit->GetData(str) != VTK_OK )
      {
      cout << "Problem accessing data from iterator" << endl;
      error =1;
      }
    if ( nit->GetKey(idx) != VTK_OK )
      {
      cout << "Problem accessing key from iterator" << endl;
      error =1;     
      }
    //cout << "Item: " << idx << " = " << str << endl;
    nit->GoToNextItem();
    }
  nit->GoToLastItem();
  while ( nit->IsDoneWithTraversal() != VTK_OK )
    {
    char* str = 0;
    int idx = 0;
    if ( nit->GetData(str) != VTK_OK )
      {
      cout << "Problem accessing data from iterator" << endl;
      error =1;
      }
    if ( nit->GetKey(idx) != VTK_OK )
      {
      cout << "Problem accessing key from iterator" << endl;
      error =1;     
      }
    //cout << "Item: " << idx << " = " << str << endl;
    nit->GoToPreviousItem();
    }
  nit->Delete();
 
  am->Delete();

  vtkArrayMap<const char*,char*> *sam 
    = vtkArrayMap<const char*,char*>::New();
  for ( cc = 9; cc >= 0; cc -- )
    {
    char *nkey = vtkString::Duplicate(names[cc]);
    if ( sam->SetItem(nkey, names[cc]) != VTK_OK )
      {
      cout << "Problem adding item to the array map" << endl;
      error = 1;
      }
    delete [] nkey;
    }
  //sam->DebugList();
  for ( cc = 0; cc < 10; cc ++ )
    {
    char *buffer = 0;
    if ( sam->GetItem(names[cc], buffer) != VTK_OK)
      {
      cout << "Problem retrieving item from the array map" << endl;
      error = 1;
      }
    if ( !buffer )
      {
      cout << "Cannot access key: " << names[cc] << endl;
      }      
    else
      {
      if ( !vtkString::Equals(buffer, names[cc]) )
        {
        cout << "Retrieved string: " << (buffer?buffer:"") 
             << " is not the same as the"
          " one inserted" << endl;
        error  = 1;
        }
      }
    }
  char *buffer = 0;
  if ( sam->GetItem("Brad", buffer) != VTK_OK)
    {
    cout << "Problem retrieving item from the array map" << endl;
    error = 1;
    }
  if ( !buffer )
    {
    cout << "Cannot access key: " << names[cc] << endl;
    error  = 1;
    }      
  else
    {
    if ( !vtkString::Equals(buffer, "Brad") )
      {
      cout << "Retrieved string: " << (buffer?buffer:"") 
           << " is not the same as the"
           << " one inserted" << endl;
      error  = 1;
      }
    }
 
  sam->Delete();

  vtkArrayMap<const char*, vtkObject*> *soam 
    = vtkArrayMap<const char*, vtkObject*>::New();

  char name[20];
  for( cc = 0; cc < 10; cc ++ )
    {
    sprintf(name, "actor%02d", cc);
    vtkObject* actor = vtkObject::New();
    if ( soam->SetItem(name, actor) != VTK_OK )
      {
      cout << "Problem inserting item in the map, key: " 
           << name << " data: " << actor << endl;
      error  = 1;
      }

    vtkObject* nactor = 0;
    if ( soam->GetItem(name, nactor) != VTK_OK )
      {
      cout << "Problem accessing item in the map: " << name << endl;
      error  = 1;
      }
    if ( !nactor )
      {
      cout << "Item: " << name << " should not be null" << endl;
      error  = 1;
      }
    if ( nactor != actor )
      {
      cout << "Item: " << nactor << " at key: " << name 
           << " is not the same as: " << actor << endl;
      error  = 1;
      }
    
    actor->Delete();
    }

  soam->Delete();

  return error;
}

