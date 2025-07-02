## Masters project - Cloth Desgin Plugin for Unreal Engine 5.5 on Linux
### Project Update
I looked over my initial presentation (available [here](https://livebournemouthac.sharepoint.com/:b:/r/sites/MScCAVE202425/Shared%20Documents/MSc%20Project/Initial%20Presentations/Marisa-MSc-Presentation.pdf?csf=1&web=1&e=PkPUL9)) and slightly revised the goals to focus on the most important ones first. I also included an update on what's working so far and what I'll be working on next.
# 
### Revised Plan (Minimal Goals)
I am still working with these three main goals: pattern creation in 2D, edge-based sewing and cloth simulation in 3D.
#### Pattern Creation
- 2D viewport
- Basic drawing tools (starting with rectangles)
- Save format for 2D patterns
#### Sewing
- Edge-based sewing (starting with full-edge connections, so one full edge of one piece is connected to a full edge of another piece)
#### Simulation
- Cloth simulation using Chaos Cloth in Unreal Engine
- Garment-body collisions
- Editable fabric settings or some basic presets (like jersey, denim, wool, etc)
#
### Progress So Far
- Explored Unreal Engine’s C++ workflow and plugin system
- I have a custom editor mode that can be imported as a plugin (screenshot below)
- The editor interface is very basic right now — just two buttons (screenshot below)
- One of the buttons opens a 2D viewport, which is where the pattern creation and sewing tools will live (screenshot below)
- I started with the simulation using Chaos Cloth in Blueprints
### What’s Next
- Get the 3D cloth simulation working in C++ instead of Blueprints
- Add simple editable fabric settings (or some presets)
- Then, start working on the 2D viewport interface beyond just opening it — so begin implementing drawing tools and save logic

#
### Selection of editor mode
![photo](media/photo1.png)

#
### Editor mode buttons
![photo](media/photo2.png)

#
### 2D viewport opens from ClothDesign editor mode 
![photo](media/photo3.png)


