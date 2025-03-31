# Entity

Entity-Component-System (ECS) library based on archetypes.
It serves as a framework for implementing gameplay in video games
but is also suitable for other types of simulations.

It provides a data-oriented approach for efficiently operating on large numbers of entities
while maintaining a programmer-friendly API and ensuring high runtime performance.

## Build (with Conan 2)

* Ensure [the pre-requisites](https://github.com/ShredEagle/public_build_info?tab=readme-ov-file#requirements) are met.
* Clone:
    ```bash
    git clone --recurse-submodule https://github.com/ShredEagle/entity.git
    cd entity
    ```
* Build:
    ```bash
    conan build ./conan/
    ```

## Build System and Dependencies

The project uses CMake for its build scripts, which is sufficient for building the project.
However, CMake does not manage upstream dependencies.

To address this, a Conan recipe is provided on top of the CMake scripts.
It handles dependency management and facilitates the integration inside a dependency graph.
