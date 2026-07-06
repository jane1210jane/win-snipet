#pragma once
#include <functional>
#include <iostream>
#include <string>
#include <vector>

struct TestCase { const char* name; std::function<void()> run; };
struct SkipException : std::runtime_error { using std::runtime_error::runtime_error; };
inline std::vector<TestCase>& Tests() { static std::vector<TestCase> tests; return tests; }
struct TestRegistration { TestRegistration(const char* name, std::function<void()> run) { Tests().push_back({name, std::move(run)}); } };
#define TEST(name) static void name(); static TestRegistration reg_##name(#name, name); static void name()
#define REQUIRE(expr) do { if (!(expr)) throw std::runtime_error("Requirement failed: " #expr); } while (false)
#define SKIP(reason) throw SkipException(reason)
