#include <stdio.h>
#include <ctype.h>
#include <metaContour.h>

int main(int, char * [])
{
  METAIO_STREAM::cout << "Creating test file ..." << METAIO_STREAM::endl;
  MetaContour Contour(3);
  Contour.ID(0);
  Contour.Name("First Contour");
  ContourControlPnt* pnt;

  METAIO_STREAM::cout << "Allocating points..." << METAIO_STREAM::endl;
  unsigned int i;
  for(i=0;i<10;i++)
  {
    pnt = new ContourControlPnt(3);
    pnt->m_Id = i;
    pnt->m_XPicked[0]=0;
    pnt->m_XPicked[1]=1;
    pnt->m_XPicked[2]=2;
    pnt->m_X[0]=(float)0.2;
    pnt->m_X[1]=(float)i;
    pnt->m_X[2]=(float)i;
    Contour.GetControlPoints().push_back(pnt);
  }

  Contour.Interpolation(MET_EXPLICIT_INTERPOLATION);

  ContourInterpolatedPnt* pntI;
  for(i=0;i<5;i++)
  {
    pntI = new ContourInterpolatedPnt(3);
    pntI->m_Id = i;
    pntI->m_X[0]=(float)0.2;
    pntI->m_X[1]=(float)i;
    pntI->m_X[2]=(float)i;
    Contour.GetInterpolatedPoints().push_back(pntI);
  }

  METAIO_STREAM::cout << "Writing test file ..." << METAIO_STREAM::endl;
  Contour.BinaryData(true);
  Contour.Write("C:/Julien/Contours.meta");

  METAIO_STREAM::cout << "  done" << METAIO_STREAM::endl;

  METAIO_STREAM::cout << "Reading test file ..." << METAIO_STREAM::endl;
  Contour.Read("C:/Julien/Contours.meta");

  METAIO_STREAM::cout << "  done" << METAIO_STREAM::endl;

  Contour.PrintInfo();

  METAIO_STREAM::cout << "Accessing pointlist..." << METAIO_STREAM::endl;

  MetaContour::ControlPointListType plist =  Contour.GetControlPoints();
  MetaContour::ControlPointListType::const_iterator it = plist.begin();

  while(it != plist.end())
  {
    METAIO_STREAM::cout << (*it)->m_Id << " ";
    unsigned int d;
    for(d = 0; d < 3; d++)
      {
      METAIO_STREAM::cout << (*it)->m_X[d] << " ";
      }
    for(d = 0; d < 3; d++)
      {
      METAIO_STREAM::cout << (*it)->m_XPicked[d] << " ";
      }
    for(d = 0; d < 3; d++)
      {
      METAIO_STREAM::cout << (*it)->m_V[d] << " ";
      }
    METAIO_STREAM::cout << METAIO_STREAM::endl;
    it++;
  }


  MetaContour::InterpolatedPointListType ilist =  Contour.GetInterpolatedPoints();
  MetaContour::InterpolatedPointListType::const_iterator iti = ilist.begin();

  while(iti != ilist.end())
    {
    METAIO_STREAM::cout << (*iti)->m_Id << " ";
    for(unsigned int d = 0; d < 3; d++)
      {
      METAIO_STREAM::cout << (*iti)->m_X[d] << " ";
      }

    METAIO_STREAM::cout << METAIO_STREAM::endl;
    iti++;
    }

  METAIO_STREAM::cout << "done" << METAIO_STREAM::endl;
  return 0;
}
