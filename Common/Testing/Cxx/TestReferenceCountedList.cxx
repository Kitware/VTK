#include "vtkVector.txx"
#include "vtkLinkedList.txx"
#include "vtkObject.h"
#include "vtkCallbackCommand.h"

#define C_ERROR(c) cout << "Container: " << c->GetClassName() << " "

int Count = 0;

void DeleteCommand( vtkObject* ,
                    unsigned long, 
                    void *, void * )
{
  ::Count --;
  //cout << "Delete object; " << ::Count << " left..." << endl;
}

template<class DType>
int TestList(DType* tlist, int count)
{
  vtkCallbackCommand *ccm = vtkCallbackCommand::New();
  ccm->SetCallback(&DeleteCommand);
  int cc;
  for ( cc = 0; cc < count; cc ++ )
    {
    vtkObject *act1 = vtkObject::New();
    //cout << "Append: " << act1 << endl;
    tlist->AppendItem(act1);
    act1->AddObserver(vtkCommand::DeleteEvent, ccm); ::Count ++;
    act1->Delete();
    }
  //tlist->DebugList();
  for ( cc = 0; cc < count; cc ++ )
    {
    vtkObject *act1 = vtkObject::New();
    //cout << "Prepend: " << act1 << endl;
    tlist->PrependItem(act1);
    act1->AddObserver(vtkCommand::DeleteEvent, ccm); ::Count ++;
    act1->Delete();
    }
  //tlist->DebugList();
  for ( cc = 0; cc < count; cc ++ )
    {
    vtkObject *act1 = vtkObject::New();
    //cout << "Insert: " << act1 << endl;
    tlist->InsertItem(cc, act1);
    act1->AddObserver(vtkCommand::DeleteEvent, ccm); ::Count ++;
    act1->Delete();
    }
  //tlist->DebugList();
  for ( cc = 0; cc < count; cc ++ )
    {
    //cout << "Remove item" << endl;
    tlist->RemoveItem(cc);
    }
  //tlist->DebugList();

  //cout << "Remove: " << tlist->GetNumberOfItems() << " items" << endl;
  tlist->RemoveAllItems();
   //tlist->DebugList();
  for ( cc = 0; cc < count; cc ++ )
    {
    vtkObject *act1 = vtkObject::New();
    //cout << "Prepend: " << act1 << endl;
    tlist->PrependItem(act1);
    act1->AddObserver(vtkCommand::DeleteEvent, ccm); ::Count ++;
    act1->Delete();
    }
  //tlist->DebugList();
  for ( cc = 0; cc < count; cc ++ )
    {
    vtkObject *act1 = vtkObject::New();
    //cout << "Insert: " << act1 << endl;
    tlist->InsertItem(cc, act1);
    act1->AddObserver(vtkCommand::DeleteEvent, ccm); ::Count ++;
    act1->Delete();
    }
  //tlist->DebugList();
  for ( cc = 0; cc < count; cc ++ )
    {
    //cout << "Remove item" << endl;
    tlist->RemoveItem(cc);
    } 
  //cout << "Remove: " << tlist->GetNumberOfItems() << " items" << endl;

  return 0;
}

int main()
{
  int res = 0;
  int count = 300;
  
  //cout << "Vector: " << endl;

  vtkVector<vtkObject*> *vv 
    = vtkVector<vtkObject*>::New();
  res += TestList(vv, count);
  vv->Delete();

  //cout << "Linked List: " << endl;
  vtkLinkedList<vtkObject*> *vl 
    = vtkLinkedList<vtkObject*>::New();
  res += TestList(vl, count);
  vl->Delete();

  if ( ::Count )
    {
    cout << "Looks like reference counting does not work. The count"
         << " is " << ::Count << " when it should be 0" << endl;
    return res;
    } 

  return res;
}
