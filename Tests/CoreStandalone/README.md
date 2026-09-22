# Core standalone tests

`MISSplitCore` only uses a handful of UE container/math types, so it can be compiled
without the engine against the minimal shim in `Shim/CoreMinimal.h`.
The shim is never seen by UBT (it lives outside `Source/`).

```bash
g++ -std=c++17 -O2 -Wall -Wextra \
  -I Tests/CoreStandalone/Shim -I Source/MeshIslandSplitter/Private \
  Source/MeshIslandSplitter/Private/MISSplitCore.cpp Tests/CoreStandalone/CoreTests.cpp \
  -o core_tests && ./core_tests
```

If the core starts using another UE type, add the smallest possible stand-in to the shim.
