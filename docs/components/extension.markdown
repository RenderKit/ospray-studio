### Extending OSPRay Studio

OSPRay Studio can be customized in various ways, including widgets for UI
controls, plugins for application behavior, importers for file type support,
and generators for creating new data.
 
#### Widgets

A widget is a GUI component. It is not necessarily a part of the MainWindow by
default, but can be included for providing controls for special
scenes/datasets. Widgets are implemented as separate classes, which can be
instanced in the MainWindow class for loading the special controls. For
example, an `AnimationWidget` provides animation playback controls once an
animated scene is loaded.

#### Plugins

A plugin is a CMake subproject that enhances the functionality of OSPRay
Studio. Plugins may be individually selected to build in CMake. They can then
be individually chosen to load at runtime on the command line.  An example
plugin is provided to demonstrate how to create new plugins. To test it, enable
plugins and the example plugin in CMake, then run:

```
./ospStudio -p example
```
 
#### Importers

OSPRay Studio comes with support for glTF and OBJ/MTL formats for geometry
data. It supports raw binary and OpenVDB formats for volumetric data.
Additional importers may be added via plugins by extending OSPRay Studio's
`importerMap`.

#### Generators for scene graph

The generator API can be used to create data and node hierarchies
programmatically.  OSPRay Studio comes with a set of sample generators for
reference. A new generator class is created by inheriting from the main
Generator class and writing a `generateData()` method to specify the node
hierarchy.
