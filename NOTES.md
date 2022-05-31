Plan: make it easier to prototype and try 2d puzzle games
- Have a generic way of handling objects such that we can have a graphical editor for any game
- Separate the game rules

Experiment:

- layered (jam) coding style, layer by layer, change one layer at a time, introduce new ideas in each new layer


Next:
- make editor
- undo

Done:
- draw portals


# Puzzle ideas

You need a block that has a portal on a specific side, so you need to switch it
with another block somewhere else.

Button of the door is on one side only. Cannot be accessed from the other side.
You need to create some kind of tunnel to the other side, or to have a block
ready for the button.

Blocks cannot be displaced from walls.

Use the same tunnel multiple times.


# Remember

Keep scope small
To a rough game but complete
Polish after (graphics, animation, sound, optimisation)


# Story

Nuketown for quantum bombs, need to get items in houses.
A quantum bomb tangled some objects and created portals to teleport with.
A house is a level.
Player goes in the house, gets the item and comes out.
A house can have multiple ways in/out.


# Mechanic

Either portals are on one side, or it's the entire object.

Remove switches for now.

A tile can have multiple items:
- open door can reveal a button on the floor
- box on top of button

Player has to go back to the beginning. Maybe not a good mechanic. It just sounds like a lazy way to make it more difficult.

Pushing a block through a portal rotates it if the 'out' direction is different than the 'in'.

Player can create new portals by pushing it onto a special "wall" that has the color of the tunnel.
Only blocks have portals. Then they can be rotated and used to limit how the player can push it.

Player can place one portal on the wall so that he can push the block from the other portal.

Block pushed against another block can make the other block go through the portal ? Might be too complicated to understand or to use.

Player tunnels recursively and can use that to access difficult spaces.


# Implementation

Player is considered as a regular item, but controllable.
