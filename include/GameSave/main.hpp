#ifndef GAMESAVE_MAIN_INCLUDED
#define GAMESAVE_MAIN_INCLUDED

#include "GameState.hpp"

namespace GameSave {

int writeEverythingToFiles(const char* outputSaveFolderPath, const GameState* state);

int readEverythingFromFiles(const char* inputSaveFolderPath, GameState* state);

int save(const GameState* state);

int load(GameState* state);

}

#endif