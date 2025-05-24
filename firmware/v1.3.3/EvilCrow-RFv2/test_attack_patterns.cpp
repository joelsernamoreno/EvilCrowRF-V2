// test_attack_patterns.cpp
#include "test_attack_patterns.h"

int TestAttackPatterns::passedTests = 0;
int TestAttackPatterns::totalTests = 0;

void TestAttackPatterns::runAll()
{
    Serial.println("\nRunning Attack Pattern Tests...");

    testMousejackPayloads();
    testDIPCodes();
    testBruteforceValidation();

    Serial.print("\nTest Results: ");
    Serial.print(passedTests);
    Serial.print("/");
    Serial.print(totalTests);
    Serial.println(" tests passed");
}

void TestAttackPatterns::testMousejackPayloads()
{
    Serial.println("\nTesting Mousejack Payloads:");

    // Test keyboard payload
    uint8_t keyboardLength;
    uint8_t *keyboardPayload = AttackManager::generateMousejackPayload(0, &keyboardLength);

    assertTrue(keyboardLength == sizeof(KeyboardReport), "Keyboard payload length check");
    assertTrue(keyboardPayload[0] == 0x08, "Keyboard modifier check"); // Windows key
    assertTrue(keyboardPayload[2] == 0x15, "Keyboard key check");      // 'r' key

    // Test mouse payload
    uint8_t mouseLength;
    uint8_t *mousePayload = AttackManager::generateMousejackPayload(1, &mouseLength);

    assertTrue(mouseLength == 3, "Mouse payload length check");
    assertTrue(mousePayload[0] == 0x01, "Mouse button check");
    assertTrue(mousePayload[1] == 0x00 && mousePayload[2] == 0x00, "Mouse movement check");

    // Test multimedia payload
    uint8_t mediaLength;
    uint8_t *mediaPayload = AttackManager::generateMousejackPayload(2, &mediaLength);

    assertTrue(mediaLength == 2, "Multimedia payload length check");
    assertTrue(mediaPayload[0] == 0x02, "Multimedia command check");
}

void TestAttackPatterns::testDIPCodes()
{
    Serial.println("\nTesting DIP Switch Codes:");

    // Test 8-switch combinations
    uint32_t allOff = AttackManager::generateDIPCode(8, 0x00);
    assertTrue(allOff == 0x55555555, "All switches off check");

    uint32_t allOn = AttackManager::generateDIPCode(8, 0xFF);
    assertTrue(allOn == 0xAAAAAAAA, "All switches on check");

    uint32_t alternating = AttackManager::generateDIPCode(8, 0x55);
    assertTrue(alternating == 0x96969696, "Alternating switches check");

    // Test different switch counts
    uint32_t fourSwitches = AttackManager::generateDIPCode(4, 0x0F);
    assertTrue((fourSwitches & 0xFFFF) == 0xAAAA, "4-switch combination check");
}

void TestAttackPatterns::testBruteforceValidation()
{
    Serial.println("\nTesting Bruteforce Validation:");

    // Test valid codes (balanced 1s and 0s)
    assertTrue(AttackManager::validateBruteforceCode(0x55555555, 32), "Balanced code validation");
    assertTrue(AttackManager::validateBruteforceCode(0xAAAAAAAA, 32), "Inverted balanced code validation");

    // Test invalid codes (too many 1s or 0s)
    assertFalse(AttackManager::validateBruteforceCode(0xFFFFFFFF, 32), "All ones code validation");
    assertFalse(AttackManager::validateBruteforceCode(0x00000000, 32), "All zeros code validation");

    // Test edge cases
    assertTrue(AttackManager::validateBruteforceCode(0x33333333, 32), "Edge case validation (1)");
    assertTrue(AttackManager::validateBruteforceCode(0x0F0F0F0F, 32), "Edge case validation (2)");
}

void TestAttackPatterns::assertTrue(bool condition, const char *message)
{
    totalTests++;
    if (condition)
    {
        passedTests++;
        Serial.print("✓ ");
    }
    else
    {
        Serial.print("✗ ");
    }
    Serial.println(message);
}

void TestAttackPatterns::assertEqual(int expected, int actual, const char *message)
{
    totalTests++;
    if (expected == actual)
    {
        passedTests++;
        Serial.print("✓ ");
    }
    else
    {
        Serial.print("✗ ");
        Serial.print("Expected: ");
        Serial.print(expected);
        Serial.print(", Got: ");
        Serial.print(actual);
        Serial.print(" - ");
    }
    Serial.println(message);
}

void TestAttackPatterns::assertArrayEqual(uint8_t *expected, uint8_t *actual, size_t length, const char *message)
{
    totalTests++;
    bool equal = true;
    for (size_t i = 0; i < length; i++)
    {
        if (expected[i] != actual[i])
        {
            equal = false;
            break;
        }
    }

    if (equal)
    {
        passedTests++;
        Serial.print("✓ ");
    }
    else
    {
        Serial.print("✗ ");
    }
    Serial.println(message);
}
