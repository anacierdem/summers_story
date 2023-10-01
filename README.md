# Summer's Story

https://anacierdem.itch.io/summers-story

Requires ARES_DIR to launch on emulator.

Used fonts:

https://fonts.google.com/specimen/Mali

https://www.1001freefonts.com/kg-ten-thousand-reasons.font

Audio assets:

https://freesound.org/people/InspectorJ/sounds/376415/
"Wind, Synthesized, A.wav" by InspectorJ (www.jshaw.co.uk) of Freesound.org

https://freesound.org/people/amholma/sounds/376804/

### Outstanding todo list at the time of jam submission

- [ ] Play a shimmer like audio when an object is reveled. Currently it is possible to miss you found an object.
- [ ] Introduce a third ending for a very high/low number of digs.
- [ ] Fadeout the volume when the environment fades.
- [ ] Create a pickup animation. Scaling the object down and only taking the item when the animation completes will accentuate the pickup feel and prevent unintended pickups when digging with the same key.
- [ ] Play a heartbeat audio when near the chest. It will be an additional clue for it representing the "chest" of the girl.
- [ ] Font is a little too small to read.
- [ ] Record narration.
- [ ] Provide a way to see the text history in case the player misses some clues.
- [ ] Breathing animation for the first person camera.
- [ ] Visual polish. Adjust fog, sky, add lens flares and sun etc.
- [ ] When digging with the spade, enable depth test for it to create the illusion that it is entering the sand. I have tried this but for some camera angles the animation started below the sand, so it needs more thought.
- [ ] Add an occasional glimmer to distant items that are not below the sand.
- [ ] Collisions are delayed a frame because of the order of the current checks. When you let the controls go, you teleport to the actual position you should be at. Fix that bug by splitting necessary functions and arranging their order.
- [ ] Provide a feedback on how many times the played used the shovel to improve the feeling that it is not a good thing. A down counter will be too obvious. Just counting up might be ok with an appropriate label?
- [ ] I think there was a bug where the view was drifting without a controller plugged in, check.
- [ ] Optimize the terrain with display lists by recording/rendering it only when we update it.
- [ ] Add footstep audio.
- [ ] When close by to the magnet underground provide rumble feedback. Also potentially for each dig.
- [ ] There is a bug on libdragon for distant fog. It seem to loose its effectivenes for some viewpoint angles.