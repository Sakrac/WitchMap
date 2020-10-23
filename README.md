# WitchMap
 Map editor created for a specific game

 ## Points of clarification
 * Currently tile size is 32x32, additional UI for adjusting is planned
 * Map size is 10x32, UI exists but doesn't actually modify
 * There is no Commodore 64 specific code in this editor
 * Layers are currently hard coded to background (graphics), properties (just values) and overlay (placements of temporary characters and generic sprites)
 * This is an early version
 * Levels are saved in a .cfg like format, to load them for processing bring LoadLevel.cpp into a separate project. You also need std and struse folders along with files.*, config.*

 ## Reuse
 * Feel free to use this code as a starting point for other tile map based projects. It was created simply because I could not place things conveniently in other editors in a way that made sense for my own game.
