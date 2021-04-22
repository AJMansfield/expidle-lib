This is a python C-API implementation of numerical routines for opimizing the idle game [Exponential Idle](https://play.google.com/store/apps/details?id=com.conicgames.exponentialidle).

This was originally started as an exercise to learn how to use the Python C api, after a native python implementation of this datatype proved far too slow for some of the number-crunching optimizations I wanted to try.

TODO:
- fix stringification ops
- add unit tests and fix (or at least document) arithmetical edge cases
- reproduce the game's cost functions
- parse the game's save data
- implement native numpy vectorization stuff (and release the GIL)
- make the routines more vectorization-friendly
- re-implement the game's timestep logic in a vectorizable way (to allow cheaply comparing alternative policies)
- implement an AST corresponding to the game's expression parsing, to use for optimization routines seeking to find efficient prestige/supremacy formulae

