#ifndef ATTACK_MANAGER_H
#define ATTACK_MANAGER_H

#include <Arduino.h>
#include "TaskScheduler.h"
#include "RFSignalProcessor.h"

// AttackType is defined in AttackSelector.h

// Attack progress and status tracking
struct AttackProgress
{
    uint32_t startTime;
    uint32_t lastUpdateTime;
    uint32_t successCount;
    uint32_t totalAttempts;
    uint32_t failedAttempts;
    float signalStrength;
    String lastStatus;
    String currentAction;
};

// Attack configuration with protocol-specific parameters
struct AttackConfig
{
    AttackType type;
    uint32_t frequency;
    uint8_t modulation;
    uint16_t deviation;
    uint32_t attackDuration;
    String targetProtocol;
    bool enabled;
    uint32_t timeout;
    String description;

    // Protocol-specific parameters
    union
    {
        struct
        {
            uint8_t payloadType; // 0=keyboard, 1=mouse, 2=multimedia
            uint32_t targetVID;  // Vendor ID
            uint32_t targetPID;  // Product ID
        } mousejack;

        struct
        {
            uint32_t recordTime; // Time to record before jamming (ms)
            uint32_t jamTime;    // Time to jam (ms)
            uint8_t replayCount; // Number of times to replay captured signal
        } rolljam;

        struct
        {
            uint32_t startCode; // Starting code for bruteforce
            uint32_t endCode;   // Ending code for bruteforce
            uint8_t codeLength; // Length of code in bits
        } bruteforce;

        struct
        {
            uint8_t switchCount; // Number of DIP switches
            uint8_t startState;  // Initial switch state
            uint8_t endState;    // Final switch state
        } dipswitch;

        struct
        {
            uint8_t jamType;    // 0=constant, 1=sweep, 2=random, 3=targeted
            float sweepStart;   // Start frequency for sweep jamming
            float sweepEnd;     // End frequency for sweep jamming
            uint8_t sweepSteps; // Number of steps in frequency sweep
        } jamming;
    } params;

    class AttackManager
    {
    public:
        AttackManager(TaskScheduler &scheduler, RFSignalProcessor &rfProcessor);

        bool startAttack(const AttackConfig &config);
        void stopAttack();
        bool isAttackRunning() const;
        String getAttackStatus() const;
        AttackProgress &getProgress();

        // Protocol-specific helper methods
        static uint8_t *generateMousejackPayload(uint8_t type, uint8_t *length);
        static uint32_t generateDIPCode(uint8_t switchCount, uint8_t state);
        static bool validateBruteforceCode(uint32_t code, uint8_t length);

    private:
        TaskScheduler &taskScheduler;
        RFSignalProcessor &rfProcessor;
        bool attackRunning;
        AttackConfig currentConfig;
        AttackProgress progress;
        TaskHandle_t attackTask;
        TaskHandle_t monitorTask;

        // Queue for attack progress updates
        QueueHandle_t statusQueue;

        // Attack implementation methods
        void runMousejackingAttack();
        void runRolljamAttack();
        void runBruteforceAttack();
        void runDipSwitchAttack();
        void runJammingAttack();

        // Helper methods
        static void attackTaskFunction(void *parameter);

        std::unique_ptr<SmartHomeAttack> smartHomeAttack;
    };
};

#endif // ATTACK_MANAGER_H
