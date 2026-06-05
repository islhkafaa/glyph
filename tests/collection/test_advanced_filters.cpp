#include <gtest/gtest.h>

#include "metadata/filter.hpp"

TEST(AdvancedFilterTest, AndFilterMatches) {
    Metadata doc1 = {{"color", std::string("red")}, {"price", 35.0}};
    Metadata doc2 = {{"color", std::string("blue")}, {"price", 35.0}};
    Metadata doc3 = {{"color", std::string("red")}, {"price", 60.0}};

    Filter filter = AndFilter{
        {EqFilter{"color", std::string("red")}, CompareFilter{"price", CompareOp::Lt, 50.0}}};

    EXPECT_TRUE(matches(doc1, filter));
    EXPECT_FALSE(matches(doc2, filter));
    EXPECT_FALSE(matches(doc3, filter));
}

TEST(AdvancedFilterTest, OrFilterMatches) {
    Metadata doc1 = {{"color", std::string("red")}, {"price", 60.0}};
    Metadata doc2 = {{"color", std::string("blue")}, {"price", 30.0}};
    Metadata doc3 = {{"color", std::string("blue")}, {"price", 60.0}};

    Filter filter = OrFilter{
        {EqFilter{"color", std::string("red")}, CompareFilter{"price", CompareOp::Lt, 50.0}}};

    EXPECT_TRUE(matches(doc1, filter));
    EXPECT_TRUE(matches(doc2, filter));
    EXPECT_FALSE(matches(doc3, filter));
}

TEST(AdvancedFilterTest, NotFilterMatches) {
    Metadata doc1 = {{"color", std::string("blue")}};
    Metadata doc2 = {{"color", std::string("red")}};

    Filter filter = NotFilter{std::make_shared<Filter>(EqFilter{"color", std::string("red")})};

    EXPECT_TRUE(matches(doc1, filter));
    EXPECT_FALSE(matches(doc2, filter));
}

TEST(AdvancedFilterTest, CompareOperators) {
    Metadata doc = {{"val", 50.0}, {"name", std::string("apple")}};

    EXPECT_TRUE(matches(doc, CompareFilter{"val", CompareOp::Lte, 50.0}));
    EXPECT_TRUE(matches(doc, CompareFilter{"val", CompareOp::Gte, 50.0}));
    EXPECT_TRUE(matches(doc, CompareFilter{"val", CompareOp::Lt, 51.0}));
    EXPECT_TRUE(matches(doc, CompareFilter{"val", CompareOp::Gt, 49.0}));
    EXPECT_FALSE(matches(doc, CompareFilter{"val", CompareOp::Lt, 50.0}));
    EXPECT_FALSE(matches(doc, CompareFilter{"val", CompareOp::Gt, 50.0}));

    EXPECT_TRUE(matches(doc, CompareFilter{"name", CompareOp::Lt, std::string("banana")}));
    EXPECT_FALSE(matches(doc, CompareFilter{"name", CompareOp::Gt, std::string("banana")}));
}

TEST(AdvancedFilterTest, InFilterMatches) {
    Metadata doc = {{"tag", std::string("ai")}};

    Filter filter_in = InFilter{"tag", {std::string("ai"), std::string("db")}, false};
    Filter filter_notin = InFilter{"tag", {std::string("web")}, true};
    Filter filter_notin_match = InFilter{"tag", {std::string("ai")}, true};

    EXPECT_TRUE(matches(doc, filter_in));
    EXPECT_TRUE(matches(doc, filter_notin));
    EXPECT_FALSE(matches(doc, filter_notin_match));
}
