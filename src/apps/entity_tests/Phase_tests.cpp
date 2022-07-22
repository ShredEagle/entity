#include "catch.hpp"

#include "Components_helpers.h"

#include <entity/Entity.h>
#include <entity/EntityManager.h>


using namespace ad;
using namespace ad::ent;


SCENARIO("Phases defer some modifications.")
{
    GIVEN("An entity manager.")
    {
        EntityManager world;

        GIVEN("An entity added to the manager.")
        {
            REQUIRE(world.countLiveEntities() == 0);

            Handle<Entity> h1 = world.addEntity();
            CHECK(world.countLiveEntities() == 1);

            WHEN("A component (A) is added.")
            {
                const double refValue = 10E6;

                {
                    Phase phase;
                    Entity e1 = *h1.get(phase);
                    e1.add(ComponentA{refValue});

                    THEN("The component is not visible while the phase is not over.")
                    {
                        CHECK_FALSE(e1.has<ComponentA>());
                    }
                }

                THEN("The component becomes visible when the phase is over.")
                {
                    Phase phase;
                    Entity e1 = *h1.get(phase);
                    CHECK(e1.has<ComponentA>());
                }
            }
        }
    }
}
