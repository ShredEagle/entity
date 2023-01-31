
#include "catch.hpp"

#include <entity/EntityManager.h>
#include <entity/Wrap.h>


using namespace ad;
using namespace ad::ent;


struct MyType
{
    int i;
    float f;
    std::string s;
};

class TypeWithManager
{
public:
    TypeWithManager(EntityManager & aWorld, int aInt) :
        worldAddress{&aWorld},
        i{aInt}
    {}

    int & getInt()
    {
        return i;
    }

    EntityManager * getWorldAddress()
    {
        return worldAddress;
    }

private:
    EntityManager * worldAddress;
    int i;
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


SCENARIO("Wrap construction.")
{
    GIVEN("An EntityManager.")
    {
        EntityManager world;

        WHEN("A Wrap of MyType is instantiated for this manager, with arguments for list initialization.")
        {
            const std::string str{"Message string"};
            Wrap<MyType> myType{world, 1, 12.f, str};

            THEN("The list initialization values were stored.")
            {
                CHECK(myType->i == 1);
                CHECK(myType->f == 12.f);
                CHECK(myType->s == str);
            }
        }
        
        WHEN("A Wrap of TypeWithManager is instantiated for this manager, with arguments for a constructor.")
        {
            // Important: we did not have to repeat world argument, even though the ctor takes an EntityManager first.
            Wrap<TypeWithManager> typeWithManager{world, 100};

            THEN("The constructor was called.")
            {
                CHECK(typeWithManager->getInt() == 100);
                CHECK(typeWithManager->getWorldAddress() == &world);
            }
        }
    }
}