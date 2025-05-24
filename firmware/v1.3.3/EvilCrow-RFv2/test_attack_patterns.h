// test_attack_patterns.h
#ifndef TEST_ATTACK_PATTERNS_H
#define TEST_ATTACK_PATTERNS_H

#include <Arduino.h>
#include "AttackManager.h"

class TestAttackPatterns
{
public:
    static void runAll();

private:
    static void testMousejackPayloads();
    static void testDIPCodes();
    static void testBruteforceValidation();

    static void assertTrue(bool condition, const char *message);
    static void assertEqual(int expected, int actual, const char *message);
    static void assertArrayEqual(uint8_t *expected, uint8_t *actual, size_t length, const char *message);

    static int passedTests;
    static int totalTests;
};

#endif // TEST_ATTACK_PATTERNS_H
