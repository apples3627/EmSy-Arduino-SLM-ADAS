# MAIN (Uno, register-only)

The four-board topology is MAIN → full H64 SLM over SPI, MAIN → HC-SR04 ADAS over I2C, and MAIN → ACTUATOR over UART. The learned model is `0x95616d56`, V245/E16/H64, label version 3. The existing planner accepts up to four clauses and sends one final actuation or batch. The SLM returns all ten scores including global bias; MAIN initializes scores to zero and never adds another bias or a second worker score.

## Connections

| MAIN pin | Connection |
|---|---|
| D0 RX / D1 TX | USB hardware UART, 9600 baud |
| D2 RX / D3 TX | ACTUATOR D1 TX / D0 RX, software UART 9600 |
| D10 / D11 / D12 / D13 | SLM SS / MOSI / MISO / SCK, mode 0, MSB first, 125 kHz |
| A4 / A5 | ADAS SDA / SCL, address `0x12`, 100 kHz |
| **D4** | **ADAS active-low alert net**, also ACTUATOR **D3** |
| GND | Common ground across all four boards |

Use one 4.7 kΩ pull-up from the alert net to MAIN 5 V. ADAS drives this net as open drain. MAIN D2 is UART RX and must never be attached to the alert net. ACTUATOR D2 remains the local emergency-stop button.

## Safety behavior

MAIN D4 uses PD4/PCINT20/PCINT2. Its ISR only latches a falling event; a rising edge cannot erase an earlier danger event. Foreground services this latch before PC input, replies, and neural work. Danger cancels the plan and queued UART command, increments the safety epoch, requests all outputs OFF, and marks observed output state unknown. Only matching session/epoch ACK or STATUS confirms actual OFF state. An in-progress UART frame is allowed to finish; ACTUATOR's independent interrupt prevents it from restoring outputs.

I2C sends ADAS metadata, telemetry and shared state, never neural tokens. Polls continue every 70 ms during inference. Metadata must match HC-SR04, stop 20 cm, release 25 cm, three clear samples, protocol 3 and 70 ms period. Missing/invalid transactions, sample age above 200 ms, startup, invalid echo or sensor fault inhibit every actuator output. A physically LOW alert always wins over a clear telemetry report.

Release requires HIGH alert plus valid fresh telemetry and three clear samples. MAIN sends a strictly newer STOP epoch and waits for actual OFF ACK before READY. Previous input and inference are discarded; motion requires a fresh user command. This front sensor implements global ALL_STOP, including reverse and accessories. It is a tabletop interlock, not a vehicle ADAS implementation or road-safety system.

ACTUATOR HELLO resets its epoch and requires initial STOP synchronization. ACTUATE and ACTUATE_BATCH carry the current session and safety epoch; older epochs cannot resume outputs. STOP carries the new epoch and source mask. The actor preserves its UART session during ADAS inhibition but watchdog and local emergency stop retain their existing session invalidation behavior.

## Shared state and diagnostics

MAIN owns the requested state and separately observes the actor's actual state. The 22-byte shared record contains session, revision, safety epoch, command ID, desired/applied drive, accessory flags, source mask, applied-known flag and actor flags. MAIN sends STATE_SYNC to SLM and ADAS when changed and at least every 500 ms, checks ACKs, and waits for a free bus phase so snapshots cannot replace neural transactions. Query/snapshot IDs never advance the actuation command counter. Drive lease expiration reconciles desired and applied drive to zero without falsely clearing latched accessories.

`!status` works while a neural job is running. It prints readiness and a bounded STATE line containing requested/applied signed drive, known flag, source mask, epoch, distance in mm, telemetry flags and fresh clear state. `!trace` identifies the scheduler. Each neural clause first obtains real ACTUATOR STATUS, while ADAS polling and asynchronous SPI computation retain their independent bus schedules. `stop` takes the priority path and source `SLM_STOP=1` without inference. `!retry` starts a new persisted UART session.

Source bits: SLM stop 1, ADAS obstacle 2, ADAS sensor fault 4, communication fault 8, user emergency stop 16, boot synchronization 32, drive lease 64. Sources describe the last cause; actor active inhibition is separately represented by flags.

## Verification

Build with Arduino Uno FQBN `arduino:avr:uno`. MAIN uses only AVR registers for UART, timers, SPI, TWI and IRQs. No sensors or drivers are physically connected or uploaded by this change. Host tests `adas_arbiter_main_test.cpp` and `adas_arbiter_actuator_register_test.cpp` cover real MAIN scheduling, real actor logic, full model scores, direct IRQ register gating, sticky low-high pulses, old epochs, partial UART frames, stale telemetry, shared OFF state, SLM stop, and motor lease reconciliation.
