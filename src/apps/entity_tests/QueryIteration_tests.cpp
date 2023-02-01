
#include "Components_helpers.h"

#include <entity/Entity.h>
#include <entity/EntityManager.h>
#include <entity/Query.h>

#include <iostream>
#include <sstream>


// Should appear before catch inclusion.
std::ostream & operator<<(std::ostream & aOut, const std::pair<ad::ent::EntityIndex, ad::ent::EntityIndex> & aValue)
{
    return aOut << "{" << aValue.first << ", " << aValue.second << "}";
}


#include "catch.hpp"


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
    GIVEN("An entity manager with three entities.")
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
                        { 10.,  100.},
                        { 10., 1000.},
                        {100., 1000.},
                    };
                    queryA.eachPair([&](ComponentA & aLeft, ComponentA & aRight)
                            {
                                ++pairCounter;
                                std::pair<double, double> inPair{aLeft.d, aRight.d};
                                std::ostringstream oss;
                                oss << "Received pair: " << inPair.first << ", " << inPair.second;
                                INFO(oss.str());
                                CHECK(expectedPairs.erase(inPair) == 1);
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


SCENARIO("Query iteration with handles.")
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
                    queryAB.each([&](Handle<Entity> aEntity, ComponentA & a, ComponentB & b)
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


SCENARIO("Query iteration with subset of components.")
{
    GIVEN("An entity manager with two entities.")
    {
        EntityManager world;
        Handle<Entity> h1 = world.addEntity();
        Handle<Entity> h2 = world.addEntity();

        const EntityIndex h1Id = h1.id();
        const EntityIndex h2Id = h2.id();

        GIVEN("Three components (A, B) with distinct values on each entity.")
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

                WHEN("The query is iterated with a callable taking only B.")
                {
                    std::size_t entityCounter{0};
                    std::vector<std::string> strings;
                    queryAB.each([&](ComponentB & b)
                            {
                                ++entityCounter;
                                strings.push_back(b.str);
                            });

                    THEN("The callback is called as many times as there are entities.")
                    {
                        CHECK(entityCounter == queryAB.countMatches());
                    }

                    THEN("The callback was invoked with on the two entities in order.")
                    {
                        CHECK(strings.at(0) == firstB);
                        CHECK(strings.at(1) == secondB);
                    }
                }
                
                WHEN("The query is iterated with a callable taking components out of order.")
                {
                    std::size_t entityCounter{0};
                    std::vector<std::string> strings;
                    queryAB.each([&](ComponentB & b, ComponentA & a)
                            {
                                ++entityCounter;
                            });

                    THEN("The callback is called as many times as there are entities.")
                    {
                        CHECK(entityCounter == queryAB.countMatches());
                    }
                }

                WHEN("The query is iterated with a callable taking only the handle.")
                {
                    std::set<EntityIndex> visitedIds;
                    std::size_t entityCounter{0};
                    queryAB.each([&](Handle<Entity> aEntity)
                            {
                                visitedIds.insert(aEntity.id());
                                ++entityCounter;
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


SCENARIO("Query pair iteration with handles and subset of components.")
{
    GIVEN("An entity manager with three entities.")
    {
        EntityManager world;
        Handle<Entity> h1 = world.addEntity();
        Handle<Entity> h2 = world.addEntity();
        Handle<Entity> h3 = world.addEntity();

        GIVEN("Three components (A, B) with distinct values on each entity, a component (C) on the third entity.")
        {
            const double firstA = 10.;
            const double secondA = 100.;
            const double thirdA = 1000.;

            const std::string firstB{"first"};
            const std::string secondB{"second"};
            const std::string thirdB{"third"};

            {
                Phase phase;
                h1.get(phase)->add<ComponentA>({firstA})
                    .add<ComponentB>({firstB});
                h2.get(phase)->add<ComponentA>({secondA})
                    .add<ComponentB>({secondB});
                h3.get(phase)->add<ComponentA>({thirdA})
                    .add<ComponentB>({thirdB})
                    .add<ComponentC>({});
            }

            GIVEN("A query on components (A, B).")
            {
                Query<ComponentA, ComponentB> queryAB{world};
                REQUIRE(queryAB.countMatches() == 3);

                WHEN("The query is iterated on each pair, A component only.")
                {
                    std::size_t pairCounter{0};
                    std::set<std::pair<double, double>> expectedPairs{
                        {10., 100},
                        {10., 1000},
                        {100., 1000},
                    };

                    queryAB.eachPair([&](Handle<Entity> aLeftHandle, std::tuple<ComponentA &> aLeft,
                                         Handle<Entity> aRightHandle, std::tuple<ComponentA &> aRight)
                            {
                                auto & [leftA] = aLeft;  
                                auto & [rightA] = aRight;  
                                ++pairCounter;
                                std::pair<double, double> inPair{leftA.d, rightA.d};
                                std::ostringstream oss;
                                oss << "Received pair: " << inPair.first << ", " << inPair.second;
                                INFO(oss.str());
                                CHECK(expectedPairs.erase(inPair) == 1);
                            });

                    THEN("The callback is called on each pair.")
                    {
                        CHECK(pairCounter == 3);
                        CHECK(expectedPairs.empty());
                    }
                }
                
                WHEN("The query is iterated on each pair, A component for left, B component for right.")
                {
                    std::size_t pairCounter{0};
                    std::set<std::pair<EntityIndex, EntityIndex>> visitedPairs;
                    const std::set<std::pair<EntityIndex, EntityIndex>> expectedPairs{
                        {h1.id(), h2.id()},
                        {h1.id(), h3.id()},
                        {h2.id(), h3.id()},
                    };

                    Phase dummy;
                    queryAB.eachPair([&](Handle<Entity> aLeftHandle, std::tuple<ComponentA &> a,
                                         Handle<Entity> aRightHandle, std::tuple<ComponentB &> b)
                            {
                                ++pairCounter;
                                visitedPairs.insert({aLeftHandle.id(), aRightHandle.id()});

                                // Checks that the handle corresponds to the provided components.
                                CHECK(aLeftHandle.get(dummy)->get<ComponentA>().d == std::get<0>(a).d);
                                CHECK(aRightHandle.get(dummy)->get<ComponentB>().str == std::get<0>(b).str);
                            });

                    THEN("The callback is called on each pair.")
                    {
                        CHECK(pairCounter == 3);
                    }

                    THEN("The expected pairs were visited")
                    {
                        CHECK(visitedPairs == expectedPairs);
                    }
                }
            }
        }
    }
}
