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
    - [x] BUG: no covered items spawn sometimes.

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
    - [x] Set off hazards when player gets near to them (random).

- [x] Phase 3: "Grab hand speed date"
    - [x] Create render texture that is the screenspace size. This is "Freeze Frame Render Texture".
        - [x] Blit it
        - [x] Blit it before it gets tonemapped.
    - [x] Draw render texture as a backdrop.
        - [f] Blit the former snapshot image to the newly created one when resizing the window.
    - [x] Create simple UI framework.
        > See the implementation in Textbox.h/.cpp
        - [x] Image quad.
        - [x] Color quad.
    - [x] 4 directions of input lead to different dialog options
        - [x] Bottom one is always "Will you go on a date with me?"
            - NOTE: never ask this on the first one bc it's supposed to be a pick-up line (but you CAN)
        - [x] Pressing bad line will get contestant rejected, sending back to Phase 2
        - [x] Pressing good line will get a good response from Date, and another set of options.
        - [x] Pool of dialogue from Date and options for dialogue for player.
    - [x] "I don't think we're right for each other."
        - [x] Ends the dating interface.
    - [x] "Will you go on a date with me?"
        - [x] Random amount of thinking (very very soon and it's immediate NO. Early and it's very long, later and it's short (yes, there can be a short "no thanks"))
    > NOTE: Only the player enters phase 3. Other contestants just talk for a bit, then it results in either getting rejected after a random amt of time or hearing the contestant pop the question, Date thinks for random amt of time and then reject or accept.

- [x] Add in Dialogue options for dating sim
    - [x] Disable questions from contestant as they ask.
    - [x] Disable questions from date as they ask.
        - [x] Don't disable the last question (marked as lastQuestion) since this will just be "are you enjoying the mixer?"
- [x] For no more questions to ask, gray out all 4 question slots and just make player answer rejection or date ask question
    - [x] If ask on date get rejected subtract VERY_BAD trust level
    - [x] If say not feasible, then no change to trust level
- [x] Fix BUGS
    - [x] Dating interface disabled text selections don't immediately get grayed out.
    - [x] Dating interface doesn't get text regenerated when text to regenerate is set to "".
- [x] Create wrapped text.
    - [x] Initial wrapped text.
    - [f] Make text break on words.
    - [x] Apply text rendered size to size of speech box.
        - [x] Move x position of character speech bubble based off of the size of the rendered text.
        - [x] Move the 6 speech selection bubble positions based off of their respective heights.
    - [x] Apply to date character speech bubble.
    - [x] Apply to contestant speech bubble.
- [ ] ~~Add text showing if trust level got better or not in the dating sim~~
    - [ ] ~~Add flowers particle effect for good reaction.~~ ~~Tint image green then fadeaway for good reaction.~~ 
    - [ ] ~~Add disgusted particle effect for bad reaction.~~ ~~Tint image red then fadeaway for bad reaction.~~ Squish and stretch.
    - [x] Add the corresponding sounds for the reactions.
        > Now you can use where those sfx are being played as where to add in the reactions.
- [x] Add text showing how far away from date in hallway
- [x] Game over and game win screens
    - [x] Correctly shows the correct date pic for the game win screens.

- [x] Phase 0: View Tarot cards
    - [x] 10 cards show up, and 3 are flipped over
    - [x] Move to the center of the screen ~~and they show the bios of the 3 girls.~~
    - [x] Show the bios in text underneath the cards.
        - [f] Have the bios all show up after all of the cards are gone.
    - [x] Press any key to continue to Phase 1.

- [ ] Create 3d interior for hallway and ballroom
    - [f] Create 3d exporting tool for the voxel fields.
    - [x] Create blockout for the hall and ballroom
    - [x] Remove the contestant B's or change where they spawn in chase phase.
    - [ ] Add more windows and a doorway for ballroom.
    - [ ] Add dining table with chairs between contA and date at the chase hall.
    - [f] Add door that contA can go thru that's really close (left) that ends the chase hall early.

- [x] Add in music for main menu
- [x] Add in music for chase hallway.
    - [x] Add music
    - [x] Display ui quads for the ready go!
    - [x] Disable character and date movement until the go!
    - [x] Add sfx for date getting to door.

- [x] Add sfx for guillotine.
- [x] Add sfx for character getting stunned.
- [x] Add sfx for card flip in main menu.

- [x] Add credits page (just a bunch of ui quads)
    - [x] TimoEngine
    - [x] Vulkan
    - [x] FMOD
    - [x] SDL
    - [x] Jolt Physics
    - [x] Special thanks
        - [x] ArsenalFortitude
        - [x] Sascha Willems
        - [x] Joey de Vries (learnopengl.com)
        - [x] vblanco20-1 (vkguide.dev)
        - [x] Wendy's
        - [x] Timo's toe
        - [x] Oreilly's Auto Parts
        - [x] NomNom for allowing me time to work on this game that I did not utilize.
        - [x] Maverick for my donut.

- [x] ~~Add in start game and exit game buttons for main menu~~ Just add text
    - [x] "Press Spacebar to draw tarot cards for dates."
    - [x] "Press Esc to exit anytime."

- [x] Add bgm for searching time.
    - [x] Add sfx for finding a date.
    - [x] Add sfx for time expiring.

- [x] Add assets for characters etc.
    - [x] Nefertiti
    - [x] Ophelia
    - [x] Isabella
    - [x] MC
    - [x] Title screen
    - [x] Dating background.

- [x] Anim system for characters.

- [x] Sfx for winning the game.
- [ ] You win and you lose screens.
    - [ ] Win
    - [x] Lose

- [x] Write prompts for the 3 ppl.
    - [x] Fix J and j from the word "reject"

- [x] Hides guillotine better.

- [ ] Final wrapup.
    - [x] Fix J and j spacing.
    - [x] Disable imgui on startup.
    - [x] Set startup cam to game cam instead of freecam.
    - [x] Start game in fullscreen (use f11 to toggle screen size).
    - [x] Allow support for arrow keys too.
    - [x] Add new ready start icons.
    - [ ] Insert date profiles into dating simulator.
    - [ ] Add silhouettes to tarot cards.
    - [ ] Finish environment art.
    - [ ] FIX BUG: crashing when game over.
    - [ ] EXTRA: implement volumetric lighting.


## Things to do if I have time before the deadline.

- [ ] Fix reset state when restarting game.
    - [ ] Skybox is the picture instead of normal skybox.
    - [ ] Player cannot move for some reason.
    - [ ] Player is in hallway for some reason.
- [ ] Have text selection boxes be the same width (max) and then center the text.
- [ ] Change text boxes to be zelda style.


## Action Items for Otomege part of Game.

- [x] Blur out background of 3d game.
- [x] Show UI for selecting 3 buttons for answers and then a textbox for what the other says.


How do we handle if a contestant other than the player gets a date success?
    GAME OVER, you LOSE, type deal.
How do we handle if there are no more places to uncover or <# of dates?
    This will not happen. When you reset to Phase 1, the whole entire room gets reset and everything is covered up again.
