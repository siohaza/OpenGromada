#ifndef VERSION_H
#define VERSION_H

// The engine's own version, distinct from the version of the game data it
// loads. CMake passes this in from project(OpenGromada VERSION ...); the
// fallback keeps non-CMake builds (premake) compiling - keep the two in sync.
#ifndef OPENGROMADA_VERSION
#define OPENGROMADA_VERSION "0.2.0"
#endif

#endif
