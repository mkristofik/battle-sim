# Magic System Ideas

Here are some ideas on how the spellcasting system should work.  This is quite obviously inspired by a certain collectible card game.

## Core Concepts

- Shared mana pool using the four classical elements: Air, Earth, Fire, and Water.
- Commanders have an opportunity to cast a spell at the beginning of each round.
- One spell cast per commander per round.
- Going first should be an advantage, but not too much of one.  There should always be an opportunity for a response to a spell cast.
- Not sure yet whether Colorless mana costs should be allowed.  That may be too unbalancing given the shared mana pool.

![concept art](https://raw.github.com/mkristofik/battle-sim/master/magic_concept.jpg)

## Idea #1

- Each commander generates 1 mana per round of the type that matches his alignment (Knight=Air, Barbarian=Earth, Warlock=Fire, Sorceress=Water).
- Mana is not generated until one of his units becomes active.  This ensures at least 1 mana of the aligned type is always available.
- The terrain generates 1 mana per round (Grass=Water, Dirt=Earth, Swamp=Fire, Snow=Air, Ocean=Water x 2, Desert=None).
- Alternatively, note that there are 6 terrain types and 6 possible pairs of elements.
- Enchantment spells should require a continuous use of mana (e.g., 1 Air mana per round).
- Instant spells use up their mana cost immediately.
- Enchancements can be canceled as part of the beginning-of-round commander action.  Their upkeep cost should be refunded to the mana pool.
- Enchantments are canceled whenever the upkeep cost cannot be paid or the target unit is killed.

*[ed: Trying to define some simple rules gets complicated very quickly.  No wonder that card game has a squillion rules.]*

## Idea #2

- Combine the battle game with the randomly-generated adventure map.
- Each region generates X mana per day according to terrain type.
- The shared mana pool in battle contains whatever the current mana level is for that region.
- Commanders do not generate any mana on their own.
