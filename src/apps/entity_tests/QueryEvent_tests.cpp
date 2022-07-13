#include "catch.hpp"

#include "Components_helpers.h"

#include <entity/Entity.h>
#include <entity/EntityManager.h>
#include <entity/Query.h>


using namespace ad;
using namespace ad::ent;


// TODO removed
// TODO notification of pre-existing entities.
SCENARIO("Queries are notified of entities added.")
{
    GIVEN("An entity manager with an entity.")
    {
        EntityManager world;
        Handle<Entity> h1 = world.addEntity();

        GIVEN("A query on component (A), with a listener on added entities.")
        {
            const double valA = 139.642;

            Query<ComponentA> queryA{world};
            std::size_t addCount = 0;
            queryA.onAddEntity([&](ComponentA & a)
                    {
                        ++addCount;
                        CHECK(a.d == valA);
                    });

            REQUIRE(addCount == 0);

            WHEN("A component (A) is added to the entity.")
            {
                {
                    Phase phase;
                    h1.get(phase)->add<ComponentA>({valA});
                }

                THEN("The added entity listener was invoked once.")
                {
                    CHECK(addCount == 1);
                }

                WHEN("A component (A) is added a second time to the entity.")
                {
                    {
                        Phase phase;
                        h1.get(phase)->add<ComponentA>({valA});
                    }

                    THEN("The added entity listener was not invoked a second time.")
                    {
                        CHECK(addCount == 1);
                    }
                }

                WHEN("A component (A) is removed then added back.")
                {
                    {
                        Phase phase;
                        h1.get(phase)->remove<ComponentA>();
                        h1.get(phase)->add<ComponentA>({valA});
                    }

                    THEN("The added entity listener was invoked a second time.")
                    {
                        CHECK(addCount == 2);
                    }
                }
            }

            WHEN("A component (B) is added to the entity.")
            {
                {
                    Phase phase;
                    h1.get(phase)->add<ComponentB>({});
                }

                THEN("The added entity listener is not invoked.")
                {
                    CHECK(addCount == 0);
                }
            }
        }
    }
}


SCENARIO("The events are only listened as long as the query is alive.")
{
    GIVEN("An entity manager with an entity.")
    {
        EntityManager world;
        Handle<Entity> h1 = world.addEntity();

        GIVEN("A query on component (A), with a listener on added entities.")
        {
            auto queryA = std::make_unique<Query<ComponentA>>(world);
            std::size_t addCount = 0;
            queryA->onAddEntity([&](ComponentA &)
            {
                ++addCount;
            });

            WHEN("A component (A) is added to the entity.")
            {
                {
                    Phase phase;
                    h1.get(phase)->add<ComponentA>({});
                }
                THEN("The listener was invoked.")
                {
                    CHECK(addCount == 1);
                }
            }

            WHEN("The query is destructed.")
            {
                queryA = nullptr;

                WHEN("A component (A) is added to the entity.")
                {
                    {
                        Phase phase;
                        h1.get(phase)->add<ComponentA>({});
                    }
                    THEN("The listener was not invoked.")
                    {
                        CHECK(addCount == 0);
                    }
                }
            }
        }
    }
}
