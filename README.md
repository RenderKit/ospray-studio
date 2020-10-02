# OSPRay Studio

Intel OSPRay Studio is an open source, interactive visualization and ray
tracing application that leverages Intel OSPRay as its core rendering engine.
It can be used to load complex scenes requiring high fidelity rendering or very
large scenes requiring supercomputing resources. The main control structure is
a scene graph which allows users to create an abstract scene in a generic tree
hierarchy. Scenes can either be imported or created using scene graph nodes and
structure support.  The scenes can then be rendered either with a `pathtracer`
or a `scivis` renderer.
 
## Current version

OSPRay Studio is released under the Apache 2.0 license and latest version is
OSPRay Studio v0.5.0
 
## Find OSPRay Studio

OSPRay Studio is available for Linux, Mac OS X, and Windows.

Binary packages of the latest release of OSPRay Studio can be downloaded as a
part of [Intel oneAPI Rendering
Toolkit](https://software.intel.com/content/www/us/en/develop/tools/oneapi/download.html#renderkit).

OSPRay Studio sources can be downloaded from the default `master` branch of
[OSPRay Studio Github repository](https://github.com/ospray/ospray_studio/).
Download and build instructions for the sources are listed below.
 
## Building OSPRay Studio

OSPRay studio has following set of required and optional dependencies:
 
### Required dependencies

* _Intel OSPRay(v2.4.0)_
  
OSPRay Studio builds on top of OSPRay, please refer to OSPRay building
instructions here to create the latest OSPRay install libraries.  OSPRay Studio
however recommends usage of C++14 (OSPRay recommends C++11)
 
* _OpenGL and GLFW_
  
The windowing environment of the application requires you to have some version
of OpenGL and GLFW.
 
* _Intel OpenVKL(v0.11.0), Intel TBB(2020.2) and rkcommon(v1.5.1)_
  
These dependencies are common with OSPRay, hence same install library locations
can be reused during OSPRay Studio build.
 
### Optional Dependencies

* _Intel OpenImageDenoise_
   
To be able to use Intel OpenImageDenoise with OSPRay Studio, OSPRay should be
built with `-DBUILD_OIDN=ON` option.

* _OpenImageIO and OpenEXR_
  
OSPRay Studio can be built with OpenImageIO and OpenEXR to support images in a
variety of file formats. Please set `OPENIMAGEIO_ROOT` and  `OPENEXR_ROOT`
environment variables to include correct libraries and headers for compilation.
 
### Building on Linux and Mac OS X

Having installed OSPRay correctly, please perform the following steps to
download and build OSPRay Studio Export all dependency install locations in the
following cmake recognizable environment variables

```
export ospray_DIR = ${OSPRAY_INSTALL_LOCATION}
export openvkl_DIR = ${OPENVKL_INSTALL_LOCATION}
export rkcommon_DIR = ${RKCOMMON_INSTALL_LOCATION}
```
 
* For cloning OSPRay Studio sources:

```
git clone https://github.com/ospray/ospray_studio/
```
 
* Create build directory and change directory to it

```
cd ospray_studio
mkdir build
cd build
```
 
* Run cmake

```
cmake ..
```
 
* Run make

```
make -j `nproc`
``` 

(on Linux)

```
cmake --build .
``` 

(on MAC OS X)
 
* To run OSPRay Studio

Make sure LD_LIBRARY_PATH(on Linux) or DYLD_LIBRARY_PATH(on Mac OS X) is set to OSPRay and OpenVKL build directories using :

```
export LD_LIBRARY_PATH = ${OSPRAY_BUILD_LOCATION}
```

```
./ospStudio
```
 
### Building on Windows

For configuring and generating a Microsoft Visual Studio solution file for
OSPRay Studio please use the CMake application(cmake-gui). Steps for generation
are as follows:
 
* Specify the source folder and the build directory in the CMake GUI.
* Specify ospray_DIR, openvkl_DIR and rkcommon_DIR CMake variables for the
  respective install locations.
* Click configure and select the appropriate generator(we recommend using at
  least Visual Studio 15 2017).
* Select x64 as optional parameter for the generator(32-bit builds are not
  supported).
* Click generate to create `ospray_studio.sln`. Open this in visual studio and
  compile.

Optionally, with CMake command line you can do: cmake --build . --config
Release --target install
 
## OSPRay Studio Features
 
Following are the main components of OSPRayStudio:
 
### Application MainWindow

The application MainWindow is a `GLFWwindow` instance, which is an opaque
window object with `imgui` GUI menu options. The MainWindow instance for a
session is responsible for managing the Frame object, which in turn manages the
`Framebuffer`, `Camera`, `Renderer` and `World` objects. Imgui controls allow
interaction with the objects and manipulate the scene.
 
### Modes

* GUI Mode:

This is the default mode of using OSPRay Studio, and  upon successful launch
presents a blank application MainWindow with GUI controls in the top bar.
Scenes can now be loaded from demo scenes or imported, they can be manipulated
and interacted with using menu options. To explore further options of GUI mode,
please try:

```
./ospStudio gui --help
```

 
* Batch Mode:

This mode does not present the user with an interactive window but can write
images in the desired file format upon successfully loading the scene. It can
be used by using the following command line syntax:
 
```
./ospStudio batch [parameters] [scene_files]
```
 
A convenient `--help` option presents a description of other params that can be
specified while using the batch mode.
 
* Timeseries Mode:

This mode can be used to import timestep volume datasets in `raw` or `openVDB`
formats. Correct syntax to use this mode is :
 
```
./ospStudio timeseries [parameters] [files]
```
 
If syntax entered is incorrect, it would present a detailed description of the
right usage of this mode.
 
It runs a MainWindow instance same as the GUI mode but with an additional
timeseries control widget to allow users to have an overview of the timeline
and options to play/pause the timeseries data.
 
### Importing scenes with command-line

To import scene files directly via command line, provide scene file paths separated by spaces.

```
./ospStudio [file1 file2 file3 ..]
```

### GUI features

* Demo scenes: 
  
Demo scenes are located under the file menu option and are a good starting
place to try various OSPRay Studio controls.  After selecting a demo to load,
you can try different editors and viewers present under the Edit and View menu
option to interact with the scene.

* Edit Menu:
  
Edit menu allows for changing renderer settings and framebuffer properties.
Additional options include setting a Tone Map or Background texture.

* View Menu:
  
Presents users with scene objects specific editors and viewers. Notable editors
include :

Lights Editor, which allows you to add new light objects of different light
types in the scene.

Camera Editor, which allows you to try different camera settings at runtime.

Geometry Viewer, presents a hierarchy of loaded geometries in the scene and
their transform properties.

Materials Editor, presents a hierarchy of materials in the scene and the
ability alter their appearance.
 
### Widgets

A widget is a GUI component, which is not a part of the MainWindow by default
but can be included for providing controls for special scenes/datasets. For eg:
Animation Widget can be used to provide animation controls for an animated
scene. Widgets are implemented as separate classes, which can be instanced in
the MainWindow class for loading the special controls.

### Plugins

A plugin is like a CMake subproject that enhances the functionality of existing
OSPRay Studio build but does not have to be compiled with original source by
default. A CMake variable can be used to include this subproject sources for
compilation, which then allows for loading the plugin at runtime. Plugins can
be used to load Geometry, UI, and additional file importers to OSPRay Studio.
We have a sample plugin to showcase this functionality which can be run using : 

```
./ospStudio -p example
```
 
### Importers for scene graph

OSPRay Studio supports importing scenes and volumes in a variety of formats.
Meshes/Scenes in `GLTF` and `OBJ` format can be loaded using gltf importer and
obj importer respectively. Volumes in `raw` or `OpenVDB` format are supported
as well.  Importers in OSPRay Studio create abstract scenes from the imported
scene files. Abstract scenes are then processed by scene graph visitors to
create OSPRay scene objects and world.

Importer API:

```cpp
 std::string getImporter(rkcommon::FileName fileName)
```

Fetches the correct importer type identifier for the file.

```cpp
void importScene(
    std::shared_ptr<StudioContext> context, rkcommon::FileName &fileName)
```

Generates scene graph hierarchy of the imported scene.

### Generators for scene graph

For creating test/small scenes, you can use the generator API and create node
hierarchies that load it into the current world of the MainWindow. OSPRay
studio comes with a set of sample generators for reference. A new generator
class is created by inheriting from the main Generator class and writing a
`generateData()` method to specify the node hierarchy.

### Scene save and reload

A rendered scene in OSPRay Studio can be saved to a `scene graph file`(.sg
file) and reloaded at any time. The .sg file is a JSON file which can include
entire scene hierarchy starting from root World node and include camera state
as well. Imported scene information like meshes are stored by their filenames
which avoids storing large amounts of data in the .sg file.

The scene graph API documentation is available here.
