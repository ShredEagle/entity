#include "catch.hpp"

#include "Components_helpers.h"

#include <entity/Entity.h>
#include <entity/EntityManager.h>
#include <entity/Query.h>

#include <array>


using namespace ad;
using namespace ad::ent;


SCENARIO("Query population.")
{
    GIVEN("An entity manager with 3 entities.")
    {
        EntityManager world;
        Handle<Entity> h1 = world.addEntity();
        Handle<Entity> h2 = world.addEntity();
        Handle<Entity> h3 = world.addEntity();

        WHEN("A query on component (A) is instantiated.")
        {
            Query<ComponentA> queryA{world};

            THEN("It matches no entities.")
            {
                CHECK(queryA.countMatches() == 0);
            }
        }

        WHEN("A component (A) with distinct values is added on each entity.")
        {
            const double a[] = {
                10.,
                100.,
                0.25,
            };

            {
                Phase phase;
                h1.get(phase)->add<ComponentA>({a[0]});
                h2.get(phase)->add<ComponentA>({a[1]});
                h3.get(phase)->add<ComponentA>({a[2]});
            }

            WHEN("A query on component (A) is instantiated.")
            {
                Query<ComponentA> queryA{world};

                THEN("It matches the 3 entities.")
                {
                    CHECK(queryA.countMatches() == 3);
                }

                THEN("The 3 values are present.")
                {
                    std::set<double> values{std::begin(a), std::end(a)};
                    REQUIRE(values.size() == 3);

                    queryA.each([&](ComponentA & a)
                    {
                        values.erase(a.d);
                    });
                    CHECK(values.empty());
                }
            }

            WHEN("Other components are added to some entities.")
            {
                {
                    Phase phase;
                    h2.get(phase)->add<ComponentB>({"b2"});
                    h3.get(phase)->add<ComponentB>({"b3"});
                    h3.get(phase)->add<ComponentEmpty>({});
                }

                WHEN("A query on component (A) is instantiated.")
                {
                    Query<ComponentA> queryA{world};

                    THEN("It matches the 3 entities.")
                    {
                        CHECK(queryA.countMatches() == 3);
                    }

                    THEN("The 3 values are present.")
                    {
                        std::set<double> values{std::begin(a), std::end(a)};
                        REQUIRE(values.size() == 3);

                        queryA.each([&](ComponentA & a)
                        {
                            values.erase(a.d);
                        });
                        CHECK(values.empty());
                    }
                }

                WHEN("A query on component (B) is instantiated.")
                {
                    Query<ComponentB> queryB{world};

                    THEN("It matches the 2 entities.")
                    {
                        CHECK(queryB.countMatches() == 2);
                    }
                }

                WHEN("A query on component (Empty) is instantiated.")
                {
                    Query<ComponentEmpty> queryEmpty{world};

                    THEN("It matches 1 entity.")
                    {
                        CHECK(queryEmpty.countMatches() == 1);
                    }
                }

                WHEN("Component (A) is removed from the second entity")
                {
                    {
                        Phase phase;
                        h2.get(phase)->remove<ComponentA>();
                    }

                    WHEN("A query on component (A) is instantiated.")
                    {
                        Query<ComponentA> queryA{world};

                        THEN("It matches the 2 entities.")
                        {
                            CHECK(queryA.countMatches() == 2);
                        }

                        THEN("The 2 expected values are present.")
                        {
                            std::set<double> values{a[0], a[2]};
                            REQUIRE(values.size() == 2);

                            queryA.each([&](ComponentA & a)
                            {
                                values.erase(a.d);
                            });
                            CHECK(values.empty());
                        }
                    }

                    WHEN("A query on component (B) is instantiated.")
                    {
                        Query<ComponentB> queryB{world};

                        THEN("It matches the 2 entities.")
                        {
                            CHECK(queryB.countMatches() == 2);
                        }
                    }

                    WHEN("A query on component (Empty) is instantiated.")
                    {
                        Query<ComponentEmpty> queryEmpty{world};

                        THEN("It matches 1 entity.")
                        {
                            CHECK(queryEmpty.countMatches() == 1);
                        }
                    }
                }
            }
        }
    }
}


SCENARIO("Queries are kept up to date.")
{
    GIVEN("An entity manager with an entity.")
    {
        EntityManager world;
        Handle<Entity> h1 = world.addEntity();

        GIVEN("A query on component (A).")
        {
            using A = ComponentA;
            Query<A> qA{world};

            THEN("Initially, the query does not match any entity.")
            {
                REQUIRE(qA.countMatches() == 0);
            }

            WHEN("Component (A) is added on the entity.")
            {
                {
                    Phase phase;
                    h1.get(phase)->add<ComponentA>({5.8});
                }

                THEN("The query matches the entity.")
                {
                    REQUIRE(qA.countMatches() == 1);
                }

                WHEN("Component (A) is removed from the entity.")
                {
                    {
                        Phase phase;
                        h1.get(phase)->remove<ComponentA>();
                    }

                    THEN("The query does not match the entity.")
                    {
                        REQUIRE(qA.countMatches() == 0);
                    }
                }
            }
        }
    }
}


SCENARIO("Query on multiple components.")
{
    GIVEN("An entity manager with 3 entities.")
    {
        EntityManager world;
        std::array<Handle<Entity>, 3> entities{
            world.addEntity(),
            world.addEntity(),
            world.addEntity(),
        };

        GIVEN("Queries on (A), (A, B), (A, C), (A, B, C).")
        {
            using A = ComponentA;
            using B = ComponentB;
            using C = ComponentC;

            Query<A>        qA{world};
            Query<A, B>     qAB{world};
            Query<A, C>     qAC{world};
            Query<A, B, C>  qABC{world};

            THEN("Initially the queries match no entites.")
            {
                REQUIRE(qA.countMatches() == 0);
                REQUIRE(qAB.countMatches() == 0);
                REQUIRE(qAC.countMatches() == 0);
                REQUIRE(qABC.countMatches() == 0);
            }

            WHEN("A component (A) is added on each entity.")
            {
                const double a[] = {
                    10.,
                    100.,
                    0.25,
                };

                {
                    Phase phase;
                    entities[0].get(phase)->add<ComponentA>({a[0]});
                    entities[1].get(phase)->add<ComponentA>({a[1]});
                    entities[2].get(phase)->add<ComponentA>({a[2]});
                }

                THEN("query (A) contains 3 entities.")
                {
                    REQUIRE(qA.countMatches() == 3);
                    REQUIRE(qAB.countMatches() == 0);
                    REQUIRE(qAC.countMatches() == 0);
                    REQUIRE(qABC.countMatches() == 0);
                }
            }
        }
    }
}


SCENARIO("Query iteration.")
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
