#include "vtkIncrementalForceLayout.h"

#include "vtkCommand.h"
#include "vtkDirectedGraph.h"
#include "vtkFloatArray.h"
#include "vtkGraph.h"
#include "vtkMath.h"
#include "vtkObjectFactory.h"
#include "vtkPoints.h"
#include "vtkSmartPointer.h"
#include "vtkVariant.h"
#include "vtkVector.h"

#include <utility>
#include <vector>

class Quad
{
public:
  Quad();
  Quad(float *points, int n, float x1, float y1, float x2, float y2);
  ~Quad();

  void Insert(vtkVector2f &p, vtkIdType vert, float x1, float y1, float x2, float y2);
  void InsertChild(vtkVector2f &p, vtkIdType vert, float x1, float y1, float x2, float y2);
  void ForceAccumulate(float alpha, float charge);
  bool Repulse(vtkVector2f &prev, vtkVector2f &p, vtkIdType vert, float x1, float x2, float theta);
  void Visit(vtkVector2f &prev, vtkVector2f &p, vtkIdType vert, float x1, float y1, float x2, float y2, float theta);

  bool Leaf;
  bool ValidPoint;
  vtkVector2f Point;
  vtkIdType Vertex;
  float PointCharge;
  vtkVector2f Center;
  float Charge;
  Quad *Nodes[4];
};

Quad::Quad()
{
  this->Leaf = true;
  this->ValidPoint = false;
  this->Point = vtkVector2f(0.0f, 0.0f);
  this->Vertex = 0;
  this->Charge = 0.0f;
  this->Nodes[0] = this->Nodes[1] = this->Nodes[2] = this->Nodes[3] = 0;
}

Quad::Quad(float *points, int n, float x1, float y1, float x2, float y2)
{
  this->Leaf = true;
  this->ValidPoint = false;
  this->Point = vtkVector2f(0.0f, 0.0f);
  this->Vertex = 0;
  this->Nodes[0] = this->Nodes[1] = this->Nodes[2] = this->Nodes[3] = 0;

  // Insert points
  for (int i = 0; i < n; ++i)
    {
    vtkVector2f p = *reinterpret_cast<vtkVector2f*>(points + 3*i);
    this->Insert(p, i, x1, y1, x2, y2);
    }
}

Quad::~Quad()
{
  for (int i = 0; i < 4; ++i)
    {
    if (this->Nodes[i])
      {
      delete this->Nodes[i];
      this->Nodes[i] = 0;
      }
    }
}

void Quad::Insert(vtkVector2f &p, vtkIdType vert, float x1, float y1, float x2, float y2) {
  if (vtkMath::IsNan(p.GetX()) || vtkMath::IsNan(p.GetY()))
    {
    return;
    }
  if (this->Leaf)
    {
    if (this->ValidPoint) {
      vtkVector2f v = this->Point;
      // If the point at this leaf node is at the same position as the new
      // point we are adding, we leave the point associated with the
      // internal node while adding the new point to a child node. This
      // avoids infinite recursion.
      if ((fabs(v.GetX() - p.GetX()) + fabs(v.GetY() - p.GetY())) < .01) {
        this->InsertChild(p, vert, x1, y1, x2, y2);
      } else {
        this->ValidPoint = false;
        this->InsertChild(v, this->Vertex, x1, y1, x2, y2);
        this->InsertChild(p, vert, x1, y1, x2, y2);
      }
    } else {
      this->Point = p;
      this->ValidPoint = true;
      this->Vertex = vert;
    }
  } else {
    this->InsertChild(p, vert, x1, y1, x2, y2);
  }
}

// Recursively inserts the specified point p into a descendant of node n. The
// bounds are defined by [x1, x2] and [y1, y2].
void Quad::InsertChild(vtkVector2f& p, vtkIdType vert, float x1, float y1, float x2, float y2)
{
  // Compute the split point, and the quadrant in which to insert p.
  float sx = (x1 + x2) * .5f;
  float sy = (y1 + y2) * .5f;
  bool right = p.GetX() >= sx;
  bool bottom = p.GetY() >= sy;
  int i = (bottom << 1) + right;

  // Recursively insert into the child node.
  this->Leaf = false;
  if (!this->Nodes[i])
    {
    this->Nodes[i] = new Quad();
    }
  Quad *n = this->Nodes[i];

  // Update the bounds as we recurse.
  if (right) x1 = sx; else x2 = sx;
  if (bottom) y1 = sy; else y2 = sy;
  n->Insert(p, vert, x1, y1, x2, y2);
}

