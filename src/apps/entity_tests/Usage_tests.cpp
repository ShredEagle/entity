#include "catch.hpp"

#include "Components_helpers.h"

#include <entity/EntityManager.h>
#include <entity/Query.h>


using namespace ad;
using namespace ad::ent;


namespace ad {
namespace ent {

template <class T_inspected>
class Inspector;

template <>
class Inspector<EntityManager>
{
public:
    static std::size_t countArchetypes(const EntityManager & aEntityManager)
    { return aEntityManager.mArchetypes.size(); }

    template <class... VT_components>
    static Archetype & getArchetype(EntityManager & aEntityManager)
    { return aEntityManager.getArchetype(getTypeSet<VT_components...>()); }
};


} // namespace ent
} // namespace ad


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


SCENARIO("Components manipulation.")
{
    GIVEN("An entity manager with an entity.")
    {
        EntityManager world;
        REQUIRE(Inspector<EntityManager>::countArchetypes(world) == 0);

        Handle<Entity> h1 = world.addEntity();
        // The default empty archetype becomes present as soon as one entity was added
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

            WHEN("A second component (B) is added.")
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

        WHEN("A component (A) is added multiple times.")
        {
            const double firstValue = 8.6;
            const double secondValue = firstValue + 10.;

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
                        Inspector<EntityManager>::getArchetype<ComponentA>(world);
                    CHECK(archetype.countEntities() == 1);
                    CHECK(archetype.verifyConsistency());
                }

                THEN("The last value is stored in the component.")
                {
                    REQUIRE(e1.get<ComponentA>().d == secondValue);
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
                Inspector<EntityManager>::getArchetype<ComponentB>(world);
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
