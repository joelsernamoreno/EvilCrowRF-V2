# EvilCrowRF V2 Attack Documentation

## Table of Contents

1. [Introduction](#introduction)
2. [Attack Types](#attack-types)
   - [Mousejacking](#mousejacking)
   - [Rolljam](#rolljam)
   - [Bruteforce](#bruteforce)
   - [DIP Switch](#dip-switch)
   - [Jamming](#jamming)
3. [Configuration Parameters](#configuration-parameters)
4. [Safety and Legal Considerations](#safety-and-legal-considerations)

## Introduction

This document provides detailed information about the various attack types supported by EvilCrowRF V2, their configuration parameters, and usage guidelines. This information is provided for educational and security research purposes only.

## Attack Types

### Mousejacking

Mousejacking attacks exploit vulnerabilities in wireless keyboard and mouse protocols.

**Capabilities:**

- Keyboard injection
- Mouse movement simulation
- Multimedia key simulation

**Parameters:**

- `payloadType`: Type of HID device to emulate (0=keyboard, 1=mouse, 2=multimedia)
- `targetVID`: Target vendor ID in hex
- `targetPID`: Target product ID in hex

**Example Usage:**

```json
{
  "type": "MOUSEJACKING",
  "frequency": 2.405,
  "modulation": 2,
  "params": {
    "payloadType": 0,
    "targetVID": "0x046D",
    "targetPID": "0xC52B"
  }
}
```

### Rolljam

Rolljam attacks capture and replay rolling code signals used in automotive and gate systems.

**Capabilities:**

- Signal recording
- Jamming transmission
- Replay captured signals

**Parameters:**

- `recordTime`: Time to record signals in milliseconds
- `jamTime`: Duration of jamming in milliseconds
- `replayCount`: Number of times to replay captured signal

**Example Usage:**

```json
{
  "type": "ROLLJAM",
  "frequency": 433.92,
  "modulation": 0,
  "params": {
    "recordTime": 1000,
    "jamTime": 2000,
    "replayCount": 3
  }
}
```

### Bruteforce

Systematically tests combinations of codes to find valid signals.

**Capabilities:**

- Sequential code generation
- Code validation
- Protocol-specific patterns

**Parameters:**

- `startCode`: Starting code in hex
- `endCode`: Ending code in hex
- `codeLength`: Length of code in bits

**Example Usage:**

```json
{
  "type": "BRUTEFORCE",
  "frequency": 315.0,
  "modulation": 0,
  "params": {
    "startCode": "0x000000",
    "endCode": "0xFFFFFF",
    "codeLength": 24
  }
}
```

### DIP Switch

Tests combinations of DIP switch positions commonly used in garage doors and gates.

**Capabilities:**

- Generate all possible switch combinations
- Protocol-specific encodings
- Optimized sequence generation

**Parameters:**

- `switchCount`: Number of DIP switches (1-12)
- `startState`: Initial switch state in binary
- `endState`: Final switch state in binary

**Example Usage:**

```json
{
  "type": "DIP_SWITCH",
  "frequency": 433.92,
  "modulation": 0,
  "params": {
    "switchCount": 8,
    "startState": "00000000",
    "endState": "11111111"
  }
}
```

### Jamming

Various techniques for RF signal interference.

**Capabilities:**

- Constant frequency jamming
- Frequency sweep jamming
- Random frequency jamming

**Parameters:**

- `jamType`: Jamming technique (0=constant, 1=sweep, 2=random)
- `sweepStart`: Start frequency for sweep (MHz)
- `sweepEnd`: End frequency for sweep (MHz)
- `sweepSteps`: Number of steps in frequency sweep

**Example Usage:**

```json
{
  "type": "JAMMING",
  "frequency": 433.92,
  "modulation": 0,
  "params": {
    "jamType": 1,
    "sweepStart": 433.0,
    "sweepEnd": 434.0,
    "sweepSteps": 10
  }
}
```

## Configuration Parameters

### Common Parameters

- `frequency`: Center frequency in MHz
- `modulation`: Modulation type (0=ASK/OOK, 2=2-FSK, 3=4-FSK, 4=MSK)
- `deviation`: Frequency deviation for FSK modes
- `attackDuration`: Maximum duration in milliseconds (0=unlimited)

### Attack Monitoring

All attacks provide real-time monitoring of:

- Success/failure counts
- Signal strength
- Current progress
- Status messages

## Safety and Legal Considerations

**WARNING:** These tools are provided for educational and security research purposes only. Usage must comply with all applicable laws and regulations.

### Legal Requirements

- Obtain necessary permissions before testing
- Respect frequency regulations
- Follow responsible disclosure procedures
- Document all testing activities

### Safety Guidelines

1. Never interfere with emergency or safety equipment
2. Test in controlled environments only
3. Keep detailed logs of all activities
4. Use minimum power necessary for testing
