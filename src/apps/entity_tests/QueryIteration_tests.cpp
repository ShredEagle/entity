#include "catch.hpp"

#include "Components_helpers.h"

#include <entity/Entity.h>
#include <entity/EntityManager.h>
#include <entity/Query.h>


using namespace ad;
using namespace ad::ent;


// Iterating all components of the Query, in the same order.
SCENARIO("Query simple iteration.")
{
    GIVEN("An entity manager with two entities.")
    {
        EntityManager world;
        Handle<Entity> h1 = world.addEntity();
        Handle<Entity> h2 = world.addEntity();

        GIVEN("A component (A) with distinct values on each entity.")
        {
            const double firstA = 10.;
            const double secondA = 100.;

            {
                Phase phase;
                h1.get(phase)->add<ComponentA>({firstA});
                h2.get(phase)->add<ComponentA>({secondA});
            }

            GIVEN("A query on component (A).")
            {
                Query<ComponentA> queryA{world};
                REQUIRE(queryA.countMatches() == 2);

                WHEN("The query is iterated.")
                {
                    std::size_t entityCounter{0};
                    queryA.each([&](ComponentA &)
                            {
                                ++entityCounter;
                            });

                    THEN("The callback is called as many times as there are entities.")
                    {
                        CHECK(entityCounter == queryA.countMatches());
                    }
                }

                WHEN("The query is iterated to change the value of components.")
                {
                    queryA.each([](ComponentA & a)
                            {
                               a.d += 1;
                            });

                    THEN("The entities components contain the new values.")
                    {
                        Phase phase;
                        CHECK(h1.get(phase)->get<ComponentA>().d == firstA + 1);
                        CHECK(h2.get(phase)->get<ComponentA>().d == secondA + 1);
                    }
                }
            }

            WHEN("A component (B) is added on the second entity.")
            {
                const std::string secondB{"I'm B."};
                {
                    Phase phase;
                    h2.get(phase)->add<ComponentB>({secondB});
                }

                WHEN("A query on component (A) is instantiated.")
                {
                    Query<ComponentA> queryA{world};
                    REQUIRE(queryA.countMatches() == 2);

                    WHEN("The query is iterated.")
                    {
                        std::size_t entityCounter{0};
                        queryA.each([&](ComponentA &)
                                {
                                    ++entityCounter;
                                });

                        THEN("The callback is called as many times as there are entities.")
                        {
                            CHECK(entityCounter == queryA.countMatches());
                        }
                    }

                    WHEN("The query is iterated to change the value of components.")
                    {
                        queryA.each([](ComponentA & a)
                                {
                                   a.d += 1;
                                });

                        THEN("The entities components contain the new values.")
                        {
                            Phase phase;
                            CHECK(h1.get(phase)->get<ComponentA>().d == firstA + 1);
                            CHECK(h2.get(phase)->get<ComponentA>().d == secondA + 1);
                        }
                    }
                }
            }
        }
    }
}


SCENARIO("Pair simple iteration.")
{
    GIVEN("An entity manager with two entities.")
    {
        EntityManager world;
        Handle<Entity> h1 = world.addEntity();
        Handle<Entity> h2 = world.addEntity();
        Handle<Entity> h3 = world.addEntity();

        GIVEN("A component (A) with distinct values on each entity, a component (B) on the third entity.")
        {
            const double firstA = 10.;
            const double secondA = 100.;
            const double thirdA = 1000.;

            {
                Phase phase;
                h1.get(phase)->add<ComponentA>({firstA});
                h2.get(phase)->add<ComponentA>({secondA});
                h3.get(phase)->add<ComponentA>({thirdA})
                    .add<ComponentB>({});
            }

            GIVEN("A query on component (A).")
            {
                Query<ComponentA> queryA{world};
                REQUIRE(queryA.countMatches() == 3);

                WHEN("The query is iterated on each pair.")
                {
                    std::size_t pairCounter{0};
                    std::set<std::pair<double, double>> expectedPairs{
                        {10., 100},
                        {10., 1000},
                        {100., 1000},
                    };
                    queryA.eachPair([&](ComponentA & aLeft, ComponentA & aRight)
                            {
                                ++pairCounter;
                                expectedPairs.erase({aLeft.d, aRight.d});
                            });

                    THEN("The callback is called on each pair.")
                    {
                        CHECK(pairCounter == 3);
                        CHECK(expectedPairs.empty());
                    }
                }
            }
        }
    }
}


SCENARIO("Query adavanced iteration.")
{
    GIVEN("An entity manager with two entities.")
    {
        EntityManager world;
        Handle<Entity> h1 = world.addEntity();
        Handle<Entity> h2 = world.addEntity();

        const EntityIndex h1Id = h1.id();
        const EntityIndex h2Id = h2.id();

        GIVEN("Two components (A, B) with distinct values on each entity.")
        {
            const double firstA = 10.;
            const double secondA = 100.;

            const std::string firstB{"first"};
            const std::string secondB{"second"};

            {
                Phase phase;
                h1.get(phase)
                    ->add<ComponentA>({firstA})
                    .add<ComponentB>({firstB});
                h2.get(phase)
                    ->add<ComponentA>({secondA})
                    .add<ComponentB>({secondB});
            }

            GIVEN("A query both components (A, B).")
            {
                Query<ComponentA, ComponentB> queryAB{world};
                REQUIRE(queryAB.countMatches() == 2);

                WHEN("The query is iterated with a callable taking the entity handle.")
                {
                    std::set<EntityIndex> visitedIds;
                    std::size_t entityCounter{0};
                    Phase dummy{};
                    queryAB.eachHandle([&](Handle<Entity> aEntity, ComponentA & a, ComponentB & b)
                            {
                                visitedIds.insert(aEntity.id());
                                ++entityCounter;

                                // Checks that the handle corresponds to the provided components.
                                CHECK(aEntity.get(dummy)->get<ComponentA>().d == a.d);
                                CHECK(aEntity.get(dummy)->get<ComponentB>().str == b.str);
                            });

                    THEN("The callback is called as many times as there are entities.")
                    {
                        CHECK(entityCounter == queryAB.countMatches());
                    }

                    THEN("The callback was invoked once for each handle, corresponding to each entity.")
                    {
                        CHECK(visitedIds.contains(h1Id));
                        CHECK(visitedIds.contains(h2Id));
                    }
                }
            }
        }
    }
}
