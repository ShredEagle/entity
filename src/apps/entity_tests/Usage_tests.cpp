#include "catch.hpp"

#include "Inspector.h"
#include "Components_helpers.h"

#include <entity/EntityManager.h>
#include <entity/Query.h>


using namespace ad;
using namespace ad::ent;


SCENARIO("Adding and removing entities.")
{
    GIVEN("An entity manager.")
    {
        EntityManager world;
        REQUIRE(world.countLiveEntities() == 0);

        WHEN("An entity is added to the manager.")
        {
            Handle<Entity> h1 = world.addEntity();
            REQUIRE(h1.isValid());

            THEN("There is one entity in the manger.")
            {
                CHECK(world.countLiveEntities() == 1);
            }

            WHEN("The entity is erased.")
            {
                {
                    Phase phase;
                    h1.get(phase)->erase();
                }
                THEN("The manager is empty again.")
                {
                    CHECK(world.countLiveEntities() == 0);
                }
                THEN("The handle is invalidated.")
                {
                    CHECK_FALSE(h1.isValid());
                }
            }

            WHEN("Another entity is added to the manager.")
            {
                world.addEntity();
                THEN("Both entities are present in the manager.")
                {
                    CHECK(world.countLiveEntities() == 2);
                }

                WHEN("The first entity is erased.")
                {
                    {
                        Phase phase;
                        h1.get(phase)->erase();
                    }
                    THEN("The manager only contains the one entity.")
                    {
                        CHECK(world.countLiveEntities() == 1);
                    }
                    THEN("The handle to the first object invalidated.")
                    {
                        CHECK_FALSE(h1.isValid());
                    }
                }
            }
        }
    }
}


