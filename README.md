# Heroes Battle Sim

This game was inspired by the turn-based combat of the Heroes of Might and Magic and Disciples fantasy strategy games.  It is a sandbox to experiment with various battle mechanics to learn what's fun, and what isn't.

## Core Principles

- Be simple enough that an AI player can play well.
- All units move 1 hex per turn by default.
- All attacks must do at least *some* damage.  No abilities may cause an attack to miss entirely.
- Most creatures should have a special ability.  Special attacks take the place of a normal (boring) attack.
- Special abilities must be **special**.  Anything that reduces to a numerical increase in damage or hit points is not special.
- Magic should be **magical**.  Direct-damage spells (e.g., Fireball, Lightning) are boring, and not very effective as stack sizes get larger.
- Good spells should have a large-scale effect.  Spells that affect the entire battlefield indiscriminantly are even better.

## Requirements

[SDL 1.2.15](http://www.libsdl.org/)  
[SDL\_image 1.2.12](http://www.libsdl.org/projects/SDL_image/)  
[SDL\_ttf 2.0.11](http://www.libsdl.org/projects/SDL_ttf/)  
[SDL\_mixer 1.2.12](http://www.libsdl.org/projects/SDL_mixer/)  
[DejaVuSans](http://dejavu-fonts.org/wiki/Main_Page) font (drop the .ttf file
in the top-level directory)

## Special Thanks To
[Battle for Wesnoth](www.wesnoth.org) for all of the art assets.  They can
draw, and I can't.

## License

Copyright 2013 by Michael Kristofik  
The license for code and art assets is described in the file COPYING.txt.

## Contact Info

[Michael Kristofik](mailto:kristo605@gmail.com)