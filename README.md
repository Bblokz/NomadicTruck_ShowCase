## This is a public showcase of some of my work in Unreal Engine
This repo contains some of the backend source files.
This repo does not contain any base classes nor derived blueprints or assets.
The original directory layout has been changed in this repo to find the appropriate files easier.

### Main show case
This project shows how my nomadic truck mechanic works which relys on parallelism in several ways.
1. The truck is able to convert into a buidling and back into a truck, for this a procedural animation is made using material slots.
This happens completely automatically, as I do not want to manually create each animiation I use cpp to calculate the 
relative offsets from the pivot of the building mesh of each material slot.
This only happens once in the game and is cached afterwards.

Since my meshes are nanite-based they can be very complex which makes these calculations very expensive.
To solve this I use parallelism to calculate the offsets on a different thread.
When the calculation is complete the results are writen back on the main thread to avoid race conditions.
In the exceptional case that the background thread does not finish the calculations in time the animation falls back
to random smoke locations.

2. Memory footprint optimization and improved loading times.
The expanded trucks can make use of buiding expansion to add extra meshes to their construction.
From a design perspective, this is very useful as it allows me to create a lot of variation as well as reuse assets.
However, this can be very expensive in terms of memory and loading times.
Hard referenced assets force the CPU to load in every single possible expansion, even if the player will never use them.
To solve this I use a custom asset manager that loads in the assets only when they are needed, see ARTSAsyncSpawner.
This spawner makes use of handle which allows for only loading in one assets at the time.

This creates an interesting problem on itsown as now the player does not have direct access to the building meshes.
We do not want to make the player wait, instead I load a preview mesh which is a highly optimized version of the total
building mesh in sync with the main thread. This mesh can be placed as a preview which is replace by the actual building
once the background loading is complete.