SCENARIO("Components adding and removing.")
{
    GIVEN("An entity manager with an entity.")
    {
        EntityManager world;

        Handle<Entity> h1 = world.addEntity();
        // The default empty archetype must exist by this time.
        REQUIRE(Inspector<EntityManager>::countArchetypes(world) == 1);

        WHEN("A component (A) is added.")
        {
            const double refValue = 8.6;

            {
                Phase phase;
                Entity e1 = *h1.get(phase);
                REQUIRE_FALSE(e1.has<ComponentA>());

                e1.add(ComponentA{refValue});
            }

            // Scopes the phase to get to e1
            {
                Phase phase;
                Entity e1 = *h1.get(phase);

                THEN("The total number of entities is not changed")
                {
                    CHECK(world.countLiveEntities() == 1);
                }

                THEN("An archetype was created, containing the component.")
                {
                    CHECK(Inspector<EntityManager>::countArchetypes(world) == 2);
                }

                THEN("Component presence can be tested.")
                {
                    CHECK(e1.has<ComponentA>());
                }

                THEN("Component value can be read.")
                {
                    CHECK(e1.get<ComponentA>().d == refValue);
                }

                GIVEN("A query on this component.")
                {
                    Query<ComponentA> queryA{world};

                    THEN("It matches the expected number of entities.")
                    {
                        CHECK(queryA.countMatches() == 1);
                    }
                }

                GIVEN("A query on another component.")
                {
                    Query<ComponentB> queryB{world};

                    THEN("There are no matches.")
                    {
                        CHECK(queryB.countMatches() == 0);
                    }
                }

                GIVEN("A query on both components.")
                {
                    Query<ComponentA, ComponentB> queryBoth{world};

                    THEN("There are no matches.")
                    {
                        CHECK(queryBoth.countMatches() == 0);
                    }
                }
            }

            WHEN("A component (B) is added.")
            {
                const std::string refString{"Here we are."};

                {
                    Phase phase;
                    Entity e1 = *h1.get(phase);
                    e1.add<ComponentB>({.str = refString});
                }

                // Scopes the phase to get to e1
                {
                    Phase phase;
                    Entity e1 = *h1.get(phase);

                    THEN("Both component presence can be tested.")
                    {
                        CHECK(e1.has<ComponentB>());
                        CHECK(e1.has<ComponentA>());
                    }

                    THEN("Both component values can be read.")
                    {
                        CHECK(e1.get<ComponentB>().str == refString);
                        CHECK(e1.get<ComponentA>().d == refValue);
                    }

                    GIVEN("A query on component A.")
                    {
                        Query<ComponentA> queryA{world};

                        THEN("It matches the one entity.")
                        {
                            CHECK(queryA.countMatches() == 1);
                        }
                    }

                    GIVEN("A query on component B.")
                    {
                        Query<ComponentB> queryB{world};

                        THEN("It matches the one entity.")
                        {
                            CHECK(queryB.countMatches() == 1);
                        }
                    }

                    GIVEN("A query on both components.")
                    {
                        Query<ComponentA, ComponentB> queryBoth{world};

                        THEN("It matches the one entity.")
                        {
                            CHECK(queryBoth.countMatches() == 1);
                        }
                    }
                }

                WHEN("Component (A) is removed.")
                {
                    {
                        Phase phase;
                        h1.get(phase)->remove<ComponentA>();
                    }

                    // Scopes the phase to get to e1
                    {
                        Phase phase;
                        Entity e1 = *h1.get(phase);

                        THEN("Component presence can be tested.")
                        {
                            CHECK(e1.has<ComponentB>());
                            CHECK_FALSE(e1.has<ComponentA>());
                        }

                        THEN("Component (B) value can be read.")
                        {
                            CHECK(e1.get<ComponentB>().str == refString);
                        }

                        GIVEN("A query on component A.")
                        {
                            Query<ComponentA> queryA{world};

                            THEN("It matches no entity.")
                            {
                                CHECK(queryA.countMatches() == 0);
                            }
                        }

                        GIVEN("A query on component B.")
                        {
                            Query<ComponentB> queryB{world};

                            THEN("It matches the one entity.")
                            {
                                CHECK(queryB.countMatches() == 1);
                            }
                        }

                        GIVEN("A query on both components.")
                        {
                            Query<ComponentA, ComponentB> queryBoth{world};

                            THEN("It matches no entity.")
                            {
                                CHECK(queryBoth.countMatches() == 0);
                            }
                        }
                    }
                }

                WHEN("Component (B) is removed.")
                {
                    {
                        Phase phase;
                        h1.get(phase)->remove<ComponentB>();
                    }

                    // Scopes the phase to get to e1
                    {
                        Phase phase;
                        Entity e1 = *h1.get(phase);

                        THEN("Component presence can be tested.")
                        {
                            CHECK(e1.has<ComponentA>());
                            CHECK_FALSE(e1.has<ComponentB>());
                        }

                        THEN("Component (A) value can be read.")
                        {
                            CHECK(e1.get<ComponentA>().d == refValue);
                        }

                        GIVEN("A query on component A.")
                        {
                            Query<ComponentA> queryA{world};

                            THEN("It matches the one entity.")
                            {
                                CHECK(queryA.countMatches() == 1);
                            }
                        }

                        GIVEN("A query on component B.")
                        {
                            Query<ComponentB> queryB{world};

                            THEN("It matches no entity.")
                            {
                                CHECK(queryB.countMatches() == 0);
                            }
                        }

                        GIVEN("A query on both components.")
                        {
                            Query<ComponentA, ComponentB> queryBoth{world};

                            THEN("It matches no entity.")
                            {
                                CHECK(queryBoth.countMatches() == 0);
                            }
                        }
                    }
                }

                WHEN("Both components are removed.")
                {
                    {
                        Phase phase;
                        h1.get(phase)->remove<ComponentA>();
                        h1.get(phase)->remove<ComponentB>();
                    }

                    // Scopes the phase to get to e1
                    {
                        Phase phase;
                        Entity e1 = *h1.get(phase);

                        THEN("Component presence can be tested.")
                        {
                            CHECK_FALSE(e1.has<ComponentA>());
                            CHECK_FALSE(e1.has<ComponentB>());
                        }

                        GIVEN("A query on component A.")
                        {
                            Query<ComponentA> queryA{world};

                            THEN("It matches no entity.")
                            {
                                CHECK(queryA.countMatches() == 0);
                            }
                        }

                        GIVEN("A query on component B.")
                        {
                            Query<ComponentB> queryB{world};

                            THEN("It matches no entity.")
                            {
                                CHECK(queryB.countMatches() == 0);
                            }
                        }

                        GIVEN("A query on both components.")
                        {
                            Query<ComponentA, ComponentB> queryBoth{world};

                            THEN("It matches no entity.")
                            {
                                CHECK(queryBoth.countMatches() == 0);
                            }
                        }
                    }
                }
            }
        }
    }
}


