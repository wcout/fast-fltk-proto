# FLTK Fast Prototyping Demo
`v0.1: 2017/01/22`

A simple *proof of concept* of an idea how to "rapid prototype"
a single source `FLTK` application using `FLTK` itself as an IDE.

Opens an editor widget, where you can type a `FLTK` source code,
which is **live compiled and executed** during typing. In case of
compiler errors, the first error is displayed in a box below
the edit field and the source line highlighted.

**Tested on Linux only.**

Made compilable for Windows (using mingw) too, but not tested
(probably will need additional work).
