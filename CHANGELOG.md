Version History
---------------

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
