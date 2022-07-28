#include "catch.hpp"

#include "Components_helpers.h"
#include "Inspector.h"

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
                State backupModification = world.saveState();

                WHEN("The initial state backup is restored.")
                {
                    world.restoreState(backup);

                    THEN("The initial value is accessed.")
                    {
                        Phase phase;
                        CHECK(h1.get(phase)->get<ComponentA>().d == valA);
                    }

                    WHEN("The modified state is restored.")
                    {
                        world.restoreState(backupModification);

                        THEN("The modified value is accessed.")
                        {
                            Phase phase;
                            CHECK(h1.get(phase)->get<ComponentA>().d == 2 * valA);
                        }
                    }
                }
            }
        }
    }
}


SCENARIO("State saves the archetypes.")
{
    GIVEN("An entity manager with two entities with component (A).")
    {
        EntityManager world;
        Handle<Entity> h1 = world.addEntity();
        Handle<Entity> h2 = world.addEntity();

        {
            Phase phase;
            h1.get(phase)->add<ComponentA>({});
            h2.get(phase)->add<ComponentA>({});
        }

        GIVEN("A backup of the initial state.")
        {
            State initial = world.saveState();

            WHEN("A component (B) is added to entities, and state is saved.")
            {
                {
                    Phase phase;
                    h1.get(phase)->add<ComponentB>({});
                    h2.get(phase)->add<ComponentB>({});
                }

                State second = world.saveState();

                THEN("There are two archetypes on top of empty.")
                {
                    REQUIRE(Inspector<EntityManager>::countArchetypes(world) == 2 + 1);
                }
                THEN("Archetype (A, B) has two entities.")
                {
                    CHECK(Inspector<EntityManager>
                        ::getArchetypeHandle<ComponentA, ComponentB>(world).get().countEntities()
                        == 2);
                }

                WHEN("A component (C) is added to the last entity.")
                {
                    {
                        Phase phase;
                        h2.get(phase)->add<ComponentC>({});
                    }

                    THEN("There are three archetypes on top of empty.")
                    {
                        REQUIRE(Inspector<EntityManager>::countArchetypes(world) == 3 + 1);
                    }
                    THEN("Archetype (A, B) has 1 entity.")
                    {
                        CHECK(Inspector<EntityManager>
                            ::getArchetypeHandle<ComponentA, ComponentB>(world)
                                .get().countEntities()
                            == 1);
                    }
                    THEN("Archetype (A, B, C) has 1 entity.")
                    {
                        CHECK(Inspector<EntityManager>
                            ::getArchetypeHandle<ComponentA, ComponentB, ComponentC>(world)
                                .get().countEntities()
                            == 1);
                    }

                    WHEN("Initial state is restored.")
                    {
                        world.restoreState(initial);

                        THEN("There is 1 archetype on top of empty.")
                        {
                            REQUIRE(Inspector<EntityManager>::countArchetypes(world) == 1 + 1);
                        }
                    }

                    WHEN("Second state is restored.")
                    {
                        world.restoreState(second);

                        THEN("There are 2 archetypes on top of empty.")
                        {
                            REQUIRE(Inspector<EntityManager>::countArchetypes(world) == 2 + 1);
                        }
                        THEN("Archetype (A, B) has 2 entities.")
                        {
                            CHECK(Inspector<EntityManager>
                                ::getArchetypeHandle<ComponentA, ComponentB>(world)
                                    .get().countEntities()
                                == 2);
                        }
                    }
                }
            }
        }
    }
}


SCENARIO("Queries remain valid accross states.")
{
    GIVEN("An entity manager with two entities with component (A), a state backup, and queries.")
    {
        EntityManager world;
        Handle<Entity> h1 = world.addEntity();
        Handle<Entity> h2 = world.addEntity();

        {
            Phase phase;
            h1.get(phase)->add<ComponentA>({});
            h2.get(phase)->add<ComponentA>({});
        }

        // IMPORTANT: client code should never instantiate Queries outside of the current world state.
        // (i.e. Queries should always be part of a System or Component stored in the world state.)
        Query<ComponentA> qA{world};
        Query<ComponentA, ComponentB> qAB{world};
        Query<ComponentA, ComponentC> qAC{world};
        Query<ComponentA, ComponentB, ComponentC> qABC{world};

        State initial = world.saveState();

        REQUIRE(qA.countMatches() == 2);

        GIVEN("A backup of the state with component (B) added to the first entity.")
        {
            {
                Phase phase;
                h1.get(phase)->add<ComponentB>({});
            }
            State second = world.saveState();

            GIVEN("A backup of the state with component (C) added to the last entity, and (B) removed.")
            {
                {
                    Phase phase;
                    h1.get(phase)->remove<ComponentB>();
                    h2.get(phase)->add<ComponentC>({});
                }
                State third = world.saveState();

                WHEN("Initial state is restored.")
                {
                    world.restoreState(initial);

                    THEN("Query(A) has 2 entities, others have 0")
                    {
                        CHECK(qA.countMatches() == 2);
                        CHECK(qAB.countMatches() == 0);
                        CHECK(qAC.countMatches() == 0);
                        CHECK(qABC.countMatches() == 0);
                    }
                }

                WHEN("Second state is restored.")
                {
                    world.restoreState(second);

                    THEN("Query(A) has 2 entities, (A, B) has 1, others have 0")
                    {
                        CHECK(qA.countMatches() == 2);
                        CHECK(qAB.countMatches() == 1);
                        CHECK(qAC.countMatches() == 0);
                        CHECK(qABC.countMatches() == 0);
                    }
                }

                WHEN("Third state is restored.")
                {
                    world.restoreState(third);

                    THEN("Query(A) has 2 entities, (A, C) has 1, others have 0")
                    {
                        CHECK(qA.countMatches() == 2);
                        CHECK(qAB.countMatches() == 0);
                        CHECK(qAC.countMatches() == 1);
                        CHECK(qABC.countMatches() == 0);
                    }
                }
            }
        }
    }
}


