Version History
---------------

### Changes in OSPRay Studio v1.0.0

- Update 3rd party dependencies
  - Implicitly included in this repo:
    - CLI11 v2.4.0 (github.com/CLIUtils/CLI11)
    - dear imgui v1.90.2 WIP (docking branch) (github.com/ocornut/imgui)
    - dirent v1.24 (github.com/tronkko/dirent)
    - ImGuiFileDialog v0.6.6.1 (github.com/aiekick/ImGuiFileDialog)
    - imGuIZMO.quat v3.0 (github.com/BrutPitt/imGuIZMO.quat)
    - JSON for Modern C++ 3.11.3 (github.com/nlohmann/json)
    - stb_image v2.29 (github.com/nothings/stb)
    - tinydng v0.1.0 (github.com/syoyo/tinydng)
    - tinyexr v1.0.7 (github.com/syoyo/tinyexr)
    - tinygltf v2.8.20 (github.com/syoyo/tinygltf)
    - tinyobjloader v2.0.0rc13 (github.com/tinyobjloader/tinyobjloader)
  - via FetchContent:
    - glfw v3.3.9 (github.com/glfw/glfw)
    - pybind11 v2.11.1 (github.com/pybind/pybind11)
    - draco v1.5.7 (github.com/google/draco)

### Changes in OSPRay Studio v0.13.0

-   Compatible with OSPRay release v3.0.0

- Features and Improvements
  - OSPRay v3 Compatibility:<br>
    - Intel Xe GPU beta support: launch Studio with parameters
      `--osp:load-modules=gpu --osp:device=gpu`
    - Add OSPRay v3 enum compatibility, tightened type-checking on all enum usage
    - Add OSPRay v3 enum type comprehension to json for .sg files<br>
      **(Note: This change will break existing .sg files.  The most
      common parameter is "intensityQuantity", which previously had
      a subType of "uchar" and needs to change to
      "OSPIntensityQuantity".)**
  - Texture Formats:  Support for .exr and .tiff image formats by
    default though inclusion of tinyexr and tinydng 3rd party header-only
    libraries.  OpenImageIO is still available as a build option.
  - Convert glTF light intensity from lux to watts.  glTF specifies intensity
    in lux, OSPRay in watts.  Convert to watts for compatibility with glTF
    exporters.
  - Add default HDRI map, used when no other map is present.  OSPRay used to
    crash when removing HDRI map from light or when texture would fail to load.
    Automatically switches to this default map now.
  - Fix UI "pick focus distance"
    - Left-Shift+left-mouse sets pick-point to center-of-screen,
      center-of-arcball
    - Left-Crl-Shift+left-mouse sets pick-point to focus distance without
      moving center of arcball
  - More intuitive behavior around Search All actions vs selected actions
  - Transform editor now enables traversing selected node's parents
  - Minor cleanup/refactor of GenerateImGuiWidgets
  - Prevent filename widget collisions:
    If there were multiple filename widgets visible, they would all
      receive the file list returned by fileBrowser.
  - Enable OpenImageDenoise in python bindings and python tutorial scripts
  - Add ability to reload Assets rather than just instance
  - Cameras loaded by assets now add to scene cameras rather than replacing
  - Expose selectCamera to StudioContext, callable by plugins
  - Enable parsing of general texture.transform in mtl files, was limited to
    individual rotation, scale and translation
<br><br>

- Cleanup and bug fixes:
  - Prevent nullptr dereference on deleted importer assets
  - Fix sporadic crash SearchWidget when search term had no match
  - Use custom deleter for pre-C++17 compatability
  - Resolve incorrect delete usage in Texture2D
  - Fix addLight to update existing light parameters.
    If addLight is called with an existing light, it was returning immediately.
    It should update the light parameters with that of the new light.
  - Be more careful about adding unique names in glTF importer.
    When a node, light, or camera name is unclear a unique number is added to
    the name.  Importer was accidentally doing this twice or adding unique
    number to already unique names.

### Changes in OSPRay Studio v0.12.1

-   Compatible with OSPRay release v2.12.0

