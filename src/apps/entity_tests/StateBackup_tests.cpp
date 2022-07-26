#include "catch.hpp"

#include "Components_helpers.h"

#include <entity/Entity.h>
#include <entity/EntityManager.h>
#include <entity/Query.h>


using namespace ad;
using namespace ad::ent;


SCENARIO("Simulation states can be saved and restored.")
{
    GIVEN("An entity manager with an entity with component (A).")
    {
        EntityManager world;
        Handle<Entity> h1 = world.addEntity();

        const double valA = 16;
        {
            Phase phase;
            h1.get(phase)->add<ComponentA>({valA});
        }

        GIVEN("A backup of the initial state.")
        {
            State backup = world.saveState();

            WHEN("The state is modified.")
            {
                {
                    Phase phase;
                    h1.get(phase)->get<ComponentA>().d *= 2;
                }

                WHEN("The initial state backup is restored.")
                {
                    world.restoreState(backup);

                    THEN("The initial value is accessed.")
                    {
                        Phase phase;
                        CHECK(h1.get(phase)->get<ComponentA>().d == valA);
                    }
                }
            }
        }
    }
}
