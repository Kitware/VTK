
#include "vlMath.hh"

vlMath::vlMath()
{
  this->Seed = 1177; // One authors home address
}

//
// Generate random numbers between 0.0 and 1.0
// This is used to provide portability across different systems.
//
// Based on code in "Random Number Generators: Good Ones are Hard to Find",
// by Stephen K. Park and Keith W. Miller in Communications of the ACM,
// 31, 10 (Oct. 1988) pp. 1192-1201.
//
// Borrowed from: Fuat C. Baran, Columbia University, 1988
///

//
// some constants we need
//
#define K_A 16807
#define K_M 2147483647			/* Mersenne prime 2^31 -1 */
#define K_Q 127773			/* K_M div K_A */
#define K_R 2836			/* K_M mod K_A */

//
// return a random float between 0.0 and 1.0
//
float vlMath::Random()
{
  long hi, lo;
    
  hi = this->Seed / K_Q;
  lo = this->Seed % K_Q;
  if ((this->Seed = K_A * lo - K_R * hi) <= 0)
    Seed += K_M;
  return ((float) this->Seed / K_M);
}

//
// initialize seed value
//   NOTE: Random() has the bad property that the first random
//         number returned after RandomSeed() is called is proportional
//         to the seed value! To help solve this, I call RandomSeed() a
//         few times inside seed. This doesn't ruin the repeatability
//         of Random().
//
void vlMath::RandomSeed(long s)
{
  this->Seed = s;

  vlMath::Random();
  vlMath::Random();
  vlMath::Random();
}
