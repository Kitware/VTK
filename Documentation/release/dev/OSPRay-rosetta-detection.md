# OSPRay Rosetta Detection

VTK's OSPRay support learned to detect Apple's Rosetta translation environment
and avoid using it in that case. OSPRay uses AVX instructions which are not
supported within Rosetta.
