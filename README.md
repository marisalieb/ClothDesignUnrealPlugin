## Cloth Design Plugin for Unreal Engine 5.5 on Linux
### 


#
# Plugin User Instructions

### 1. Loading the Plugin
- Create an Unreal Engine project and add a "Plugins" folder.
- Download and unzip the plugin's zip file.
- Copy the ClothDesignAssets folder in Content Drawer of your project.
- Copy the ClothDesign folder into the Plugins folder in your project.
- Launch Unreal Engine and ensure the ClothDesign plugin is installed.
- The plugin UI will now be available in the Editor Mode section.



### 2. Accessing 3D Controls
- Enter the ClothDesign Editor Mode to access the 3D-side controls.
- Available controls:
  - 2D Editor Launch Button: Opens the 2D pattern authoring surface.
  - Cloth Asset Picker: Select the cloth mesh for simulation.
  - Material Picker: Choose a material for your cloth mesh.
  - Preset Selection: Quickly apply preconfigured simulation settings for common materials (denim, leather, silk, jersey or custom).
  - (Optional but does not yet extend functionality: Collision Asset Picker: Assign collision meshes.)

    
### 3. 2D Editor Overview
The 2D Editor is used to create and edit pattern pieces and seams.

#### 3.1 Modes
- Draw Mode:
  - Place points on the canvas to create pattern shapes.
  - Toggle point types:
    - Bézier points: Press B
    - Linear/non-Bézier points: Press N
- Edit Mode:
  - Move points and Bézier handles.
  - Separate Bézier handles: Press S
  - Delete points or handles: Backspace or Delete
  - Undo/Redo: Ctrl + Z / Ctrl + Y
- Sew Mode:
  - Define seam correspondences by clicking start and end points on two pattern pieces.
  - Remove seams by selecting and deleting connecting lines in Edit Mode.


#### 3.2 Additional Controls
- Clear All or Clear All Sewing: Buttons in the left panel.
- Save/Load Shapes: Save your work and load previously saved pattern data.
- Background Textures: Optionally import textures from the content browser to trace existing patterns.


### 4. Converting 2D Patterns to 3D Meshes
- Finalise your 2D shapes on the canvas.
- Click Generate Meshes to triangulate and create 3D meshes.
- Position meshes in the 3D viewport to match collision geometry.
- Select edges to sew and click Sewing to finalise connections.
- Merge pattern pieces into a single mesh with Merge Meshes for simulation.

Note: Sewing and merging are separate steps to allow precise placement before combining.


### 5. Preparing Skeletal Mesh for Simulation
Once the merged skeletal mesh is saved in the content browser:
- OPen the Skeletal Mesh Editor
- Create and apply Clothing Data.
- Enter Cloth Painting Mode and paint the mesh entirely.
- Exit painting mode.
- Optional: Adjust clothing settings (iteration, subdivisions) to improve simulation accuracy and collision handling.

### 6. Final Simulation Setup
- Select the skeletal mesh in the Cloth Object Picker on the 3D UI.
- Assign a material and select a Simulation Preset.
- The mesh is now ready to simulate using the grey "Simulate" button in the top bar.

Note: The cloth and material pickers also work with externally imported meshes (e.g., Marvelous Designer), enabling fast setup for any cloth asset in UE.



#
# Testing

to run tests either go to Riders test section in the bar on the left and run the tests there by clicking 'run unit tests'. 
or go to UE: tools > session frontend > automation > search for your tests there and click run to check them in UE

# Images
### Selection of custom editor mode
![photo](media/photo1.png)



Further sources:
https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/GeometryCore/CompGeom/PolygonTriangulation__Triangulat-/1
https://dev.epicgames.com/community/learning/tutorials/LZZo/unreal-engine-epic-games-store-chaos-cloth-updates-5-6

https://github.com/jothepro/doxygen-awesome-css?tab=readme-ov-file
