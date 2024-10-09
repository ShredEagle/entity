#include "catch.hpp"

#include "Components_helpers.h"

#include <entity/Entity.h>
#include <entity/Blueprint.h>
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


SCENARIO("Queries can be instantiated with different components ordering.")
{
    GIVEN("An entity manager, with an entity with components (A, B, C).")
    {
        using A = ComponentA;
        using B = ComponentB;
        using C = ComponentC;

        EntityManager world;
        const double valueA = 5.8;

        Handle<Entity> h1 = world.addEntity();
        {
            Phase phase;
            h1.get(phase)->add<A>({valueA})
                .add<B>({})
                .add<C>({});
        }

        WHEN("A query on (A, B, C) is instantiated.")
        {
            Query<A, B, C>  qABC{world};
            REQUIRE(qABC.countMatches() == 1);

            WHEN("A query on (C, B, A) is instantiated.")
            {
                Query<C, B, A>  qCBA{world};

                THEN("Both queries match the entity.")
                {
                    CHECK(qABC.countMatches() == 1);
                    CHECK(qCBA.countMatches() == 1);
                }

                WHEN("The component A is written through the first query.")
                {
                    qABC.each([&](A & a, B &, C &)
                    {
                        a.d *= 2;
                    });

                    THEN("The change in value is visible to the second query.")
                    {
                        qCBA.each([&](C &, B &, A & a)
                        {
                            CHECK(a.d == 2 * valueA);
                        });
                    }
                }
            }
        }
    }
}


SCENARIO("Removing components takes entities out of corresponding queries.")
{
    GIVEN("An entity manager, with an entity with components (A, B).")
    {
        EntityManager world;
        Handle<Entity> h1 = world.addEntity();
        {
            Phase phase;
            h1.get(phase)->add<ComponentA>({5.8})
                .add<ComponentB>({});
        }
        REQUIRE(Query<ComponentA>{world}.countMatches() == 1);

        WHEN("Component (A) is removed from the entity.")
        {
            {
                Phase phase;
                h1.get(phase)->remove<ComponentA>();
            }

            THEN("The entity does not match queries on (A).")
            {
                Query<ComponentA> qA{world};
                CHECK(qA.countMatches() == 0);
            }
        }

        WHEN("Component (C) is added while component (A) is removed from the entity.")
        {
            {
                Phase phase;
                h1.get(phase)->add<ComponentC>({})
                    .remove<ComponentA>();
            }

            THEN("The entity does not match queries on (A).")
            {
                Query<ComponentA> qA{world};
                CHECK(qA.countMatches() == 0);
            }
        }

        WHEN("Component (C) is added, then component (A) is removed from the entity.")
        {
            {
                Phase phase;
                h1.get(phase)->add<ComponentC>({});
            }

            {
                Phase phase;
                h1.get(phase)->remove<ComponentA>();
            }

            THEN("The entity does not match queries on (A).")
            {
                Query<ComponentA> qA{world};
                CHECK(qA.countMatches() == 0);
            }
        }
    }
}


SCENARIO("Queries are updated with new matching archetypes.")
{
    GIVEN("An entity manager, with an entity with components (A, B).")
    {
        EntityManager world;
        Handle<Entity> h1 = world.addEntity();
        {
            Phase phase;
            h1.get(phase)->add<ComponentA>({5.8})
                .add<ComponentB>({});
        }

        GIVEN("A query on component (A).")
        {
            using A = ComponentA;
            Query<A> qA{world};
            REQUIRE(qA.countMatches() == 1);

            WHEN("Component (C) is added on the entity (a new matching archetype, by extension).")
            {
                {
                    Phase phase;
                    h1.get(phase)->add<ComponentC>({});
                }

                THEN("The query still matches the entity.")
                {
                    REQUIRE(qA.countMatches() == 1);
                }

                WHEN("Component (A) is removed from the entity.")
                {
                    {
                        Phase phase;
                        h1.get(phase)->remove<ComponentA>();
                    }

                    THEN("The query does not match the entity anymore.")
                    {
                        REQUIRE(qA.countMatches() == 0);
                    }
                }
            }

            WHEN("Component (B) is removed from the entity (a new matching archetype, by restriction).")
            {
                {
                    Phase phase;
                    h1.get(phase)->remove<ComponentB>();
                }

                THEN("The query still matches the entity.")
                {
                    REQUIRE(qA.countMatches() == 1);
                }

                WHEN("Component (A) is removed from the entity.")
                {
                    {
                        Phase phase;
                        h1.get(phase)->remove<ComponentA>();
                    }

                    THEN("The query does not match the entity anymore.")
                    {
                        REQUIRE(qA.countMatches() == 0);
                    }
                }
            }
        }
    }
}

