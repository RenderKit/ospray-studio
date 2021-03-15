glTF importer:

-   mesh, vanilla materials, animation (including camera), skinning
-   KHR_lights_punctual
-   KHR_materials_pbrSpecularGlossiness: "diffuseFactor", "diffuseTexture", "specularFactor", "glossinessFactor"
-   KHR_materials_clearcoat: "clearcoatFactor", "clearcoatRoughnessFactor", "clearcoatTexture", "clearcoatRoughnessTexture", "clearcoatNormalTexture"
-   KHR_materials_transmission: "transmissionFactor", "transmissionTexture"
-   KHR_materials_sheen: "sheenColorFactor", "sheenColorTexture", "sheenRoughnessFactor", "sheenRoughnessTexture"
-   KHR_materials_ior: "ior"
-   KHR_materials_volume: "thicknessFactor", "thicknessTexture", "attenuationDistance", "attenuationColor"
-   KHR_materials_specular: "specularFactor", "specularTexture"
-   KHR_texture_transform: "offset", "rotation", "scale"
-   BIT_scene_background: "background-uri", "rotation"
-   BIT_asset_info: "title"
-   BIT_reference_link: "id", "title"
-   BIT_node_info: "id"
-   EXT_cameras_sensor, respecting
    -   "imageSensor": "pixels", "pixelSize"
    -   "lens": "focalLength", "apertureRadius", "focusDistance", "centerPointShift"

PCD importer

Meta data export, using plugin "metadata" via `ospStudio batch -p metadata -m ...`:

-   in exr: id (instanceID), objectID, depth, worldPosition
-   ID maps in prefix.id.json and prefix.objectId.json
-   3D bounding boxes in prefix.bboxId.json

batch mode:

-   setting animation speed (fps)
-   selecting frames (sub-range) to be rendered
-   selecting camera (if multiple specified in glTF)
-   selecting scene configuration from command line
-   setting rendering parameters (maxContribution, accumLimit, variance)
