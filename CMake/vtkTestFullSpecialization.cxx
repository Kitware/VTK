template <class T> struct A {};
template <> struct A<int*>
{
  static int f() { return 0; }
};
int main() { return A<int*>::f(); }
