// reference to a 2D array element; this class is nested within zfp::array2
class reference {
public:
  operator Scalar() const { return array->get(i, j); }
  reference operator=(const reference& r) { array->set(i, j, r.operator Scalar()); return *this; }
  reference operator=(Scalar val) { array->set(i, j, val); return *this; }
  reference operator+=(Scalar val) { array->add(i, j, val); return *this; }
  reference operator-=(Scalar val) { array->sub(i, j, val); return *this; }
  reference operator*=(Scalar val) { array->mul(i, j, val); return *this; }
  reference operator/=(Scalar val) { array->div(i, j, val); return *this; }
  pointer operator&() const { return pointer(*this); }
  // swap two array elements via proxy references
  friend void swap(reference a, reference b)
  {
    Scalar x = a.operator Scalar();
    Scalar y = b.operator Scalar();
    b.operator=(x);
    a.operator=(y);
  }

protected:
  friend class array2;
  friend class iterator;
  explicit reference(array2* array, uint i, uint j) : array(array), i(i), j(j) {}
  array2* array;
  uint i, j;
};