SCENARIO("Queries on multiple components.")
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

                WHEN("A component (B) is added to the two first entities.")
                {
                    {
                        Phase phase;
                        entities[0].get(phase)->add<ComponentB>({""});
                        entities[1].get(phase)->add<ComponentB>({""});
                    }

                    THEN("query (A) contains 3 entities, query (AB) contains 2.")
                    {
                        REQUIRE(qA.countMatches() == 3);
                        REQUIRE(qAB.countMatches() == 2);
                        REQUIRE(qAC.countMatches() == 0);
                        REQUIRE(qABC.countMatches() == 0);
                    }

                    WHEN("A component (C) is added to the two last entities,"
                         " while (A) is removed from the middle.")
                    {
                        {
                            Phase phase;
                            entities[1].get(phase)
                                ->add<ComponentC>({})
                                .remove<ComponentA>();
                            entities[2].get(phase)->add<ComponentC>({});
                        }

                        THEN("query (A) contains 2 entities, query (AB) contains 1,"
                             "query (AC) contains 1.")
                        {
                            REQUIRE(qA.countMatches() == 2);
                            REQUIRE(qAB.countMatches() == 1);
                            REQUIRE(qAC.countMatches() == 1);
                            REQUIRE(qABC.countMatches() == 0);
                        }

                        WHEN("A component (A) is put back on the second entity.")
                        {
                            {
                                Phase phase;
                                entities[1].get(phase)->add<ComponentA>({a[1]});
                            }

                            THEN("query (A) contains 3 entities, query (AB) contains 2,"
                                 "query (AC) contains 2, query (ABC) contains 1.")
                            {
                                REQUIRE(qA.countMatches() == 3);
                                REQUIRE(qAB.countMatches() == 2);
                                REQUIRE(qAC.countMatches() == 2);
                                REQUIRE(qABC.countMatches() == 1);
                            }
                        }
                    }
                }
            }

            WHEN("A component (C) is added on each entity.")
            {
                {
                    Phase phase;
                    entities[0].get(phase)->add<ComponentC>({});
                    entities[1].get(phase)->add<ComponentC>({});
                    entities[2].get(phase)->add<ComponentC>({});
                }

                THEN("Queries match no entities.")
                {
                    REQUIRE(qA.countMatches() == 0);
                    REQUIRE(qAB.countMatches() == 0);
                    REQUIRE(qAC.countMatches() == 0);
                    REQUIRE(qABC.countMatches() == 0);
                }
            }
        }
    }
}

SCENARIO("Queries on archetype where some entity are blueprints.")
{
    GIVEN("An entity manager with 2 entities.")
    {
        EntityManager world;
        std::array<Handle<Entity>, 2> entities{
            world.addEntity(),
            world.addEntity(),
        };

        GIVEN("Queries on (A)")
        {
            using A = ComponentA;

            Query<A>        qA{world};

            THEN("Initially the queries match no entites.")
            {
                REQUIRE(qA.countMatches() == 0);
            }

            WHEN("A component (A) is added on each entity.")
            {
                const double a[] = {
                    10.,
                    100.,
                };

                {
                    Phase phase;
                    entities[0].get(phase)->add<ComponentA>({a[0]});
                    entities[1].get(phase)->add<ComponentA>({a[1]});
                }

                THEN("query (A) contains 2 entities.")
                {
                    REQUIRE(qA.countMatches() == 2);
                }

                WHEN("A blueprint component is added to an entity")
                {
                    {
                        Phase phase;
                        entities[1].get(phase)->add<Blueprint>({});
                    }

                    THEN("query (A) contains 1")
                    {
                        REQUIRE(qA.countMatches() == 1);
                    }
                }
            }
        }
    }
}
