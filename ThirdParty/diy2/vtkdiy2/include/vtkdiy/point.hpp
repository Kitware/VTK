#ifndef DIY_POINT_HPP
#define DIY_POINT_HPP

#include <iostream>
#include <vector>
#include <string>
#include <sstream>

#include <array>

namespace diy
{

template<class Coordinate_, unsigned D>
class Point: public std::array<Coordinate_, D>
{
    public:
        typedef             Coordinate_                             Coordinate;
        typedef             std::array<Coordinate, D>               ArrayParent;

        typedef             Point<Coordinate, D-1>                  LPoint;
        typedef             Point<Coordinate, D+1>                  UPoint;

        template<class U>
        struct rebind       { typedef Point<U,D> type; };

    public:
                            Point()                                 { for (unsigned i = 0; i < D; ++i) (*this)[i] = 0; }
                            Point(const ArrayParent& a):
                                ArrayParent(a)                      {}
        template<class T>   Point(const Point<T, D>& p)             { for (size_t i = 0; i < D; ++i) (*this)[i] = p[i]; }
        template<class T>   Point(const T* a)                       { for (unsigned i = 0; i < D; ++i) (*this)[i] = a[i]; }
        template<class T>   Point(const std::vector<T>& a)          { for (unsigned i = 0; i < D; ++i) (*this)[i] = a[i]; }
                            Point(std::initializer_list<Coordinate> lst)   { unsigned i = 0; for (Coordinate x : lst) (*this)[i++] = x; }

                            Point(Point&&)                          =default;
                            Point(const Point&)                     =default;
        Point&              operator=(const Point&)                 =default;

        static constexpr
        unsigned            dimension()                             { return D; }

        static Point        zero()                                  { return Point(); }
        static Point        one()                                   { Point p; for (unsigned i = 0; i < D; ++i) p[i] = 1; return p; }

        LPoint              drop(int dim) const                     { LPoint p; unsigned c = 0; for (unsigned i = 0; i < D;   ++i) { if (i == dim) continue; p[c++] = (*this)[i]; } return p; }
        UPoint              lift(int dim, Coordinate x) const       { UPoint p; for (unsigned i = 0; i < D+1; ++i) { if (i < dim) p[i] = (*this)[i]; else if (i == dim) p[i] = x; else if (i > dim) p[i] = (*this)[i-1]; } return p; }

        using ArrayParent::operator[];

        Point&              operator+=(const Point& y)              { for (unsigned i = 0; i < D; ++i) (*this)[i] += y[i];  return *this; }
        Point&              operator-=(const Point& y)              { for (unsigned i = 0; i < D; ++i) (*this)[i] -= y[i];  return *this; }
        Point&              operator*=(Coordinate a)                { for (unsigned i = 0; i < D; ++i) (*this)[i] *= a;     return *this; }
        Point&              operator/=(Coordinate a)                { for (unsigned i = 0; i < D; ++i) (*this)[i] /= a;     return *this; }

        Coordinate          norm() const                            { return (*this)*(*this); }

        std::ostream&       operator<<(std::ostream& out) const     { out << (*this)[0]; for (unsigned i = 1; i < D; ++i) out << " " << (*this)[i]; return out; }
        std::istream&       operator>>(std::istream& in);

        friend
        Point               operator+(Point x, const Point& y)       { x += y; return x; }

        friend
        Point               operator-(Point x, const Point& y)       { x -= y; return x; }

        friend
        Point               operator/(Point x, Coordinate y)         { x /= y; return x; }

        friend
        Point               operator*(Point x, Coordinate y)         { x *= y; return x; }

        friend
        Point               operator*(Coordinate y, Point x)         { x *= y; return x; }

        friend
        Coordinate          operator*(const Point& x, const Point& y)   { Coordinate n = 0; for (size_t i = 0; i < D; ++i) n += x[i] * y[i]; return n; }

        template<class T>
        friend
        Coordinate          operator*(const Point<T,D>& x, const Point& y)   { Coordinate n = 0; for (size_t i = 0; i < D; ++i) n += x[i] * y[i]; return n; }
};

template<class C, unsigned D>
std::istream&
Point<C,D>::
operator>>(std::istream& in)
{
    std::string point_str;
    in >> point_str;        // read until ' '
    std::stringstream ps(point_str);

    char x;
    for (unsigned i = 0; i < dimension(); ++i)
    {
        ps >> (*this)[i];
        ps >> x;
    }

    return in;
}


template<class Coordinate, unsigned D>
Coordinate norm2(const Point<Coordinate,D>& p)
{ Coordinate res = 0; for (unsigned i = 0; i < D; ++i) res += p[i]*p[i]; return res; }

template<class C, unsigned D>
std::ostream&
operator<<(std::ostream& out, const Point<C,D>& p)
{ return p.operator<<(out); }

template<class C, unsigned D>
std::istream&
operator>>(std::istream& in, Point<C,D>& p)
{ return p.operator>>(in); }

}

#endif // DIY_POINT_HPP