SCENARIO("Components duplication.")
{
    GIVEN("An entity manager with an entity.")
    {
        EntityManager world;
        Handle<Entity> h1 = world.addEntity();

        WHEN("A component (A) is added multiple times in the same phase.")
        {
            const double firstValue = 8.6;
            const double secondValue = firstValue + 10.;
            const double thirdValue = secondValue + 10.;

            {
                Phase phase;
                Entity e1 = *h1.get(phase);
                e1.add(ComponentA{firstValue});
                e1.add(ComponentA{secondValue});
            }

            // Scopes the phase to get to e1
            {
                Phase phase;
                Entity e1 = *h1.get(phase);

                THEN("The component was added.")
                {
                    CHECK(e1.has<ComponentA>());
                }

                THEN("The component was stored only once.")
                {
                    Archetype & archetype =
                        Inspector<EntityManager>::getArchetypeHandle<ComponentA>(world).get();
                    CHECK(archetype.countEntities() == 1);
                    CHECK(archetype.verifyConsistency());
                }

                THEN("The last value is stored in the component.")
                {
                    REQUIRE(e1.get<ComponentA>().d == secondValue);
                }
            }

            WHEN("The component is added again, in a distinct phase.")
            {
                {
                    Phase phase;
                    Entity e1 = *h1.get(phase);
                    e1.add(ComponentA{thirdValue});
                }
                // Scopes the phase to get to e1
                {
                    Phase phase;
                    Entity e1 = *h1.get(phase);

                    THEN("The component was added.")
                    {
                        CHECK(e1.has<ComponentA>());
                    }

                    THEN("The component was stored only once.")
                    {
                        Archetype & archetype =
                            Inspector<EntityManager>::getArchetypeHandle<ComponentA>(world).get();
                        CHECK(archetype.countEntities() == 1);
                        CHECK(archetype.verifyConsistency());
                    }

                    THEN("The last value is stored in the component.")
                    {
                        REQUIRE(e1.get<ComponentA>().d == thirdValue);
                    }
                }
            }
        }
    }
}


SCENARIO("Components multiple delete.")
{
    const double firstValue = 8.6;

    GIVEN("An entity manager with an entity.")
    {
        EntityManager world;
        Handle<Entity> h1 = world.addEntity();

        WHEN("A component (A) is added then removed during the same phase.")
        {
            {
                Phase phase;
                Entity e1 = *h1.get(phase);
                e1.add(ComponentA{firstValue});
                e1.remove<ComponentA>();
            }

            // Scopes the phase to get to e1
            {
                Phase phase;
                Entity e1 = *h1.get(phase);

                THEN("The component was NOT added.")
                {
                    CHECK_FALSE(e1.has<ComponentA>());
                }
            }
        }

        WHEN("A component (A) is removed then added during the same phase.")
        {
            {
                Phase phase;
                Entity e1 = *h1.get(phase);
                e1.remove<ComponentA>();
                e1.add(ComponentA{firstValue});
            }

            // Scopes the phase to get to e1
            {
                Phase phase;
                Entity e1 = *h1.get(phase);

                THEN("The component was added.")
                {
                    CHECK(e1.has<ComponentA>());
                }

                THEN("The component was stored only once.")
                {
                    Archetype & archetype =
                        Inspector<EntityManager>::getArchetypeHandle<ComponentA>(world).get();
                    CHECK(archetype.countEntities() == 1);
                    CHECK(archetype.verifyConsistency());
                }

                THEN("The value is stored in the component.")
                {
                    REQUIRE(e1.get<ComponentA>().d == firstValue);
                }
            }
        }


        GIVEN("A component (A) is present on the entity.")
        {
            {
                Phase phase;
                Entity e1 = *h1.get(phase);
                e1.add(ComponentA{firstValue});
            }

            WHEN("The component is removed several times during the same phase.")
            {

                {
                    Phase phase;
                    Entity e1 = *h1.get(phase);
                    e1.remove<ComponentA>();
                    e1.remove<ComponentA>();
                }
                // Scopes the phase to get to e1
                {
                    Phase phase;
                    Entity e1 = *h1.get(phase);

                    THEN("The component is not present anymore on the entity.")
                    {
                        CHECK_FALSE(e1.has<ComponentA>());
                    }

                    THEN("The component is not stored in the Archetype.")
                    {
                        Archetype & archetype =
                            Inspector<EntityManager>::getArchetypeHandle<ComponentA>(world).get();
                        CHECK(archetype.countEntities() == 0);
                        CHECK(archetype.verifyConsistency());
                    }
                }
            }

            WHEN("The component is removed again, in a distinct phase.")
            {
                {
                    Phase phase;
                    Entity e1 = *h1.get(phase);
                    e1.remove<ComponentA>();
                }
                // Scopes the phase to get to e1
                {
                    Phase phase;
                    Entity e1 = *h1.get(phase);

                    THEN("The component is not present on the entity.")
                    {
                        CHECK_FALSE(e1.has<ComponentA>());
                    }

                    THEN("The component is not stored in the Archetype.")
                    {
                        Archetype & archetype =
                            Inspector<EntityManager>::getArchetypeHandle<ComponentA>(world).get();
                        CHECK(archetype.countEntities() == 0);
                        CHECK(archetype.verifyConsistency());
                    }
                }
            }
        }
    }
}


