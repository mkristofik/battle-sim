# Heroes Battle Sim

This game was inspired by the turn-based combat of the Heroes of Might and Magic and Disciples fantasy strategy games.  It is a sandbox to experiment with various battle mechanics to learn what's fun, and what isn't.  I've deliberately limited the options at each turn to help a computer player understand them.  If the existing Heroes combat is like chess, this project is like checkers.

## Features

- Mouseover highlights indicating move and attack targets.
- Attack and defend animations.
- Labels indicating unit size.
- Random damage rolls, separate values for melee and ranged attacks.
- Units have hit points, animate and fade out when killed.
- Initiative stat determines the order in which units get to act.
- Scrolling battle log to record every attack action.
- Report a winner when one side is defeated.
- Commanders have influence over the strength of their units.
- Units can retaliate against melee attacks once per round.
- Pressing the 'S' key will skip a unit's turn.

![screenshot](https://raw.github.com/mkristofik/battle-sim/master/screenshot.jpg)

There's a [demo video](http://youtu.be/2TbIJfgpM-o) too.

## Core Principles

- Be simple enough that an AI player can play well.
- All units move 1 hex per turn by default.
- All attacks must do at least *some* damage.  No abilities may cause an attack to miss entirely.
- Most creatures should have a special ability.  Special attacks take the place of a normal (boring) attack.
- Special abilities must be **special**.  Anything that reduces to a numerical increase in damage or hit points is not special.
- Magic should be **magical**.  Direct-damage spells (e.g., Fireball, Lightning) are boring, and not very effective as stack sizes get larger.
- Good spells should have a large-scale effect.  Spells that affect the entire battlefield indiscriminantly are even better.

## Artwork Notes

Here are the decisions I've made to simplify the code that deals with art assets.

- Hexes are 72x72 pixels.  Anything designed to fit in one hex should use the same size.
- Animated images are stored in a sprite sheet with the first frame on the left and no padding between frames.  Frames are assumed to be 72x72.
- Projectiles are drawn with their trailing edges in the center of the hex and facing East.  The image rotation code goes counter-clockwise, like angles on the Unit Circle.
- Commander portraits are 200x200 pixels.

## Requirements

[SDL 1.2.15](http://www.libsdl.org/)  
[SDL\_image 1.2.12](http://www.libsdl.org/projects/SDL_image/)  
[SDL\_ttf 2.0.11](http://www.libsdl.org/projects/SDL_ttf/)  
[SDL\_mixer 1.2.12](http://www.libsdl.org/projects/SDL_mixer/)  
[SDL\_gfx 2.0.24](http://www.ferzkopp.net/joomla/content/view/19/14/)  
[DejaVuSans](http://dejavu-fonts.org/wiki/Main_Page) font (drop the .ttf file
in the top-level directory)

## Special Thanks To
[Battle for Wesnoth](www.wesnoth.org) for all of the art and sound assets.

## License

Copyright 2013 by Michael Kristofik  
The license for code, art, and sound assets is described in the file
COPYING.txt.

## Contact Info

[Michael Kristofik](mailto:kristo605@gmail.com)
