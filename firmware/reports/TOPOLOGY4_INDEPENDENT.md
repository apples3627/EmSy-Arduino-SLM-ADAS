# Independent four-Uno topology verification

The scenario list was sealed before reading the new MAIN, SLM_UART and
PERIPHERAL_SPI implementations. `firmware/verification/topology4/scenario_seal.json`
contains its SHA-256. The verifier writes only its own tests and reports, compiles
the implementation directly from a supplied source root, and does not run source
preparation, modify firmware, upload sketches, or open serial ports.

Required command:

```sh
python3 firmware/verification/topology4/run.py --source-root /absolute/source/root --build-uno --verify-packages
```

The host register shim records the actual firmware register writes. It does not
model real pin voltage, interrupt latency, analog motor behavior, or a physical
I2C/SPI/UART bus. Uno builds verify the AVR target compilation and static memory
limits; physical hardware remains unverified.

## Acceptance map

| Requirement | Evidence required |
|---|---|
| Session and epoch authority, ACK gating | Actual MAIN controller + actual peer replies; old ACK and old epoch rejection |
| Two L298N motors on D9/D10, D8 SPI CS | Actual MAIN register functions and ISR; both OCR registers zero on ALERT; SS stays output |
| Timer separation and UART timing | Actual Timer0/1/2 register initialization and transmit/receive ISR constants |
| Reversal dead time and lease | Controller clock boundary checks at 99/100 ms and 499/500 ms |
| Sticky ALERT and recovery | Low/release pulse while foreground is stalled; STOP barrier and fresh command required |
| Unsupported wipers and STOP | Actual SLM planner and MAIN/peripheral whole-plan validator |
| Observer state | Actual SLM/peripheral STATE_SYNC decode after STOP; no output changes from snapshots |
| Full model | Frozen PC scores for model `0x95616d56`, repeatedly through actual SLM worker |
| ADAS safety thresholds | Boundary, three-clear, stale and clock-wrap policy checks |
| LCD absence | Bounded I2C fault path while actual SPI receive and command reply remain available |
| Deployment artifacts | Four Uno builds and separately inspected standalone role packages |

Final outcomes and exact commands are recorded in
`firmware/verification/topology4/results.json`. No PASS is claimed until all
required source failures have been fixed and rechecked.

## Final result: PASS

Verified frozen integrated source commit
`21057b26c632ac5246325c8e14c1abc1c2522bcd`. All active-role source hashes remained
unchanged for the complete verification run. Protocol is version 4, labels remain
version 3, and the full model remains `0x95616d56`.

All 11 host checks passed. The integration uses the actual MAIN controller, SLM
session/planner/model and peripheral engine in separate translation units, with
framed UART delivery and simulated request/reply timing. It covers old ACKs and
epochs, pending command cancellation, duplicate STOP both before and after its
peripheral ACK, STOP while an SPI status query is in flight, 100 ms reversal,
500 ms drive lease, both link expiry paths, whole-plan wiper rejection, STOP
precedence, observer snapshots and fresh command recovery after a brief ALERT.
The SLM check compares 50 repeated evaluations against frozen PC scores. ADAS
policy, capture ISR and board checks cover threshold, timeout, stale sample,
clock wrap, open-drain ALERT and immutable replies.

The MAIN register test verifies both Timer1 PWM enable channels and direction
pairs, D10 configured as an output while D8 independently selects SPI, and Timer2
TX/RX increments of 208/104 ticks without changing Timer1. The actual ALERT ISR
disconnects both PWM compare outputs and drives both enable pins low. This is the
electrical off criterion; OCR duty registers may retain their prior value while
the compare outputs are disconnected. The sealed scenario wording named zero
OCR registers, but the supplied requirement is immediate motor OFF, which this
register check verifies through the pin gates.

The peripheral register check runs both active buzzer/0x27 LCD and passive
buzzer/0x3F LCD configurations. A stuck LCD transaction expires while actual SPI
ISRs still process status and command frames. A separate test reconstructs LCD
characters from actual PCF8574 expander writes: local accessories display off
after ALERT, and drive becomes unknown during inhibition or when the retained
snapshot has an older epoch.

Three observed source issues were corrected and rechecked: peripheral STOP
precedence originally rejected a supported STOP mixed with deferred wipers or
opposing motion; MAIN originally errored an identical STOP retry before its
peripheral ACK; LCD originally displayed a stale moving/accessory snapshot after
ALERT. All corresponding regressions pass against the frozen source.

| Uno role | Flash bytes | Static SRAM bytes | Static SRAM spare |
|---|---:|---:|---:|
| MAIN | 10,266 | 753 | 1,295 |
| SLM_UART | 21,354 | 1,299 | 749 |
| ADAS_I2C | 3,984 | 291 | 1,757 |
| PERIPHERAL_SPI | 6,296 | 448 | 1,600 |

All four source sketches and all four separately extracted packages compiled
with `arduino:avr:uno`. The package verifier checked the exact four-role
manifest, archive and member SHA-256 hashes, protocol/model/labels versions,
role-local sketch names and every quoted source include. Source files matched
their archives, allowing only the documented GUIDE link normalization for ZIP
contents. Static SRAM spare is a linker value, not a measured runtime stack.
Hardware behavior, actual baud timing and motor load remain unverified.
