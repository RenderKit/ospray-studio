# OSPRay Studio

-   [Overview](index.md)
-   [Getting Started](quickstart.md)
-   [API Documentation](api.md)
-   [Gallery](gallery.md)

## Overview

Intel OSPRay Studio is an open source and interactive visualization and
ray tracing application that leverages [Intel
OSPRay](https://www.ospray.org) as its core rendering engine. It can be
used to load complex scenes requiring high fidelity rendering or very
large scenes requiring supercomputing resources.

The main control structure is a *scene graph* which allows users to
create an abstract scene in a *directed acyclical graph* manner. Scenes
can either be imported or created using scene graph nodes and structure
support. The scenes can then be rendered either with OSPRay's pathtracer
or scivis renderer.
