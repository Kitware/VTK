/* Return 1 for char signed and 0 for char unsigned.  */
int main()
{
  unsigned char uc = 255;
  return (*reinterpret_cast<char*>(&uc) < 0)?1:0;
}