void Quad::ForceAccumulate(float alpha, float charge)
{
  float cx = 0.0f;
  float cy = 0.0f;
  this->Charge = 0.0f;
  if (!this->Leaf)
    {
    for (int i = 0; i < 4; ++i)
      {
      Quad *c = this->Nodes[i];
      if (!c)
        {
        continue;
        }
      c->ForceAccumulate(alpha, charge);
      this->Charge += c->Charge;
      cx += c->Charge * c->Center.GetX();
      cy += c->Charge * c->Center.GetY();
      }
    }
  if (this->ValidPoint)
    {
    // Jitter internal nodes that are coincident
    if (!this->Leaf)
      {
      this->Point.SetX(this->Point.GetX() + static_cast<float>(vtkMath::Random()) - 0.5f);
      this->Point.SetY(this->Point.GetY() + static_cast<float>(vtkMath::Random()) - 0.5f);
      }
    float k = alpha * charge;
    this->PointCharge = k;
    this->Charge += this->PointCharge;
    cx += k * this->Point.GetX();
    cy += k * this->Point.GetY();
    }
  this->Center = vtkVector2f(cx / this->Charge, cy / this->Charge);
}

bool Quad::Repulse(vtkVector2f &prev, vtkVector2f &p, vtkIdType vert, float x1, float x2, float theta)
{
  if (this->Vertex != vert)
    {
    float dx = this->Center.GetX() - p.GetX();
    float dy = this->Center.GetY() - p.GetY();
    float dn = 1.0f / sqrt(dx * dx + dy * dy);

    // Barnes-Hut criterion.
    if ((x2 - x1) * dn < theta)
      {
      float k = this->Charge * dn * dn;
      prev.SetX(prev.GetX() - dx * k);
      prev.SetY(prev.GetY() - dy * k);
      return true;
      }
    else if (this->ValidPoint && !vtkMath::IsNan(dn) && !vtkMath::IsInf(dn))
      {
      float k = this->PointCharge * dn * dn;
      prev.SetX(prev.GetX() - dx * k);
      prev.SetY(prev.GetY() - dy * k);
      }
    }
  return !this->Charge;
}

void Quad::Visit(vtkVector2f &prev, vtkVector2f &p, vtkIdType vert, float x1, float y1, float x2, float y2, float theta)
{
  if (!this->Repulse(prev, p, vert, x1, x2, theta))
    {
    float sx = (x1 + x2) * .5;
    float sy = (y1 + y2) * .5;
    if (this->Nodes[0]) this->Nodes[0]->Visit(prev, p, vert, x1, y1, sx, sy, theta);
    if (this->Nodes[1]) this->Nodes[1]->Visit(prev, p, vert, sx, y1, x2, sy, theta);
    if (this->Nodes[2]) this->Nodes[2]->Visit(prev, p, vert, x1, sy, sx, y2, theta);
    if (this->Nodes[3]) this->Nodes[3]->Visit(prev, p, vert, sx, sy, x2, y2, theta);
    }
}

class vtkIncrementalForceLayout::Implementation
{
public:
  Implementation()
    {
    }

  vtkVector2f& GetPosition(vtkIdType i)
    {
    return *(reinterpret_cast<vtkVector2f*>(this->Position + 3*i));
    }

  float *Position;
  std::vector<vtkVector2f> LastPosition;
};

vtkStandardNewMacro(vtkIncrementalForceLayout);
vtkCxxSetObjectMacro(vtkIncrementalForceLayout, Graph, vtkGraph);

vtkIncrementalForceLayout::vtkIncrementalForceLayout()
{
  this->Impl = new Implementation();
  this->Graph = 0;
  this->Fixed = -1;
  this->GravityPoint = vtkVector2f(200.0f, 200.0f);
  this->Alpha = 0.1f;
  this->Theta = 0.8f;
  this->Charge = -50.0f;
  this->Strength = 1.0f;
  this->Distance = 20.0f;
  this->Gravity = 0.1f;
  this->Friction = 0.9f;
}

vtkIncrementalForceLayout::~vtkIncrementalForceLayout()
{
  delete this->Impl;
  this->SetGraph(0);
}

