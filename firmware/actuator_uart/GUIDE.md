# ACTUATOR UART (Uno, register-only)

The actor receives MAIN software-UART TX D3 on its hardware RX D0, and returns hardware TX D1 to MAIN software-UART RX D2 at 9600 baud. All grounds are common. The global ADAS alert net connects **ACTUATOR D3** and **MAIN D4** to the ADAS open-drain output, with one 4.7 kΩ pull-up to MAIN 5 V. **ACTUATOR D2 remains the local emergency-stop button**; it is not the ADAS alert pin.

Outputs retain the existing wiring: motor PWM D5, direction D4/D7, optional standby D12/PB4; light D8/PB0; servo D9/PB1; horn D6. Timer0 provides 1 kHz time and motor PWM, Timer1 supplies 50 Hz servo pulses, and Timer2 supplies the passive horn waveform.

D3/PD3/INT1 runs on any edge. On LOW the ISR immediately disables motor COM0B PWM and drives D5/D4/D7 LOW, lowers standby PB4 and light PB0, disables servo COM1A and lowers PB1, disables Timer2 interrupts and lowers D6. It then latches the ADAS danger event. This short register gate does not parse UART, calculate scores, write EEPROM, or wait. A rising edge cannot erase a prior danger pulse. Foreground forces the Engine OFF and cancels reversal/accessory staging. Hardware apply checks the sticky latch, live D3 and synchronization barrier in a brief critical register section, invalidating its cached applied state after every IRQ. A stalled or disconnected MAIN therefore cannot keep outputs energized.

HELLO payload is session32, model32 (`0x95616d56`), labels version 3. A new HELLO resets safety epoch to zero and requires STOP synchronization; an active same-session HELLO is idempotent. EEPROM retains the greatest accepted session and rejects older sessions after reboot.

Protocol 3 payloads:

- ACTUATE: session32, epoch32, intent8, duration16 (11 bytes).
- ACTUATE_BATCH: session32, epoch32, count8, up to four intents, duration16.
- STOP_ALL: session32, new epoch32, stop sources16 (10 bytes).
- STATUS/HEARTBEAT: session32 (4 bytes).
- ACK/STATUS_REPLY: session32, epoch32, drive8, lights8, wiper8, horn8, servo8, flags8, last applied command16, sources16 (18 bytes).

Status flags are reversal pending bit 0, local stop latched bit 1, active ADAS bit 2, and safety synchronization required bit 3. Commands require an exact epoch match and no active ADAS or synchronization barrier. Old epochs return STALE_SAFETY; inhibited commands return SAFETY_INHIBIT. Duplicate requests cannot restore stopped motion. Release leaves outputs OFF and requires a strictly newer STOP epoch before a fresh command. STOP while ADAS is still LOW cannot clear that barrier. ADAS retains the UART session for observation. Local D2 stop and watchdog remain global OFF paths that invalidate the session.

The drive lease stops only drive and reports source DRIVE_LEASE=64; accessories remain as requested. Global STOP and ADAS stop cancel all outputs and delayed reversal work. The sensor faces forward, but this baseline intentionally stops reverse and accessories as well. It is a tabletop interlock and makes no vehicle safety claims.

Build using `arduino:avr:uno`. `adas_arbiter_actuator_register_test.cpp` checks direct register gating, a low-high pulse before foreground execution, epoch barriers, and alert arrival in a partial UART frame. No physical sensors or drivers were connected or uploaded during these checks.
