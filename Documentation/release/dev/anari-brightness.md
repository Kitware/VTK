## ANARI now renders directional lights as brightly as the OpenGL backend

Scenes lit by directional lights looked much darker through ANARI than they did
through the OpenGL backend. Directional lights are now scaled the same way point
and spot lights already were, so a scene keeps its brightness when you render it
with ANARI.

| Before | After |
|:------:|:-----:|
| ![Sphere lit by a directional light, too dark through ANARI](anari-directional-light-before.png) | ![The same sphere through ANARI, now as bright as the OpenGL backend](anari-directional-light-after.png) |
