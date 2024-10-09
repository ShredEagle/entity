#include "catch.hpp"

#include "Components_helpers.h" 

#include <entity/Blueprint.h>
#include <entity/Entity.h>
#include <entity/EntityManager.h>
#include <entity/Query.h>

using namespace ad;
using namespace ad::ent;


SCENARIO("Creating a entity from a blueprint")
{
    GIVEN("An entity manager with a blueprint and an entity created from a blueprint")
    {
        EntityManager world;
        Handle<Entity> h1 = world.addBlueprint();
        {
            Phase phase;
            h1.get(phase)->add<ComponentA>({1.0});
        }

        GIVEN("A query on ComponentA")
        {
            Query<ComponentA> q{world};

            THEN("Initially the query is empty")
            {
                REQUIRE(q.countMatches() == 0);
                REQUIRE(h1.get()->get<ComponentA>().d == 1.0f);
            }


            WHEN("Creating a entity from a blueprint")
            {
                Handle<Entity> h2;
                h2 = world.createFromBlueprint(h1, "hello");

                THEN("The entity has the same component as the blueprint")
                {
                    CHECK(q.countMatches() == 1);
                    CHECK(h2.isValid());
                    CHECK(h2.get()->get<ComponentA>().d == 1.0f);
                    CHECK(h2.get()->has<Blueprint>() == false);
                }
            }
        }
    }
}
