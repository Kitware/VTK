#include <fstream>
#include <iostream>
#include <iomanip>
#include "math.h"

using namespace std;


/////////////////////////////////////////////////////////////////////////
//
// Read a file of halo tag and number of particles in that halo (ASCII)
// and bin into log histograms
//
/////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv)
{
  int numberOfBins = 15;
  int numberOfHalos = 5185708;
  float massFactor = 5.3835780e10;
  float lightest_halo = massFactor * 10.0;
  float M_min = 1.0e11;
  float M_max = 1.0e16;

  // Read the number of particles per halo
  string fileName = argv[1];
  ifstream inStream(fileName.c_str(), ios::in);

  float* haloMass = new float[numberOfHalos];
  int tag;
  int particleCount;

  for (int h = 0; h < numberOfHalos; h++) {
    inStream >> tag >> particleCount;
    haloMass[h] = particleCount * massFactor;
  }
  inStream.close();

  float M = M_min;
  float dM = log10(M_max/M_min) / (numberOfBins - 1);
  float dM1;

  float* M_weight = new float[numberOfBins];
  float* binCount = new float[numberOfBins];
  for (int bin = 0; bin < numberOfBins; bin++) {
    M_weight[bin] = 0.0;
    binCount[bin] = 0.0;
  }

  // Iterate over bins building histogram
  for (int bin = 0; bin < (numberOfBins - 1); bin++) {

    // Iterate over halos adding to correct bin
    int count = 0;
    for (int h = 0; h < numberOfHalos; h++) {
      if ((haloMass[h] >= M) && (haloMass[h] < (M * pow(10, dM)))) {
        count++;
        M_weight[bin] += haloMass[h];
      } 
    }
    binCount[bin] = count * 1.0;

    if ((binCount[bin] > 0.0) && (lightest_halo > M))
      dM1 = log10(M * pow(10, dM)) / lightest_halo;
    else
      dM1 = dM;

    if (binCount[bin] > 0.0)
      M_weight[bin] /= binCount[bin];
    else
      M_weight[bin] = M + (M * pow(10, dM1)) / 2.0;

    M_weight[bin] = log10(M_weight[bin]);

    cout << setprecision(8) << setw(9) << showpoint
         << log10(M) << "\t" 
         << log10(M * pow(10, dM)) << "\t" 
         << binCount[bin] << endl;

    M = M * pow(10, dM);
  }

  delete [] haloMass;
  delete [] M_weight;
  delete [] binCount;
}
