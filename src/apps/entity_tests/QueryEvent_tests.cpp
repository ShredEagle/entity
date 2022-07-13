#include "catch.hpp"

#include "Components_helpers.h"

#include <entity/Entity.h>
#include <entity/EntityManager.h>
#include <entity/Query.h>


using namespace ad;
using namespace ad::ent;


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


SCENARIO("Queries are notified of entities removed.")
{
    GIVEN("An entity manager with an entity.")
    {
        EntityManager world;
        Handle<Entity> h1 = world.addEntity();

        GIVEN("A query on component (A), with a listener on removed entities.")
        {
            const double valA = 139.642;

            Query<ComponentA> queryA{world};
            std::size_t removeCount = 0;
            queryA.onRemoveEntity([&](ComponentA & a)
                    {
                        ++removeCount;
                        THEN("The component has its value before removal.")
                        {
                            CHECK(a.d == valA);
                        }
                    });

            REQUIRE(removeCount == 0);

            WHEN("A component (A) is added to the entity.")
            {
                {
                    Phase phase;
                    h1.get(phase)->add<ComponentA>({valA});
                }

                THEN("The remove entity listener was not invoked.")
                {
                    CHECK(removeCount == 0);
                }

                WHEN("A component (A) is removed from the entity.")
                {
                    {
                        Phase phase;
                        h1.get(phase)->remove<ComponentA>();
                    }

                    THEN("The removed entity listener was invoked once.")
                    {
                        CHECK(removeCount == 1);
                    }

                    WHEN("Component (A) is removed a second time to the entity.")
                    {
                        {
                            Phase phase;
                            h1.get(phase)->remove<ComponentA>();
                        }

                        THEN("The removed entity listener was not invoked a second time.")
                        {
                            CHECK(removeCount == 1);
                        }
                    }

                    WHEN("A component (A) is added then removed again.")
                    {
                        {
                            Phase phase;
                            h1.get(phase)->add<ComponentA>({valA});
                            h1.get(phase)->remove<ComponentA>();
                        }

                        THEN("The removed entity listener was invoked a second time.")
                        {
                            CHECK(removeCount == 2);
                        }
                    }
                }

            }

            WHEN("A component (B) is added to the entity.")
            {
                {
                    Phase phase;
                    h1.get(phase)->add<ComponentB>({});
                }

                THEN("The removed entity listener is not invoked.")
                {
                    CHECK(removeCount == 0);
                }

                WHEN("Component (B) is removed from the entity.")
                {
                    {
                        Phase phase;
                        h1.get(phase)->remove<ComponentB>();
                    }

                    THEN("The removed entity listener is not invoked.")
                    {
                        CHECK(removeCount == 0);
                    }
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

        GIVEN("A query on component (A), with a listener on removed entities.")
        {
            auto queryA = std::make_unique<Query<ComponentA>>(world);
            std::size_t removeCount = 0;
            queryA->onRemoveEntity([&](ComponentA &)
            {
                ++removeCount;
            });

            WHEN("A component (A) is added to the entity, then removed.")
            {
                {
                    Phase phase;
                    h1.get(phase)->add<ComponentA>({});
                    h1.get(phase)->remove<ComponentA>();
                }
                THEN("The listener was invoked.")
                {
                    CHECK(removeCount == 1);
                }
            }

            WHEN("The query is destructed.")
            {
                queryA = nullptr;

                WHEN("A component (A) is added to the entity, then removed.")
                {
                    {
                        Phase phase;
                        h1.get(phase)->add<ComponentA>({});
                        h1.get(phase)->remove<ComponentA>();
                    }
                    THEN("The listener was not invoked.")
                    {
                        CHECK(removeCount == 0);
                    }
                }
            }
        }
    }
}


SCENARIO("Events are triggered for queries matching several components.")
{
    GIVEN("An entity manager with an entity.")
    {
        EntityManager world;
        Handle<Entity> h1 = world.addEntity();

        GIVEN("A query on component (A, B), with listener on added and removed entities.")
        {
            auto queryAB = std::make_unique<Query<ComponentA, ComponentB>>(world);
            std::size_t addCount = 0;
            std::size_t removeCount = 0;
            queryAB->onAddEntity([&](ComponentA &, ComponentB &)
            {
                ++addCount;
            });

            queryAB->onRemoveEntity([&](ComponentA &, ComponentB &)
            {
                ++removeCount;
            });

            WHEN("A component (A) is added to the entity.")
            {
                {
                    Phase phase;
                    h1.get(phase)->add<ComponentA>({});
                }
                THEN("The listener are not invoked.")
                {
                    CHECK(addCount == 0);
                    CHECK(removeCount == 0);
                }

                WHEN("A component (B) is added to the entity.")
                {
                    {
                        Phase phase;
                        h1.get(phase)->add<ComponentB>({});
                    }
                    THEN("The add listener is invoked.")
                    {
                        CHECK(addCount == 1);
                        CHECK(removeCount == 0);
                    }

                    WHEN("Component (A) is removed from the entity.")
                    {
                        {
                            Phase phase;
                            h1.get(phase)->remove<ComponentA>();
                        }
                        THEN("The remove listener is invoked.")
                        {
                            CHECK(addCount == 1);
                            CHECK(removeCount == 1);
                        }
                    }

                    WHEN("Component (B) is removed from the entity.")
                    {
                        {
                            Phase phase;
                            h1.get(phase)->remove<ComponentB>();
                        }
                        THEN("The remove listener is invoked.")
                        {
                            CHECK(addCount == 1);
                            CHECK(removeCount == 1);
                        }
                    }
                }
            }
        }
    }
}



// Ad 2022/07/13: This test is disabled, because at the moment we decided not to notify of
// pre-existing entities when the listener is installed.
SCENARIO("Existing entities matching a query are signaled when registering a listener.", "[.]")
{
    GIVEN("An entity manager with an entity.")
    {
        EntityManager world;
        Handle<Entity> h1 = world.addEntity();

        const double valA = 16;
        const std::string valB = "bee";

        GIVEN("The entity has components (A, B), and a query on (A, B).")
        {
            {
                Phase phase;
                h1.get(phase)->add<ComponentA>({valA})
                    .add<ComponentB>({valB});
            }

            Query<ComponentA, ComponentB> queryAB{world};

            WHEN("Listener on added entity is installed.")
            {
                std::size_t addCount = 0;

                queryAB.onAddEntity([&](ComponentA & a, ComponentB & b)
                {
                    ++addCount;
                    THEN("The values of the components are as set.")
                    {
                        CHECK(a.d == valA + 1);
                        CHECK(b.str == valB);
                    }
                });

                THEN("The listened is invoked.")
                {
                    CHECK(addCount == 1);
                }
            }
        }
    }
}
