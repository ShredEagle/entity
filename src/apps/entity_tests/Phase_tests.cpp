#include "catch.hpp"

#include "Components_helpers.h"

#include <entity/Entity.h>
#include <entity/EntityManager.h>


using namespace ad;
using namespace ad::ent;


SCENARIO("Phases defer some modifications.")
{
    GIVEN("An entity manager.")
    {
        EntityManager world;

        GIVEN("An entity added to the manager.")
        {
            REQUIRE(world.countLiveEntities() == 0);

            Handle<Entity> h1 = world.addEntity();
            CHECK(world.countLiveEntities() == 1);

            WHEN("A component (A) is added.")
            {
                const double refValue = 10E6;

                {
                    Phase phase;
                    Entity e1 = *h1.get(phase);
                    e1.add(ComponentA{refValue});

                    THEN("The component is not visible while the phase is not over.")
                    {
                        CHECK_FALSE(e1.has<ComponentA>());
                    }
                }

                THEN("The component becomes visible when the phase is over.")
                {
                    Phase phase;
                    Entity e1 = *h1.get(phase);
                    CHECK(e1.has<ComponentA>());
                }
            }
        }
    }
}

SCENARIO("Phase should be able to outlive handles")
{
    GIVEN("An entity manager.")
    {
        EntityManager world;

        GIVEN("An entity added to the manager")
        {
            Handle<Entity> h1 = world.addEntity();
            WHEN("A phase is created")
            {
                std::unique_ptr<Phase> phase = std::make_unique<Phase>();
                    
                WHEN("The phase outlive the handle onto which a component is added")
                {
                    {
                        Handle<Entity> h1bis = h1;
                        Entity e1 = *h1bis.get(*phase);

                        constexpr double refValue = 10E6;
                        e1.add(ComponentA{refValue});
                    }

                    phase = nullptr;

                    THEN("The component becomes visible ")
                    {
                        Phase phasebis;
                        Entity e1 = *h1.get(phasebis);
                        CHECK(e1.has<ComponentA>());
                    }
                }
                WHEN("The phase outlive the handle onto which a component is removed")
                {
                    {
                        Handle<Entity> h1bis = h1;
                        {
                            Phase phaseter;
                            Entity e1 = *h1bis.get(phaseter);

                            constexpr double refValue = 10E6;
                            e1.add(ComponentA{refValue});
                        }
                        
                        Entity e2 = *h1bis.get(*phase);
                        e2.remove<ComponentA>();
                    }

                    phase = nullptr;

                    THEN("The component becomes invisible ")
                    {
                        Phase phasebis;
                        Entity e1 = *h1.get(phasebis);
                        CHECK(!e1.has<ComponentA>());
                    }
                }
            }
        }
    }
}
