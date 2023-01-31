
#include "catch.hpp"

#include <entity/EntityManager.h>
#include <entity/Wrap.h>


using namespace ad;
using namespace ad::ent;


struct MyType
{
    static constexpr bool is_wrappable = true;

    int i;
    float f;
    std::string s;
};


SCENARIO("A user type can be wrapped.")
{
    GIVEN("An EntityManager.")
    {
        EntityManager world;
        REQUIRE(world.countLiveEntities() == 0);

        WHEN("A Wrap of MyType is created for this manager.")
        {
            {
                Wrap<MyType> wrapped{world};

                THEN("There is one entity in the manager.")
                {
                    CHECK(world.countLiveEntities() == 1);
                }

                WHEN("Its values are written.")
                {
                    wrapped->i = 1;
                    wrapped->f = 2.f;
                    wrapped->s = "My string message.";

                    THEN("The written values were stored.")
                    {
                        CHECK(wrapped->i == 1);
                        CHECK(wrapped->f == 2.f);
                        CHECK(wrapped->s == "My string message.");
                    }
                }
            }

            WHEN("The Wrap scope is exited.")
            {
                THEN("The entity is removed from the manager.")
                {
                    THEN("There is one entity in the manager.")
                    {
                        CHECK(world.countLiveEntities() == 0);
                    }
                }
            }
        }  
    }
}