#include <stdio.h>
#include <ctype.h>
#include <metaLandmark.h>

int main(int , char * [])
{

  METAIO_STREAM::cout << "Creating test file ..." << METAIO_STREAM::endl;
  MetaLandmark Landmark(3);
  Landmark.ID(0);
  LandmarkPnt* pnt;

  METAIO_STREAM::cout << "Allocating points..." << METAIO_STREAM::endl;
  unsigned int i;
  for(i=0;i<10;i++)
  {
    pnt = new LandmarkPnt(3);
    pnt->m_X[0]=(float)0.2;pnt->m_X[1]=i;pnt->m_X[2]=i;
    Landmark.GetPoints().push_back(pnt);
  }
  
  METAIO_STREAM::cout << "Writing test file ..." << METAIO_STREAM::endl;
   
  Landmark.BinaryData(true);
  Landmark.ElementType(MET_FLOAT);
  Landmark.Write("Landmarks.meta");

  METAIO_STREAM::cout << "  done" << METAIO_STREAM::endl;
 
  METAIO_STREAM::cout << "Reading test file ..." << METAIO_STREAM::endl;
  Landmark.Read("Landmarks.meta"); 

  METAIO_STREAM::cout << "  done" << METAIO_STREAM::endl;

  Landmark.PrintInfo();

  METAIO_STREAM::cout << "Accessing pointlist..." << METAIO_STREAM::endl;

  MetaLandmark::PointListType plist =  Landmark.GetPoints();
  MetaLandmark::PointListType::const_iterator it = plist.begin();
  
  while(it != plist.end())
  {
    for(unsigned int d = 0; d < 3; d++)
    {
      METAIO_STREAM::cout << (*it)->m_X[d] << " ";
    }

    METAIO_STREAM::cout << METAIO_STREAM::endl;
    it++;
  }

  METAIO_STREAM::cout << "done" << METAIO_STREAM::endl;
  return 0;
}