- Cleanup and bug fixes:
  - Fix issues when running with 192+ MPI ranks
  - Cleanup clang compile warnings
  - Expose python bindings for camera getZoomLevel/setZoomLevel.
  - Add light enable/disable property rather than relying on intensity=0
  - Optimize texture load caching
  - Batch mode camera fixes and improvments
  - UI and SearchWidget improvements
  - Additional display buffer channels in the Framebuffer menu
    (albdeo, normal, depth, ID buffers)
  - Added obj/mtl parsing of hexcode colors (#RGB or s#RGB)

### Changes in OSPRay Studio v0.12.0

-   Compatible with OSPRay release v2.11.0

- Features and Improvements
  - glTF VDB volume loading support - via custom reference link extension node
  - Enhancement to the wavelet slice generator for much larger dataset generation
  - Add command line option to save final image when GUI exits (github request)
  - Add transform node "visible" boolean to permit hiding entire hierarchies
  - Overhaul the plugins/CMakeLists.txt to allow plugins that exist outside
    repository
  - Loading glTF files will now only print INFO messages with the
    `--optVerboseImporter` option.
  - Change camera back to DefaultCamera on update so as not to overwrite scene
   cameras
<br><br>

- Cleanup and bug fixes:
  - Eliminate crash when there is no camera in an empty scene
  - Fix Intel ICX Compiler build
  - Updated all 3rd party dependencies
  - Remove strict dependency on openGL for non-UI plugins and app modes
  - Major refactor of functional units for more code reuse
  - Load raw volumes as binary, closes
    (https://github.com/ospray/ospray_studio/issues/20)
  - Fix --denoiser command line for GUI mode
    (https://github.com/ospray/ospray_studio/issues/21)
  - Allow longer than 64 character save image filenames to accommodate full path name.


### Changes in OSPRay Studio v0.11.1

-   Compatible with OSPRay release v2.10.0

- Features and Improvements
  - Updated README.md to clarify build and superbuild process.
<br>

- Bug Fixes:
  - Fixed a potential linker error when building OSPRay Studio against release
  binaries of OSPRay v2.10.0.
  - Fixed a crash due to loading scene files created prior to Studio v0.9.0.
  - Fixed an error where all command line arguments were ignored in batch and
    benchmark modes.

### Changes in OSPRay Studio v0.11.0

-   Compatible with OSPRay release v2.10.0

- Note! The background color now defaults to black.  It is left to the user to
  set a background color through either the UI or command-line arg `--bgColor
  <rgba>`.  This was done to prevent confusion when background color is blended
  with an HDRI light.

- Features and Improvements
  - UI
    - Improved transform SearchWidget to correctly handle volumes and allow 
    - Fix animation start time and allow playback without UI showing using
      `<spacebar> hotkey` to toggle playback
  - Camera
    - Improved scene camera support with ability to switch between multiple
      cameras without overwriting the preloaded camera positions
  - Batch
    - Add ability to export SceneGraph file in batch mode, including scene
      camera information
    - Benchmarking is enabled by default with the same command line options as
      batch-mode.  `ospStudio benchmark batch [options]`
  - Importer
    - Added `INTEL_lights_sunsky` vendor extension to glTF importer
    - Added support for glTF `KHR_materials_emissive_strength`
    - Added VolumentricModel `densityScale` and `anisotropy` parameters
    - VDB volumes use contiguous data layouts, which can provide improved
      performance

<br>

- Bug Fixes:
  - Fixed crash caused by deleted node not being completely removed from lists
  - Fixed rare crash when default material loaded from .sg is not OBJ
  - Improved geometry and volume visibility toggle, which would occasionally get confused
  - Fixed crash in FileBrowser widget if selecting a non-existent directory path
  - Fixed glTF skinning on models that weren't handled correctly, normalize all weights.
  - Correctly set material opacity to `baseColorfactor` alpha for glTF material
    `alphaMode BLEND`
  - Fixed unusual case where glTF textures could use the same texture

### Changes in OSPRay Studio v0.10.0

-   Compatible with OSPRay release v2.9.0

- Features and Improvements
  - UI
    - Improved color picker widget so that color value are consistent whether
      viewing frame as sRGB or linear.
    - Added widget to change file selection on HDRI textures and photometric lights
    - Added renderer setting to set and adjust OSPRay's `shadowCatcherPlane`
  - Light
    - Add support for new OSPRay `cylinder` light-type
    - Photometric intensity distribution now supported on point, quad, and spot
      light types.
    - Lights in glTF scene are now instanced and can be used with motion blur
  - Camera
    - Custom support for `panoramic` camera type in glTF files
    - All glTF cameras are instanced to allow for motion blur
  - Batch
    - Improved batch rendering flexibility with params for `cameraType`,
      `bgColor`, `frameStep` and `denoiser`.
    - Added final frame denoising to batch mode -- only final accumulation is
      denoised.
  - Added glTF support for OSPRay's new multi-segment deformation motion blur
  - Added python bindings for `affine3f` and `affine3f::translate`
  - Added tasking/scheduling system.  Currently used to greatly improve loading
    of raw volumes.  Will be expanded to other asynchronous operations.

<br>

- Bug Fixes:
  - Fixed lights editor interaction with group lights loaded in a glTF file.  UI
    now allows correct editing/removal of existing lights and addition new lights
  - Added full path name and extension to importer and texture cache entries to
    prevent collision with similar files.
  - Better handling of failed texture load across glTF and OBJ materials, as well
    as HDRI, background texture, and material editor.
  - Correctly enable to/from_json for LinearSpace2f texture transform
  - Improved saving of multi-layer EXR images

### Changes in OSPRay Studio v0.9.1

-   Compatible with OSPRay release v2.8.0

- Features and Improvements
  - Added simplified basic 5-step build instructions to README.md
<br>

- Bug Fixes:
  - Fixed compatibility with Intel® C++ Compiler Classic on Windows OS.
  - Fixed potential crash in FileDialog selection widget.
  - Fixed "gray screen" no-image issue seen on some older architectures

### Changes in OSPRay Studio v0.9.0

-   Compatible with OSPRay release v2.8.0

- Features and Improvements
  - Added initial EULUMDAT photometric light support to SpotLight
  - Enabled alternate camera selection in glTF scenes
  - Added ability for python scripts to define transfer functions and load Studio plugins
  - Enabled .sg scene files can now contain an HDRI light
  - Much improved command line parsing in all modes
  - Added support for several new KHR_materials extensions
    (`KHR_materials_volume`, `KHR_materials_specular`, `KHR_materials_ior`)
    and  `KHR_texture_transform`
  - Improved UI controls for adjusting model transforms
<br>

- Bug Fixes:
  - Fixed high-DPI display issues
  - Fixed bug causing model textures to be flipped if loading HDRI first
  - Fixed crash when selecting non-default cameras in glTF scenes
  - Fixed bug with macOS ARM build.
  - Fixed `KHR_lights_punctual` light direction bug


### Changes in OSPRay Studio v0.8.1

-   Compatible with OSPRay release v2.7.1

-   Bug Fixes
    -   Fixed CMake scripts to make optional 3rd party dependencies easier to predict.
        Optional 3rd party dependencies must be pre-installed prior to building Studio,
        rather than using FetchContent.
    -   Fixed bug in the demo scene "sphere" material

### Changes in OSPRay Studio v0.8.0

-   Compatible with OSPRay release v2.7.x
-   GitHub release binaries are built with python bindings for Python 3.7
 
-   Features/Improvements
    -   Support for OSPRay's camera Motion Blur and Transformation Motion Blur for animated glTF scenes
    -   Support for UDIM texture tiling workflow
    -   New transfer functions and color maps for volume rendering
    -   Much improved search widget for scene objects and materials
    -   Large scene performance optimization
    -   Added ImGui docking and viewport support (experimental)
    -   Updated external 3rd party dependencies bringing in features and bug fixes
    -   Modified OBJ importer to parse and use quads and triangles
    -   Improved Arcball camera navigation with up-vector lock
<br>

-   Bug Fixes
    -   Fixed materials after clearing scene
    -   Fixed materials for PointCloudData (PCD)
    -   Can no longer create a zero-dimension framebuffer
    -   Fixed instancing node-naming to allow for for many identical instances
    -   Fixed crash if optional glTF punctual light color was omitted
    -   Fixed crash if isosurface geometry is used as clipping geometry when no other
        geometries are in the scene

### Changes in OSPRay Studio v0.7.0

-   Compatible with OSPRay release v2.6.x

-   Add SceneGraph python bindings
    Access to scene graph concepts from python3 environment
    Simple examples provided

-   Expose support for OSPRay / Open VKL Particle Volume primitives

-   Add PCD (Point Cloud Data) file importer -- ascii and binary formats

-   Transfer function and isosurface enhancements provide more control over volume visualizations

-   Support OSPRay's varianceThreshold for adaptive accumulation

-   glTF and camera animation improvements

-   UI improvements
    Camera manipulator dolly and focal distance/depth-of-field
    Quick select common frame sizes
    Denoiser can be enabled/disabled without resetting accumulation

-   Add UI parameters to demo scene generators for better interactivity

### Changes in OSPRay Studio v0.6.0 (alpha)

-   Second alpha, compatible with OSPRay release v2.5.x

-   glTF loader improvements:
    Animation and skinning of triangle meshes
    KHR_lights_punctual support for glTF Lights and experimental environmental lights
    Initial glTF `KHR_materials_*` extensions and `KHR_texture_transform` support.
    Add support for glTF POINTS primitive for point clouds
    Support GLTF scene cameras
    Added rich image generation containing GeomIDs and InstanceIDs, exported as additional exr layers

-   Scene File: Load/Save scene file with model transforms, material, lights and camera state

-   Image export improvement
    Exported images and now correctly denoised
    batch-mode supports different image formats and exr options

-   Batch and plugin improvements
    animation in batch mode

-   UI improvements
    Lights editor, add/remove/edit lights for easy scene lighting
    Camera path and keyframe editor
    Camera type and parameter editor
    Rendering stats window added
    Add UI to adjust volume "filter" mode
    Transfer function widget for volumes

-   Core
    Add curves geometry
    Initial isosurface support for volumes
    Expose SceneGraph support for clipping geometries
    Implement an AssetsCatalogue to avoid re-importing assets, simply instance
    Import raw structured and spherical files with specific volume nodes

### OSPRay Studio v0.5.0 (alpha)

-   Initial public alpha release, compatible with OSPRay release v2.4.x