SCENARIO("Erasing entities remove corresponding components from archetypes.")
{
    GIVEN("An entity manager with two entities.")
    {
        EntityManager world;
        Handle<Entity> h1 = world.addEntity();
        Handle<Entity> h2 = world.addEntity();

        GIVEN("A component (B) with distinct values on each entity.")
        {
            const std::string firstValue = "first";
            const std::string secondValue = "second";

            {
                Phase phase;
                h1.get(phase)->add<ComponentB>({firstValue});
                h2.get(phase)->add<ComponentB>({secondValue});
            }

            Archetype & archetype =
                Inspector<EntityManager>::getArchetypeHandle<ComponentB>(world).get();
            REQUIRE(archetype.countEntities() == 2);

            WHEN("The first entity is erased.")
            {
                {
                    Phase phase;
                    h1.get(phase)->erase();
                }
                THEN("The corresponding archetype is down to a single entry.")
                {
                    CHECK(archetype.countEntities() == 1);

                    THEN("The handle to the second element correctly accesses the second value.")
                    {
                        CHECK(h2.isValid());
                        Phase phase;
                        CHECK(h2.get(phase)->get<ComponentB>().str == secondValue);
                    }
                }
            }

            WHEN("The second entity is erased.")
            {
                {
                    Phase phase;
                    h2.get(phase)->erase();
                }
                THEN("The corresponding archetype is down to a single entry.")
                {
                    CHECK(archetype.countEntities() == 1);

                    THEN("The handle to the first element correctly accesses the first value.")
                    {
                        CHECK(h1.isValid());
                        Phase phase;
                        CHECK(h1.get(phase)->get<ComponentB>().str == firstValue);
                    }
                }
            }
        }
    }
}


SCENARIO("Adding components to entities does not break other handles.")
{
    const double firstA = 0.;
    const double secondA = -1.25;

    GIVEN("An entity manager with two entities.")
    {
        EntityManager world;
        Handle<Entity> h1 = world.addEntity();
        Handle<Entity> h2 = world.addEntity();

        GIVEN("A component (B) with distinct values on each entity.")
        {
            const std::string firstB = "first";
            const std::string secondB = "second";

            {
                Phase phase;
                h1.get(phase)->add<ComponentB>({firstB});
                h2.get(phase)->add<ComponentB>({secondB});
            }

            // IMPORTANT: Do not save the direct reference to the archetype, keep the handle:
            // It might expires as new archetypes as added to the store.
            Handle<Archetype> archetypeB =
                Inspector<EntityManager>::getArchetypeHandle<ComponentB>(world);
            REQUIRE(archetypeB.get().countEntities() == 2);

            WHEN("A component (A) is added to the first entity.")
            {
                {
                    Phase phase;
                    h1.get(phase)->add<ComponentA>({firstA});
                }

                THEN("The archetype (B) is down to a single entry.")
                {
                    CHECK(archetypeB.get().countEntities() == 1);
                }

                THEN("The handle to the second entity correctly accesses the second value.")
                {
                    CHECK(h2.isValid());
                    Phase phase;
                    CHECK(h2.get(phase)->get<ComponentB>().str == secondB);
                }

                THEN("The handle to the first entity correctly accesses both values.")
                {
                    CHECK(h1.isValid());
                    Phase phase;
                    CHECK(h1.get(phase)->get<ComponentA>().d == firstA);
                    CHECK(h1.get(phase)->get<ComponentB>().str == firstB);
                }

                WHEN("A component (A) is added to the second entity.")
                {
                    {
                        Phase phase;
                        h2.get(phase)->add<ComponentA>({secondA});
                    }

                    THEN("The archetype (B) is empty.")
                    {
                        CHECK(archetypeB.get().countEntities() == 0);
                    }

                    THEN("The handle to the second entity correctly accesses both values.")
                    {
                        CHECK(h2.isValid());
                        Phase phase;
                        CHECK(h2.get(phase)->get<ComponentA>().d == secondA);
                        CHECK(h2.get(phase)->get<ComponentB>().str == secondB);
                    }

                    THEN("The handle to the first entity correctly accesses both values.")
                    {
                        CHECK(h1.isValid());
                        Phase phase;
                        CHECK(h1.get(phase)->get<ComponentA>().d == firstA);
                        CHECK(h1.get(phase)->get<ComponentB>().str == firstB);
                    }

                }
            }

            WHEN("A component (A) is added to the second entity.")
            {
                {
                    Phase phase;
                    h2.get(phase)->add<ComponentA>({secondA});
                }

                THEN("The archetype (B) is down to a single entry.")
                {
                    CHECK(archetypeB.get().countEntities() == 1);
                }

                THEN("The handle to the first entity correctly accesses the first value.")
                {
                    CHECK(h1.isValid());
                    Phase phase;
                    CHECK(h1.get(phase)->get<ComponentB>().str == firstB);
                }

                THEN("The handle to the second entity correctly accesses both values.")
                {
                    CHECK(h2.isValid());
                    Phase phase;
                    CHECK(h2.get(phase)->get<ComponentA>().d == secondA);
                    CHECK(h2.get(phase)->get<ComponentB>().str == secondB);
                }
            }
        }
    }
}
