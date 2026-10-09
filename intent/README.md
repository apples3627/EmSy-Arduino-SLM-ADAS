# Trained English command model for Uno R3

V4 is a real trained neural clause classifier: learned word embeddings (16), mean pooling, ReLU (64), and ten logits. The deterministic command planner splits an input into explicit clauses, runs that same neural classifier on each clause, and validates the complete action batch. There is no action keyword dictionary and no PC inference fallback in the deployed firmware.

## Try it

```sh
python3 intent/tools/infer.py 'go forward and turn on the lights'
python3 intent/tools/infer.py 'lights off and horn on'
python3 intent/tools/infer.py 'honk'
python3 intent/tools/verify.py
python3 intent/tools/verify_plan.py
sh intent/tools/build.sh
```

The CLI always returns a complete plan: numeric `actions`, `action_names`, `reason`, normalized `clauses`, each clause's effective label and raw scores, and the resulting policy. `infer_plan(text)` is the corresponding Python API. Legacy `infer(text)` remains a **single-clause** reference API: it rejects compound inputs and does not claim to classify the whole compound sentence.

## Grammar and safety policy

Inputs contain at most 192 printable ASCII bytes and four explicit clauses. Each clause is limited to 96 bytes and 16 tokens. Supported delimiters are the whole words `and`, `also`, `plus`, comma, and semicolon. Comma/semicolon may be followed by one connector; `and also` is accepted. Empty clauses, repeated connectors, trailing separators, overlong inputs, and more than four clauses reject the complete input. Case is ignored; surrounding spaces and contiguous terminal `. ! ?` characters are trimmed. Internal punctuation, control characters, and non-ASCII input reject.

Each clause needs its own explicit action/state, such as `lights off and horn on`. `turn on lights and horn` rejects because bare `horn` is ambiguous. Bare objects including `lights`, `wipers`, and `horn` were trained as UNKNOWN; `honk` is a trained HORN_ON expression. Clauses are classified separately, so opposite states bind to their own objects.

A successfully recognized STOP anywhere in the parsed batch dominates every other clause, including UNKNOWN, and yields `[2]`. Repeated actions collapse in first occurrence order. FORWARD+REVERSE or ON+OFF for one component reject the complete batch. Any UNKNOWN, OOV, guarded clause, invalid input, or confidence rejection rejects the complete batch with GLOBAL_STOP; a valid prefix is never actuated.

This is a batch of independent actions. `then`, `or`, conditionals, negation, timing, and ambiguous object ellipsis are unsupported and conservatively reject. The mean-pooling classifier cannot understand general language or word order. Its 245-word vocabulary is derived from TRAIN only; unseen words reject.

## Stable labels

| ID | Intent |
|---|---|
| 0 / 1 | FORWARD / REVERSE |
| 2 | STOP: motor stop, lights OFF, wiper OFF, horn OFF |
| 3 / 4 | LIGHT_ON / LIGHT_OFF |
| 5 / 6 | WIPER_ON / WIPER_OFF |
| 7 | UNKNOWN: complete GLOBAL_STOP |
| 8 / 9 | HORN_ON / HORN_OFF |

`uno_probe/src/command_plan.h` provides bounded `Split`, `extract`, and `Decisions` APIs. Splitting stores raw spans rather than duplicating the whole input. The probe uses a fixed 193-byte line buffer and no heap. It prints legacy RAW/EFFECTIVE for a single valid clause and PLAN count/actions/reason for the full model. Parts 1 and 2 print partial scores for each clause; those scores require the original output bias exactly once when combined.

## Training and evidence

`dataset/generate.py` authors individual clauses and grouped request templates. V4 has 53,504 globally deduplicated rows (40,596 TRAIN, 5,532 VAL, 7,376 TEST). Request template families are separate; base phrases/vocabulary are intentionally shared. Actual NumPy Adam training runs 1,600 steps and uses minimum validation loss for checkpoint selection; int8 threshold/margin selection uses VAL only (0.5 / 0.0). Test data are excluded from vocabulary and all parameter selection.

`dataset/generate_multi.py` freezes 1,818 composition cases before final training, with 606 per split. Compound text never enters neural gradients. These cases combine **known TRAIN clauses**, measure composition rather than unseen lexical understanding, and use disjoint delimiter families with globally unique text. Multi-validation gates at >=95% exact actions and zero unsafe accepted mismatches before frozen test evaluation. Both validation and frozen composition test reached 606/606 exact actions with zero unsafe accepted mismatches. These synthetic figures do not establish hardware accuracy or general language coverage.

Detailed single-clause confusion matrices, coverage, and raw versus guarded results are in `metrics.json`. Frozen single-clause guarded test accepted actions are all correct; coverage is about 98.25%. Python/C++ scalar and two-shard checks cover 1,775 train/contract contexts. Full and both shard Uno probes compile using AVR core 1.8.6: full 16,164 bytes flash / 543 bytes globals, each shard 13,892 / 555. No V4 hardware port was available for upload or actuation testing. Earlier hardware logs describe V3 only.

The V3 metadata/metrics/weight hash snapshot is `experiments/v3_before_multi.json`. V4 training, compile, and parity evidence lives in `experiments/v4_*`. Independent new sealed audit cases were not read during model development. The complete trainable implementation, dataset generators, exported int8 tensors, full/shard headers, and metadata are checked in.
