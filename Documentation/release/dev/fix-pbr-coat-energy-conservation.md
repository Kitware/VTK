# PBR: the clear coat no longer adds energy

Under image based lighting, the PBR clear coat used to add its reflection on top of a base layer
that was still lit at full strength, so a coated surface reflected more light than it received.
The effect grew towards the silhouette of an object, where the coat should reflect nearly
everything and leave the base layer dark.

Two things are fixed, in the OpenGL and the WebGPU backends alike:

- The base layer is attenuated by the Fresnel term of the coat, once for its specular lobe and
  twice for its diffuse lobe, which is the light that has to cross the coat on the way in and on
  the way out. The OpenGL backend already computed this attenuation but applied it after the base
  layer had been summed, so it had no effect.
- The reflection of the coat itself is no longer weighted by the Fresnel term twice. Its split-sum
  term already accounts for Fresnel, exactly as the base layer's does, so only the coat strength
  is applied to it.

Coated materials are therefore slightly darker where they face the viewer, noticeably darker at
grazing angles, and their coat reflection is brighter. The baseline images of the clear coat tests
were regenerated.

`TestPBRWhiteFurnace` is a new furnace test: it puts a white sphere in an environment of uniform
radiance with no other light, and checks that no material reflects more light than it receives.
It runs for every rendering backend. `TestPBRCoatAttenuation` renders a coat strength sweep and
compares the backends against one shared baseline image.
