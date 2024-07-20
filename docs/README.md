# TPG Engine

The Trigger Primitive Generation (TPG) engine is a separate module that processes raw waveform and returns the generated TPs.
Online, this is completed with an AVX2 implementation in order to keep up with the high data rate from the targeted detectors.
Offline, there are naive C++ and Python implementations that can be used to check the quality of the physics performance.

Each process in the engine is dedicated to completing one task and passing on to the next process.
In configuration, a user can decide the individual process configuration and the order to complete each process.
By the end, the engine body will take the result of the last process and check for any valid TPs.