SCENARIO("Several backups can be taken and destroyed.")
{
    GIVEN("An entity manager.")
    {
        EntityManager world;

        GIVEN("Several backups are taken.")
        {
            std::vector<State> backups{
                world.saveState(),
                world.saveState(),
                world.saveState(),
            };

            WHEN("The backups are destroyed.")
            {
                backups.clear();

                THEN("All is well.")
                {}
            }
        }

        GIVEN("A query listens on component (A).")
        {
            using QueryA = Query<ComponentA>;
            QueryA qA{world};
            qA.onAddEntity([](auto){});

            Handle<Entity> hq = world.addEntity();
            {
                Phase phase;
                hq.get(phase)->add<QueryA>(std::move(qA));
            }

            GIVEN("Several backups are taken.")
            {
                std::vector<State> backups{
                    world.saveState(),
                    world.saveState(),
                    world.saveState(),
                };

                WHEN("The backups are destroyed.")
                {
                    backups.clear();

                    THEN("All is well.")
                    {}
                }
            }
        }
    }
}


SCENARIO("A restored state listens to new archetypes.")
{
    GIVEN("An entity manager with a query on component (A).")
    {
        using QueryA = Query<ComponentA>;

        EntityManager world;
        Handle<Entity> hq = world.addEntity();

        {
            Phase phase;
            hq.get(phase)->add<QueryA>({world});
        }

        GIVEN("The query listens to entities added.")
        {
            int addCounter{0};
            {
                Phase phase;
                hq.get(phase)->get<QueryA>().onAddEntity([&addCounter](auto)
                        {
                            ++addCounter;
                        });
            }

            GIVEN("A backup of the current state, a later state modification.")
            {
                auto listeningState = world.saveState();
                Handle<Entity> h1 = world.addEntity();
                {
                    Phase phase;
                    h1.get(phase)->add<ComponentB>({});
                }
                REQUIRE(addCounter == 0);

                WHEN("The backup is restored.")
                {
                    world.restoreState(listeningState);

                    WHEN("An entity with component (A) is added, creating the Archetype (A).")
                    {
                        Handle<Entity> h1 = world.addEntity();
                        {
                            Phase phase;
                            h1.get(phase)->add<ComponentA>({});
                        }
                        THEN("The listener is invoked.")
                        {
                            CHECK(addCounter == 1);
                        }
                    }
                }
            }
        }
    }
}


SCENARIO("Destroying backups does not stop active listenings.")
{
    GIVEN("An entity manager with a query on component (A).")
    {
        using QueryA = Query<ComponentA>;

        EntityManager world;
        Handle<Entity> hq = world.addEntity();

        {
            Phase phase;
            hq.get(phase)->add<QueryA>({world});
        }

        GIVEN("The query listens to entities added.")
        {
            int addCounter{0};
            {
                Phase phase;
                hq.get(phase)->get<QueryA>().onAddEntity([&addCounter](auto)
                        {
                            ++addCounter;
                        });
            }

            GIVEN("A backup of the current state.")
            {
                auto listeningState = std::make_unique<State>(world.saveState());

                GIVEN("The backup state is destructed.")
                {
                    listeningState = nullptr;

                    WHEN("An entity with component (A) is added.")
                    {
                        Handle<Entity> h1 = world.addEntity();
                        {
                            Phase phase;
                            h1.get(phase)->add<ComponentA>({});
                        }
                        THEN("The listener is invoked.")
                        {
                            CHECK(addCounter == 1);
                        }
                    }
                }
            }
        }
    }
}

SCENARIO("Queries can listen and stop listening on different states.")
{
    GIVEN("An entity manager with a query on component (A).")
    {
        using QueryA = Query<ComponentA>;

        EntityManager world;
        Handle<Entity> hq = world.addEntity();

        {
            Phase phase;
            hq.get(phase)->add<QueryA>({world});
        }

        GIVEN("The query listens to entities added.")
        {
            int addCounter{0};
            {
                Phase phase;
                hq.get(phase)->get<QueryA>().onAddEntity([&addCounter](auto)
                        {
                            ++addCounter;
                        });
            }

            // Sanity check
            WHEN("An entity with component (A) is added.")
            {
                Handle<Entity> h1 = world.addEntity();
                {
                    Phase phase;
                    h1.get(phase)->add<ComponentA>({});
                }
                THEN("The listener is invoked.")
                {
                    CHECK(addCounter == 1);
                }
            }

            GIVEN("A backup of the current state.")
            {
                State listeningState = world.saveState();

                WHEN("The query stops listening.")
                {
                    {
                        Phase phase;
                        hq.get(phase)->get<QueryA>() = QueryA{world};
                    }

                    // Sanity check
                    WHEN("An entity with component (A) is added.")
                    {
                        Handle<Entity> h1 = world.addEntity();
                        {
                            Phase phase;
                            h1.get(phase)->add<ComponentA>({});
                        }
                        THEN("The listener is not invoked.")
                        {
                            CHECK(addCounter == 0);
                        }
                    }

                    WHEN("The listening state is restored.")
                    {
                        world.restoreState(listeningState);

                        WHEN("An entity with component (A) is added.")
                        {
                            Handle<Entity> h1 = world.addEntity();
                            {
                                Phase phase;
                                h1.get(phase)->add<ComponentA>({});
                            }
                            THEN("The listener is invoked.")
                            {
                                CHECK(addCounter == 1);
                            }
                        }
                    }
                }
            }
        }
    }
}
