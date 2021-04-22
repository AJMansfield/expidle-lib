This is a python C-API implementation of numerical routines for opimizing the idle game [Exponential Idle](https://play.google.com/store/apps/details?id=com.conicgames.exponentialidle).

This was originally started as an exercise to learn how to use the Python C api, after a native python implementation of this datatype proved far too slow for some of the number-crunching optimizations I wanted to try.

TODO:
- fix stringification ops
- add unit tests and fix (or at least document) arithmetical edge cases
- release the GIL
- add numpy vectorization routines
- implement compatibility with numba JIT compilation
- reproduce the game's cost functions
- parse the game's save data
- re-implement the timestep logic
- implement a parser/lexer/AST for the game's expressions
- implement optimization routines to crunch through the game

