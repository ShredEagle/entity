#include "catch.hpp"

#include "Components_helpers.h" 

#include <entity/Entity.h>
#include <entity/Query.h>
#include <entity/EntityManager.h>

using namespace ad;
using namespace ad::ent;


SCENARIO("Archetype should be stable")
{
    GIVEN("An entity manager with two entities")
    {
        EntityManager world;
        Handle<Entity> h1 = world.addEntity();
        Handle<Entity> h2 = world.addEntity();

        GIVEN("An archetype with ComponentA and ComponentB containing h1 and h2")
        {
            Query<ComponentA, ComponentB> q{world};
            const std::string str{"rrpp58"};
            {
                Phase init;
                h1.get(init)
                    ->add<ComponentA>({5.8})
                    .add<ComponentB>({str})
                ;
                h2.get(init)
                    ->add<ComponentA>({5.9})
                    .add<ComponentB>({str})
                ;
            }

            WHEN("Adding ComponentA should not affect the [ComponentA, ComponentB] Archetype")
            {
                {
                    Phase modify;
                    h2.get(modify)->add<ComponentA>({6.8});
                    h1.get(modify)->add<ComponentA>({6.9});
                }

                THEN("The archetype is valid")
                {
                    q.verifyArchetypes();
                }
            }
        }
    }
}

