#include "catch.hpp"

#include "Components_helpers.h" 

#include <entity/Entity.h>
#include <entity/EntityManager.h>


using namespace ad;
using namespace ad::ent;


SCENARIO("Handles provide a phase-less view access to Entities.")
{
    GIVEN("An entity manager with an entity.")
    {
        EntityManager world;
        Handle<Entity> h1 = world.addEntity();

        GIVEN("Components A and B are added.")
        {
            const std::string str{"rrpp58"};
            {
                Phase init;
                h1.get(init)
                    ->add<ComponentA>({5.8})
                    .add<ComponentB>({str})
                ;
            }

            WHEN("The handle to the entity is used to get a view to the entity.")
            {
                std::optional<Entity_view> o1 = h1.get();

                THEN("The view is to a valid Entity.")
                {
                    CHECK(o1.has_value());
                }

                Entity_view v1 = *o1;

                THEN("The components values can be read.")
                {
                    CHECK(v1.get<ComponentA>().d == 5.8);
                    CHECK(v1.get<ComponentB>().str == str);
                }

                THEN("The components values can be modified.")
                {
                    v1.get<ComponentA>().d += 1;
                    v1.get<ComponentB>().str.erase();

                    CHECK(v1.get<ComponentA>().d == 6.8);
                    CHECK(v1.get<ComponentB>().str.empty());
                }

                THEN("The components presence can be tested.")
                {
                    CHECK(v1.has<ComponentA>());
                    CHECK(v1.has<ComponentB>());
                    CHECK_FALSE(v1.has<ComponentC>());
                }
            }
        }
    }
}


SCENARIO("Handles allow to test the validity of the underlying Entity.")
{
    GIVEN("An entity manager with an entity.")
    {
        EntityManager world;
        Handle<Entity> h1 = world.addEntity();

        WHEN("The handle to the entity is used to get the entity in a scoped phase.")
        {
            {
                Phase scoped;
                std::optional<Entity> o1 = h1.get(scoped);

                THEN("The returned Entity is valid.")
                {
                    CHECK(o1.has_value());
                }

                o1->erase();
            }

            WHEN("The entity was erased and the phase scope exited.")
            {
                WHEN("The handle to the entity is used to get a view to the entity.")
                {
                    std::optional<Entity_view> o1 = h1.get();
                    THEN("The view is **not** to a valid Entity.")
                    {
                        CHECK_FALSE(o1.has_value());
                    }
                }

                WHEN("The handle to the entity is used to get a the entity in a scoped phase.")
                {
                    {
                        Phase scoped;
                        std::optional<Entity> o1 = h1.get(scoped);

                        THEN("The returned Entity is **not** valid.")
                        {
                            CHECK_FALSE(o1.has_value());
                        }
                    }
                }
            }
        }
    }
}