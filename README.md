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

    - [x] Fix camera mistakes in the larger map.

- [x] Quality of life improvements
    - [x] Change characters to be billboard sprite.
    - [ ] ~~Add camera collision. Use spherecast.~~
    - [ ] ~~Create exporter for voxelfield to obj or gltf model so can be created as template for the display model.~~
    - [ ] ~~Use the trigger voxels to display/hide different parts of the display model.~~

- [ ] Build 3 monsters to chase.
    - [x] Have monsters do a check every frame to see what is a "safe" direction to go in when getting chased.
        - [x] Figure out formula for seeing if raycast direction hits the player.
        - [x] Raycast in 16 directions, and if any walls are hit (not ramps), then that's a bad direction to go.
        - [x] Mix in the player's direction (if within 20 units and visible, then move).
        > NOTE: will need to do more work on this, but it's okay except monster doesn't want to go down forks bc of the bias of a right turn being "closer" to the player than going straight down the hallway. Need to figure out how to avoid this.


## Action Items for reworked game design (3-phase game)

- [x] Phase 1: "Find the nest egg"
    > 3 girls you can date are hidden in the room somewhere, and you, along with 2 other competitors, are trying to find one of them first. If one of the contestants finds a girl, then the girl and the contestant gets whisked away to the phase 2 as the dating hotseat. The other competitors get whisked away as booby trap operators.
    - [x] Object to uncover
        > Will be one of 10 different kinds of objects, since all this will be storage objects in an old ballroom-kinda feel. They'll be read in as objects with the following params:
            - Model path
            - Still, covered animation (rustling or not)
            - Shifting/rustling animation
            - Uncovered animation (have cloth be scaled to pretty much 0 and hidden in one of the legs of the furniture or something)
            - Collision data (# of collision cubes and the transforms and extents of the cubes)
            - Interaction sphere (offset origin and radius)
            - Interaction icon position (where the icon will pop up, showing that this is the one the person is choosing)
            - How long to hold E to uncover the object (random within range).
    - [x] Hold E to uncover an object.
        - [x] Show prompt that can uncover.
        - [x] Show cursor of the object is getting uncovered.
        - [x] Progress dial.
    - [x] Timer for 2:30 minutes in corner.
    - [x] UNRELATED: get simple main menu framework set up (open first.ssdat)
    - [x] Show uncovered girl.
        - [x] Girl model should be invisible until pulling off the covers.
        - [ ] ~~Girl is shown and person who uncovered girl gets whisked away with girl.~~
        - [ ] ~~Remaining contestants are shown getting whisked away too.~~
        - [x] Transitions from phase0 to phase1
    - [ ] BUG: no covered items spawn sometimes.

- [ ] Phase 2: "Hallway of Love"
    - [x] Contestant A logic
        - [x] Moving (just like previous phase)
        - [x] If get hit by booby trap, get stunned for 1 second (tune).
        - [x] If touch date, start Phase 3
            - [x] If fail Phase 3, then get stunned for 1.5 seconds (tune).
        - [x] Camera follows Contestant A
    - [x] Date logic
        - [x] Moves forward (tune by how much)
        - [x] If get hit by booby trap, get stunned for 0.75 seconds (tune).
        - [x] If reach end, go to Phase 1
    - [ ] Contestant B(s) logic
        - [ ] Stand at next open booby trap operator booth that is in front of contestant A.
        - [ ] Press X to set off the trap.
        - [ ] Move to next open booby trap operator booth.

- [ ] Phase 3: "Grab hand speed date"
    - [x] Create render texture that is the screenspace size. This is "Freeze Frame Render Texture".
        - [x] Blit it
        - [x] Blit it before it gets tonemapped.
    - [x] Draw render texture as a backdrop.
    - [ ] 4 directions of input lead to different dialog options
        - [ ] Bottom one is always "Will you go on a date with me?"
            - NOTE: never ask this on the first one bc it's supposed to be a pick-up line (but you CAN)
        - [ ] Pressing bad line will get contestant rejected, sending back to Phase 2
        - [ ] Pressing good line will get a good response from Date, and another set of options.
        - [ ] Pool of dialogue from Date and options for dialogue for player.
    - [ ] "Will you go on a date with me?"
        - [ ] Random amount of thinking (very very soon and it's immediate NO. Early and it's very long, later and it's short (yes, there can be a short "no thanks"))
    > NOTE: Only the player enters phase 3. Other contestants just talk for a bit, then it results in either getting rejected after a random amt of time or hearing the contestant pop the question, Date thinks for random amt of time and then reject or accept.

How do we handle if a contestant other than the player gets a date success?
    GAME OVER, you LOSE, type deal.
How do we handle if there are no more places to uncover or <# of dates?
    This will not happen. When you reset to Phase 1, the whole entire room gets reset and everything is covered up again.

- [ ] Phase 0: View Tarot cards
    - [ ] 10 cards show up, and 3 are flipped over
    - [ ] Move to the center of the screen and they show the bios of the 3 girls.
    - [ ] Press any key to continue to Phase 1.


## Action Items for Otomege part of Game.

- [ ] Blur out background of 3d game.
- [ ] Show UI for selecting 3 buttons for answers and then a textbox for what the other says.
