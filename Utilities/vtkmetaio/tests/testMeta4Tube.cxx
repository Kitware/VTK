#include <stdio.h>
#include <iostream>
#include <ctype.h>
#include <metaTube.h>
#include <metaScene.h>
#include <metaEllipse.h>

int main(int argc, char **argv)
{

  METAIO_STREAM::cout << "Initializing scene ..." << METAIO_STREAM::endl;
  MetaScene myScene = MetaScene(3);

  METAIO_STREAM::cout << "Creating test file ..." << METAIO_STREAM::endl;

  //MetaTubeNet* tubenet = new MetaTubeNet();
  
  // add two tube to the list of tubenet
  METAIO_STREAM::cout << "  Creating first tube ..." << METAIO_STREAM::endl;
  MetaTube* tube1 = new MetaTube(3);
  tube1->ID(0);
  TubePnt* pnt;

  unsigned int i;
  for(i=0;i<10;i++)
  {
    pnt = new TubePnt(3);
    pnt->m_X[0]=i;pnt->m_X[1]=i;pnt->m_X[2]=i;
    pnt->m_R=i;
    tube1->GetPoints().push_back(pnt);
  }
  
  METAIO_STREAM::cout << "  Creating second tube ..." << METAIO_STREAM::endl;
  MetaTube* tube2 = new MetaTube(3);
  tube2->ID(1);
  for(i=0;i<5;i++)
  {
    pnt = new TubePnt(3);
    pnt->m_X[0]=i;pnt->m_X[1]=i;pnt->m_X[2]=i;
    pnt->m_R=i;
    tube2->GetPoints().push_back(pnt);
  }

  // Add an ellipse
  METAIO_STREAM::cout << "  Creating ellipse ..." << METAIO_STREAM::endl;
  MetaEllipse* ellipse = new MetaEllipse();
  METAIO_STREAM::cout << "    Initializing ellipse ..." << METAIO_STREAM::endl;
  ellipse->InitializeEssential(3);
  METAIO_STREAM::cout << "    Setting radius ..." << METAIO_STREAM::endl;
  ellipse->Radius(1,2,3);

  myScene.AddObject(tube1);
  myScene.AddObject(tube2);
  myScene.AddObject(ellipse);

  myScene.Write("test.scn");

  METAIO_STREAM::cout << "done" << METAIO_STREAM::endl;
  METAIO_STREAM::cout << "Reading test file ..." << METAIO_STREAM::endl;

  // Read the result 
  MetaScene myScene2 = MetaScene();
  myScene2.InitializeEssential(3);
 
  METAIO_STREAM::cout << "  ... reading scene " << METAIO_STREAM::endl;
  myScene2.Read("test.scn");
  METAIO_STREAM::cout << "  ... read scene " << METAIO_STREAM::endl;
  
  typedef  MetaScene::ObjectListType ListType;
  ListType * list = myScene2.GetObjectList();
  ListType::iterator it = list->begin();

  METAIO_STREAM::cout << "  ... beginning loop " << METAIO_STREAM::endl;
  for(i=0;i< list->size();i++)
  {
    
    (*it)->PrintInfo();
    if(!strncmp((*it)->ObjectTypeName(),"Tube",4))
    {
      typedef MetaTube::PointListType ListType;
      MetaTube* tube = static_cast<MetaTube*>(*it);
      ListType::iterator it2 = tube->GetPoints().begin();

      for(unsigned int j=0;j< tube->GetPoints().size();j++)
      {
        METAIO_STREAM::cout << (*it2)->m_X[0] 
        << " " << (*it2)->m_X[1] << " " << (*it2)->m_X[2] << METAIO_STREAM::endl;
        it2++;
      }
    }

    it++;
  }

  METAIO_STREAM::cout << "done" << METAIO_STREAM::endl;
  return 1;
}