void vtkIncrementalForceLayout::UpdatePositions()
{
  if (!this->Graph)
    {
    return;
    }

  vtkIdType numVerts = this->Graph->GetNumberOfVertices();
  vtkIdType numEdges = this->Graph->GetNumberOfEdges();
  this->Impl->Position = vtkFloatArray::SafeDownCast(this->Graph->GetPoints()->GetData())->GetPointer(0);
  while (numVerts >= static_cast<vtkIdType>(this->Impl->LastPosition.size()))
    {
    this->Impl->LastPosition.push_back(vtkVector2f(0.0f, 0.0f));
    }

  // Swap pos and lastpos for fixed node
  if (this->Fixed >= 0 && this->Fixed < numVerts)
    {
    vtkVector2f &pos = this->Impl->GetPosition(this->Fixed);
    vtkVector2f temp = pos;
    pos = this->Impl->LastPosition[this->Fixed];
    this->Impl->LastPosition[this->Fixed] = temp;
    }

  // Gauss-Seidel relaxation for links
  bool directed = (vtkDirectedGraph::SafeDownCast(this->Graph) != NULL);
  for (vtkIdType e = 0; e < numEdges; ++e)
    {
    vtkIdType s = this->Graph->GetSourceVertex(e);
    vtkIdType t = this->Graph->GetTargetVertex(e);
    vtkVector2f& sPos = this->Impl->GetPosition(s);
    vtkVector2f& tPos = this->Impl->GetPosition(t);
    float x = tPos.GetX() - sPos.GetX();
    float y = tPos.GetY() - sPos.GetY();
    vtkIdType degreeS;
    vtkIdType degreeT;
    if (directed)
      {
      degreeS = this->Graph->GetOutDegree(s) + this->Graph->GetInDegree(s);
      degreeT = this->Graph->GetOutDegree(t) + this->Graph->GetInDegree(t);
      }
    else
      {
      degreeS = this->Graph->GetOutDegree(s);
      degreeT = this->Graph->GetOutDegree(t);
      }
    if (float l = (x * x + y * y))
      {
      float sqrtl = sqrt(l);
      l = this->Alpha * this->Strength * (sqrtl - this->Distance) / sqrtl;
      x *= l;
      y *= l;
      float sWeight = degreeS;
      float tWeight = degreeT;
      float k = sWeight / (tWeight + sWeight);
      if (t != this->Fixed)
        {
        tPos.SetX(tPos.GetX() - x * k);
        tPos.SetY(tPos.GetY() - y * k);
        }
      k = 1 - k;
      if (s != this->Fixed)
        {
        sPos.SetX(sPos.GetX() + x * k);
        sPos.SetY(sPos.GetY() + y * k);
        }
      }
    }

  // Gravity forces
  float k = this->Alpha * this->Gravity;
  if (k)
    {
    float x = this->GravityPoint.GetX();
    float y = this->GravityPoint.GetY();
    for (vtkIdType v = 0; v < numVerts; ++v)
      {
      vtkVector2f& vPos = this->Impl->GetPosition(v);
      if (v != this->Fixed)
        {
        vPos.SetX(vPos.GetX() + (x - vPos.GetX()) * k);
        vPos.SetY(vPos.GetY() + (y - vPos.GetY()) * k);
        }
      }
    }

  // Bounds
  float x1 = VTK_FLOAT_MAX;
  float x2 = VTK_FLOAT_MIN;
  float y1 = VTK_FLOAT_MAX;
  float y2 = VTK_FLOAT_MIN;
  for (vtkIdType i = 0; i < numVerts; ++i)
    {
    vtkVector2f p = this->Impl->GetPosition(i);
    x1 = std::min(x1, p.GetX());
    x2 = std::max(x2, p.GetX());
    y1 = std::min(y1, p.GetY());
    y2 = std::max(y2, p.GetY());
    }

  // Squarify the bounds.
  float dx = x2 - x1;
  float dy = y2 - y1;
  if (dx > dy)
    {
    y2 = y1 + dx;
    }
  else
    {
    x2 = x1 + dy;
    }

  // Charge force
  Quad *tree = new Quad(this->Impl->Position, numVerts, x1, y1, x2, y2);
  tree->ForceAccumulate(this->Alpha, this->Charge);
  for (vtkIdType v = 0; v < numVerts; ++v)
    {
    if (v != this->Fixed)
      {
      tree->Visit(this->Impl->LastPosition[v], this->Impl->GetPosition(v), v, x1, y1, x2, y2, this->Theta);
      }
    }
  delete tree;

  // Position Verlet integration
  for (vtkIdType v = 0; v < numVerts; ++v)
    {
    vtkVector2f& vPos = this->Impl->GetPosition(v);
    vtkVector2f& vLastPos = this->Impl->LastPosition[v];
    if (v != this->Fixed)
      {
      vPos.SetX(vPos.GetX() - (vLastPos.GetX() - vPos.GetX()) * this->Friction);
      vPos.SetY(vPos.GetY() - (vLastPos.GetY() - vPos.GetY()) * this->Friction);
      vLastPos = vPos;
      }
    else
      {
      vPos = vLastPos;
      }
    }
}

void vtkIncrementalForceLayout::SetFixed(vtkIdType v)
{
  if (this->Fixed >= 0 &&
      this->Fixed < static_cast<vtkIdType>(this->Impl->LastPosition.size()))
    {
    this->Impl->LastPosition[this->Fixed] = this->Impl->GetPosition(this->Fixed);
    }
  this->Fixed = v;
}

void vtkIncrementalForceLayout::PrintSelf(ostream &os, vtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
