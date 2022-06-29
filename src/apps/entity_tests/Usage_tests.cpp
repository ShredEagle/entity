#include "catch.hpp"

#include <entity/EntityManager.h>
#include <entity/Query.h>


using namespace ad;
using namespace ad::ent;


namespace ad {
namespace ent {

template <class T_inspected>
class Inspector;

template <>
class Inspector<EntityManager>
{
public:
    static std::size_t countArchetypes(const EntityManager & aEntityManager)
    { return aEntityManager.mArchetypes.size(); }
};


} // namespace ent
} // namespace ad

struct ComponentA
{
    double d;
};


struct ComponentB
{
    std::string str;
};

SCENARIO("Populating a world.")
{
    GIVEN("An entity manager.")
    {
        EntityManager world;
        REQUIRE(Inspector<EntityManager>::countArchetypes(world) == 0);

        THEN("Entities can be added")
        {
            REQUIRE(world.countLiveEntities() == 0);

            Handle<Entity> h1 = world.addEntity();
            CHECK(world.countLiveEntities() == 1);

            world.addEntity();
            CHECK(world.countLiveEntities() == 2);
        }

        GIVEN("An entity is added to the manager.")
        {
            Handle<Entity> h1 = world.addEntity();
            // The default empty archetype as soon as one entity was added
            REQUIRE(Inspector<EntityManager>::countArchetypes(world) == 1);

            WHEN("A component (A) is added.")
            {
                const double refValue = 8.6;

                {
                    Phase phase;
                    Entity e1 = *h1.get(phase);
                    REQUIRE_FALSE(e1.has<ComponentA>());

                    e1.add(ComponentA{refValue});
                }

                GIVEN("The entity associated to the handle.")
                {
                    Phase phase;
                    Entity e1 = *h1.get(phase);

                    THEN("The total number of entities is not changed")
                    {
                        CHECK(world.countLiveEntities() == 1);
                    }

                    THEN("An archetype was created, containing the component.")
                    {
                        CHECK(Inspector<EntityManager>::countArchetypes(world) == 2);
                    }

                    THEN("Component presence can be tested.")
                    {
                        CHECK(e1.has<ComponentA>());
                    }

                    THEN("Component value can be read.")
                    {
                        CHECK(e1.get<ComponentA>().d == refValue);
                    }

                    GIVEN("A query on this component.")
                    {
                        Query<ComponentA> queryA{world};

                        THEN("It matches the expected number of entities.")
                        {
                            CHECK(queryA.countMatches() == 1);
                        }
                    }

                    GIVEN("A query on another component.")
                    {
                        Query<ComponentB> queryB{world};

                        THEN("There are no matches.")
                        {
                            CHECK(queryB.countMatches() == 0);
                        }
                    }

                    GIVEN("A query on both components.")
                    {
                        Query<ComponentA, ComponentB> queryBoth{world};

                        THEN("There are no matches.")
                        {
                            CHECK(queryBoth.countMatches() == 0);
                        }
                    }
                }

                WHEN("A second component (B) is added.")
                {
                    const std::string refString{"Here we are."};

                    {
                        Phase phase;
                        Entity e1 = *h1.get(phase);
                        e1.add<ComponentB>({.str = refString});
                    }

                    GIVEN("The entity associated to the handle.")
                    {
                        Phase phase;
                        Entity e1 = *h1.get(phase);

                        THEN("Both component presence can be tested.")
                        {
                            CHECK(e1.has<ComponentB>());
                            CHECK(e1.has<ComponentA>());
                        }

                        THEN("Both component values can be read.")
                        {
                            CHECK(e1.get<ComponentB>().str == refString);
                            CHECK(e1.get<ComponentA>().d == refValue);
                        }

                        GIVEN("A query on component A.")
                        {
                            Query<ComponentA> queryA{world};

                            THEN("It matches the one entity.")
                            {
                                CHECK(queryA.countMatches() == 1);
                            }
                        }

                        GIVEN("A query on component B.")
                        {
                            Query<ComponentB> queryB{world};

                            THEN("It matches the one entity.")
                            {
                                CHECK(queryB.countMatches() == 1);
                            }
                        }

                        GIVEN("A query on both components.")
                        {
                            Query<ComponentA, ComponentB> queryBoth{world};

                            THEN("It matches the one entity.")
                            {
                                CHECK(queryBoth.countMatches() == 1);
                            }
                        }
                    }
                }
            }
        }
    }
}
