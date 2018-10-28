#include <catch2/catch.hpp>

#include "matter/component/sparse_vector.hpp"

struct random_component
{
    int i;

    random_component(int i) : i{i}
    {}
};

TEST_CASE("sparse_vector")
{
    matter::sparse_vector<random_component> vec;

    SECTION("check invalid")
    {
        CHECK(!vec.has_value(0));
        CHECK(!vec.has_value(1));
    }

    SECTION("insert")
    {
        vec.push_back(0, random_component(0));

        REQUIRE(vec.has_value(0));
        CHECK(vec[0].i == 0);

        SECTION("remove iterator")
        {
            auto it = std::begin(vec);
            REQUIRE(it->i == 0);
            REQUIRE(vec.has_value(0));

            vec.erase(it);

            REQUIRE(!vec.has_value(0));
        }

        SECTION("remove")
        {
            vec.erase(0);
            REQUIRE(!vec.has_value(0));
        }

        for (int i = 1; i < 10; ++i)
        {
            vec.emplace_back(i * 10, i * 10);
        }

        SECTION("remove shifing")
        {
            vec.erase(0);
            REQUIRE(!vec.has_value(0));

            for (int i = 1; i < 10; ++i)
            {
                REQUIRE(vec[i * 10].i == i * 10);
            }
        }

        SECTION("swap and pop")
        {
            vec.swap_and_pop(0);
            REQUIRE(!vec.has_value(0));

            for (int i = 1; i < 10; ++i)
            {
                REQUIRE(vec[i * 10].i == i * 10);
            }
        }
    }
}