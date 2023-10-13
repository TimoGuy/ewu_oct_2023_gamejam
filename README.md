# EWU October 2023 Game Jam

## Action Items (TODO) for 3D part of Game.

- [x] Initial Setup
    - [x] Fork from TimoEngine v1 and compile
    - [x] Remove gondolasystem
    - [x] Make all voxelfields static.
- [x] Set up setting
    - [x] Create a sample hallway thingo.
    - [x] Bake lighting
- [x] Build camera
    - [x] Remove camera from mouse
    - [~] ~~Create triggers~~
    - [~] ~~Set the triggers to move the camera angles.~~
    - [ ] Create camera rails without the voxel triggers.
        - [~] ~~Maybe disklet triggers?~~
        - [ ] Create boundaries that the focus subject (i.e. player) upon entering will set up the new settings for the camera region.

    > NOPE NOPE NOPE!!!! We're just gonna do the voxel based camera thing. This will make things easiest and quickest to author the level.

    - [x] Create camera switching direction with voxel triggers.
    - [x] Add them to smol level.
        - [x] Fix the mouse raycast to voxel field bug.

    - [x] Create camera rails ~~(Just have them be line segments with control points on the ends)~~
        - [x] Have them be something with an OBB that will be used for detection (what point is closest to the OBB), then set the rail direction (this will be an infinite line that the camera will ride on, or more accurately, have the target position (the player) be clamped onto the line).
    - [x] Detect closest camera rail and set camera to it.
        - [x] Use distance from point to line segment algo.
    - [x] Flip the camera after walking towards it for half a second.

- [ ] Build 3 monsters to chase.
- [ ] Create prompt to press 'spacebar' to 'catch' a monster.
- [ ] Build 2 other 'competitors' that are also chasing the 3 monsters.


## Action Items for Otomege part of Game.

- [ ] Blur out background of 3d game.
- [ ] Show UI for selecting 3 buttons for answers and then a textbox for what the other says.
