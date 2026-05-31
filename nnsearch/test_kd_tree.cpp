#include <gtest/gtest.h>
#include "kd-tree.h"
#include <cmath>
#include <vector>
#include <algorithm>
#include <random>

TEST(KDTreeTest, SinglePoint) {
    std::vector<Point<2>> points = {{{1.0, 2.0}}};
    KDTree<2> tree(std::move(points));
    auto [closest, cnt] = tree.GetClosest({1.0, 2.0});
    EXPECT_DOUBLE_EQ(closest[0], 1.0);
    EXPECT_DOUBLE_EQ(closest[1], 2.0);
}

TEST(KDTreeTest, TwoPoints) {
    std::vector<Point<2>> points = {{{0.0, 0.0}}, {{10.0, 10.0}}};
    KDTree<2> tree(std::move(points));

    auto [closest1, cnt1] = tree.GetClosest({1.0, 1.0});
    EXPECT_DOUBLE_EQ(closest1[0], 0.0);
    EXPECT_DOUBLE_EQ(closest1[1], 0.0);

    auto [closest2, cnt2] = tree.GetClosest({9.0, 9.0});
    EXPECT_DOUBLE_EQ(closest2[0], 10.0);
    EXPECT_DOUBLE_EQ(closest2[1], 10.0);
}

TEST(KDTreeTest, ExactMatch) {
    std::vector<Point<2>> points = {{{3.0, 4.0}}, {{7.0, 8.0}}, {{1.0, 1.0}}};
    KDTree<2> tree(std::move(points));
    auto [closest, cnt] = tree.GetClosest({7.0, 8.0});
    EXPECT_DOUBLE_EQ(closest[0], 7.0);
    EXPECT_DOUBLE_EQ(closest[1], 8.0);
}

TEST(KDTreeTest, OneDimension) {
    std::vector<Point<1>> points = {{{1.0}}, {{5.0}}, {{10.0}}, {{3.0}}};
    KDTree<1> tree(std::move(points));

    auto [closest1, cnt1] = tree.GetClosest({4.5});
    EXPECT_DOUBLE_EQ(closest1[0], 5.0);

    auto [closest2, cnt2] = tree.GetClosest({1.4});
    EXPECT_DOUBLE_EQ(closest2[0], 1.0);

    auto [closest3, cnt3] = tree.GetClosest({9.0});
    EXPECT_DOUBLE_EQ(closest3[0], 10.0);
}

TEST(KDTreeTest, ThreeDimensions) {
    std::vector<Point<3>> points = {
        {{0.0, 0.0, 0.0}},
        {{1.0, 1.0, 1.0}},
        {{5.0, 5.0, 5.0}},
        {{10.0, 10.0, 10.0}}
    };
    KDTree<3> tree(std::move(points));

    auto [closest, cnt] = tree.GetClosest({4.9, 4.9, 4.9});
    EXPECT_DOUBLE_EQ(closest[0], 5.0);
    EXPECT_DOUBLE_EQ(closest[1], 5.0);
    EXPECT_DOUBLE_EQ(closest[2], 5.0);
}

TEST(KDTreeTest, NegativeCoordinates) {
    std::vector<Point<2>> points = {
        {{-5.0, -5.0}},
        {{-1.0, -1.0}},
        {{2.0, 2.0}}
    };
    KDTree<2> tree(std::move(points));

    auto [closest, cnt] = tree.GetClosest({-0.5, -0.5});
    EXPECT_DOUBLE_EQ(closest[0], -1.0);
    EXPECT_DOUBLE_EQ(closest[1], -1.0);
}

TEST(KDTreeTest, ManyPoints) {
    std::vector<Point<2>> points;
    for (int i = 0; i < 100; ++i) {
        points.push_back({static_cast<double>(i), static_cast<double>(i)});
    }
    KDTree<2> tree(std::move(points));

    auto [closest, cnt] = tree.GetClosest({42.1, 42.1});
    EXPECT_DOUBLE_EQ(closest[0], 42.0);
    EXPECT_DOUBLE_EQ(closest[1], 42.0);
}

TEST(KDTreeTest, DistanceFunction) {
    Point<2> p1 = {{0.0, 0.0}};
    Point<2> p2 = {{3.0, 4.0}};
    EXPECT_DOUBLE_EQ(distance(p1, p2), 5.0);

    Point<3> p3 = {{1.0, 2.0, 3.0}};
    Point<3> p4 = {{1.0, 2.0, 3.0}};
    EXPECT_DOUBLE_EQ(distance(p3, p4), 0.0);
}

TEST(KDTreeTest, ClosestOnAxis) {
    std::vector<Point<2>> points = {
        {{0.0, 100.0}},
        {{1.0, 0.0}},
        {{-1.0, 0.0}}
    };
    KDTree<2> tree(std::move(points));

    auto [closest, cnt] = tree.GetClosest({0.0, 0.0});
    EXPECT_NEAR(distance(closest, Point<2>{0.0, 0.0}), 1.0, 1e-9);
}

TEST(KDTreeTest, BruteForceComparison) {
    std::mt19937 rng(42);
    std::uniform_real_distribution<double> dist(-100.0, 100.0);

    for (int iter = 0; iter < 100; ++iter) {
        std::vector<Point<30>> points(1000);
        for (auto& p : points) {
            for (auto& coord : p) {
                coord = dist(rng);
            }
        }

        Point<30> query;
        for (auto& coord : query) {
            coord = dist(rng);
        }

        std::vector<Point<30>> points_copy = points;
        KDTree<30> tree(std::move(points));
        auto [kd_closest, cnt] = tree.GetClosest(query);

        double best_dist = std::numeric_limits<double>::max();
        for (const auto& p : points_copy) {
            double d = distance(query, p);
            if (d < best_dist) {
                best_dist = d;
            }
        }

        EXPECT_DOUBLE_EQ(distance(query, kd_closest), best_dist);
    }
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
