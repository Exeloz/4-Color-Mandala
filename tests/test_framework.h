#pragma once

#include <cmath>
#include <functional>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

namespace testfw {

struct TestCase {
    std::string name;
    std::function<void()> run;
};

inline std::vector<TestCase>& registry() {
    static std::vector<TestCase> tests;
    return tests;
}

inline int& failureCount() {
    static int value = 0;
    return value;
}

inline int& assertionCount() {
    static int value = 0;
    return value;
}

inline void registerTest(const std::string& name, std::function<void()> test) {
    registry().push_back({name, std::move(test)});
}

inline void fail(const std::string& file, int line, const std::string& message) {
    failureCount()++;
    std::cerr << file << ":" << line << " - " << message << std::endl;
}

inline bool nearlyEqual(float a, float b, float epsilon = 1e-4f) {
    return std::fabs(a - b) <= epsilon;
}

inline int runAllTests() {
    int passed = 0;
    int failedTests = 0;

    for (const auto& test : registry()) {
        int beforeFailures = failureCount();
        try {
            test.run();
        } catch (const std::exception& ex) {
            fail(__FILE__, __LINE__, std::string("Unhandled exception in test '") + test.name + "': " + ex.what());
        } catch (...) {
            fail(__FILE__, __LINE__, std::string("Unhandled non-standard exception in test '") + test.name + "'");
        }

        if (failureCount() == beforeFailures) {
            passed++;
        } else {
            failedTests++;
        }
    }

    std::cout << "Tests: " << registry().size() << ", Passed: " << passed
              << ", Failed: " << failedTests
              << ", Assertions: " << assertionCount() << std::endl;

    return failedTests == 0 ? 0 : 1;
}

struct TestRegistrar {
    TestRegistrar(const std::string& name, std::function<void()> test) {
        registerTest(name, std::move(test));
    }
};

}  // namespace testfw

#define TEST_CASE(name) \
    void name(); \
    static testfw::TestRegistrar registrar_##name(#name, name); \
    void name()

#define EXPECT_TRUE(condition) \
    do { \
        testfw::assertionCount()++; \
        if (!(condition)) { \
            std::ostringstream _oss; \
            _oss << "Expected true: " #condition; \
            testfw::fail(__FILE__, __LINE__, _oss.str()); \
        } \
    } while (0)

#define EXPECT_FALSE(condition) EXPECT_TRUE(!(condition))

#define EXPECT_EQ(actual, expected) \
    do { \
        testfw::assertionCount()++; \
        auto _actual = (actual); \
        auto _expected = (expected); \
        if (!(_actual == _expected)) { \
            std::ostringstream _oss; \
            _oss << "Expected equality, actual=" << _actual << ", expected=" << _expected; \
            testfw::fail(__FILE__, __LINE__, _oss.str()); \
        } \
    } while (0)

#define EXPECT_NE(actual, expected) \
    do { \
        testfw::assertionCount()++; \
        auto _actual = (actual); \
        auto _expected = (expected); \
        if (_actual == _expected) { \
            std::ostringstream _oss; \
            _oss << "Expected inequality, value=" << _actual; \
            testfw::fail(__FILE__, __LINE__, _oss.str()); \
        } \
    } while (0)

#define EXPECT_NEAR(actual, expected, epsilon) \
    do { \
        testfw::assertionCount()++; \
        float _actual = static_cast<float>(actual); \
        float _expected = static_cast<float>(expected); \
        float _eps = static_cast<float>(epsilon); \
        if (!testfw::nearlyEqual(_actual, _expected, _eps)) { \
            std::ostringstream _oss; \
            _oss << "Expected near, actual=" << _actual << ", expected=" << _expected << ", epsilon=" << _eps; \
            testfw::fail(__FILE__, __LINE__, _oss.str()); \
        } \
    } while (0)

#define EXPECT_NOT_NULL(pointerValue) \
    do { \
        testfw::assertionCount()++; \
        if ((pointerValue) == nullptr) { \
            testfw::fail(__FILE__, __LINE__, "Expected non-null pointer"); \
        } \
    } while (0)

#define EXPECT_NULL(pointerValue) \
    do { \
        testfw::assertionCount()++; \
        if ((pointerValue) != nullptr) { \
            testfw::fail(__FILE__, __LINE__, "Expected null pointer"); \
        } \
    } while (0)
