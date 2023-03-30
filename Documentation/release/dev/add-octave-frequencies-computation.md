## Add octave band frequencies computation

You can now get lower/upper frequencies of octaves.
* for the octave, you can choose a band number, named upon its nominal band frequency
* you also can choose if you want to get one-third, half, or full octave frequency ranges (default is full)
* optionally, you can choose to compute it using base-two or base-ten power, default: base-two

You may refer to *"ANSI S1.11: Specification for Octave, Half-Octave, and Third Octave Band Filter Sets"*
for octave/one-third-octave frequencies.

### Examples

```cpp
#include "vtkFFT.h"

std::array<double, 2> freqRange = vtkFFT::GetOctaveFrequencyRange(
    vtkFFT::Octave::Hz_500,          /* Octave */
    vtkFFT::OctaveSubdivision::Full, /* OctaveSubdivision -> octave */
    true                             /* BaseTwoOctave -> base-two */
);

// Output: (353.553, 707.107)
```

```cpp
#include "vtkFFT.h"

std::array<double, 2> freqRange = vtkFFT::GetOctaveFrequencyRange(
    vtkFFT::Octave::Hz_500,                 /* Octave */
    vtkFFT::OctaveSubdivision::SecondThird, /* OctaveSubdivision -> second third-octave */
    false                                   /* BaseTwoOctave -> base-ten */
);

// Output: (446.684, 562.341)
```
