# Independent four-Uno topology verification

The scenario list was sealed before reading the new MAIN, SLM_UART and
PERIPHERAL_SPI implementations. `firmware/verification/topology4/scenario_seal.json`
contains its SHA-256. The verifier writes only its own tests and reports, compiles
the implementation directly from a supplied source root, and does not run source
preparation, modify firmware, upload sketches, or open serial ports.

Required command:

```sh
python3 firmware/verification/topology4/run.py --source-root /absolute/source/root --build-uno
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
