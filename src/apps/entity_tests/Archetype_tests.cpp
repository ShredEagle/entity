#include "catch.hpp"

#include "Components_helpers.h" 

#include <entity/Entity.h>
#include <entity/Query.h>
#include <entity/EntityManager.h>

using namespace ad;
using namespace ad::ent;


SCENARIO("Stunfest23 bug#2: Entity's position into an archetype, stored in the EntityRecord,"
         " should match the position of the corresponding handle in said Archetype.")
{
    GIVEN("An entity manager with two entities")
    {
        EntityManager world;
        Handle<Entity> h1 = world.addEntity();
        Handle<Entity> h2 = world.addEntity();

        GIVEN("An archetype with ComponentA containing h1 and h2")
        {
            Query<ComponentA> q{world};
            {
                Phase init;
                h1.get(init)
                    ->add<ComponentA>({5.8})
                ;
                h2.get(init)
                    ->add<ComponentA>({5.9})
                ;
            }

            WHEN("Adding ComponentA should not affect the [ComponentA, ComponentB] Archetype")
            {
                {
                    Phase modify;
                    h1.get(modify)->add<ComponentA>({6.9});
                }

                THEN("The archetype is valid")
                {
                    CHECK(q.verifyArchetypes());
                }
            }
        }
    }
}


SCENARIO("Stunfest23 bug#2: removal variant.")
{
    GIVEN("An entity manager with two entities")
    {
        EntityManager world;
        Handle<Entity> h1 = world.addEntity();
        Handle<Entity> h2 = world.addEntity();

        GIVEN("An archetype with ComponentA and ComponentB containing h1 and h2")
        {
            Query<ComponentA> q{world};
            const std::string str{"rrpp58"};
            {
                Phase init;
                h1.get(init)
                    ->add<ComponentA>({5.8})
                ;
                h2.get(init)
                    ->add<ComponentA>({5.9})
                ;
            }

            WHEN("Adding ComponentA should not affect the [ComponentA, ComponentB] Archetype")
            {
                {
                    // Remove a component that is not present, this triggered the bug too
                    Phase modify;
                    h1.get(modify)->remove<ComponentB>();
                }

                THEN("The archetype is valid")
                {
                    CHECK(q.verifyArchetypes());
                }
            }
        }
    }
}


// This bug is notably triggered when :
// an entity is not the last in its archetype, and it is added a component it already has, 
// then a component it does not already have.
SCENARIO("Stunfest23 bug#1.")
{
    GIVEN("An entity manager with an entity containing components A & B.")
    {
        EntityManager world;
        Handle<Entity> h1 = world.addEntity();
        Handle<Entity> h2 = world.addEntity();
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

        WHEN("Component A is added again.")
        {
            {
                Phase mutation;
                h1.get(mutation)
                    ->add<ComponentA>({66.0})
                    .add<ComponentC>({ {1, 2, 3} })
                ;
            }

            THEN("The archetype is valid")
            {
                Query<ComponentA, ComponentB> q{world};
                CHECK(q.verifyArchetypes());
            }
        }
    }
}