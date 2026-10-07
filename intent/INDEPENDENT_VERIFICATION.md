# 독립 검증 결과 — 최종 V3

최종 모델은 **영어 명령용 소형 신경망 분류기**입니다. LLM이나 범용 영어 이해 모델은 아닙니다. 최종 모델 공개 전에 별도로 봉인한 최종 평가 51개에서 모두 올바른 결정을 냈습니다. 학습 문장과 일치하는 2개를 제외한 **49개(정상 명령 34개, UNKNOWN 15개)**에서도 정확도와 macro F1이 100%, 위험한 오작동은 0개였습니다. 이 작은 표본의 결과가 모든 영어 표현의 안전성을 보장하지는 않습니다.

독립적으로 실제 Uno R3 컴파일을 수행했습니다. 전체 모델은 Flash 14,340 / 32,256 바이트, 정적 SRAM 413 / 2,048 바이트입니다. 두 분할 모델은 각각 Flash 12,620 바이트, 정적 SRAM 425 바이트입니다. 보드는 연결하지 않았고 업로드, I2C/SPI/UART 물리 통신, 모터·등·와이퍼·피에조 구동은 검증하지 않았습니다.

## Frozen identity and sealed provenance

Final source commit: `129763735099aba77883ae2ac137715c621a8bfc`. Model ID: `30918170`. Weight SHA256 is recorded independently in `verification/final_v3_summary.json` and `verification/final_corpus_recomputation.json`; it matches metadata. Architecture: learned 247-word embedding, 16 features, mean pooling, 64 ReLU hidden units, 10 logits. Quantized weight payload is 5,924 bytes including float biases/scales. Input contract: at most 96 printable ASCII bytes and 16 tokens; lowercase, repeated-space collapse and terminal `. ! ?` trimming; unsupported/OOV input rejects to UNKNOWN/global stop. Confidence 0.9 and probability margin 0.2 were selected on validation.

| Audit | Seal commit | Holdout SHA256 | First evaluation model |
|---|---|---|---|
| V1 | `fac1523` | `86395756d7e46536bd0315325557f9ea7a8b7f66f2564aa00a0a4d7b6f2536fe` | `54d239ec` |
| V2 | `e1eaff5` | `d5ae57f037063ef0ab64a9e72610c0c3f8971f875f22a09fe606f9eb1dfb10e9` | `f044b79e` |
| V3 | `2e742a8` | `50df3a33a03a50d73979a724987e9a1e780f11d3bf5bc109111bd48eda285f78` | `30918170` |

Each holdout was written and committed before that model was disclosed to the verifier. The trainer received aggregate failure categories after V1 and V2, without literal cases. Each new holdout was sealed before the next weights were disclosed. V1/V2 on final V3 are post-audit regressions. The held-out samples differ across versions, so first-audit percentages are not a controlled same-dataset improvement curve. Existing case manifests and failed-version reports remain preserved; the `intent-audit-history` branch retains the sealing commits.

## Final first evaluation and regressions

Primary result: 49 V3 semantic cases excluding exact normalized TRAIN matches, **49/49 correct**, macro F1 1.0; known-command acceptance/correctness 34/34, UNKNOWN rejection 15/15, dangerous false actions 0. There are two exact TRAIN phrase overlaps among all 51 V3 cases; none overlap VAL or TEST. Across all 51, all 36 known commands are in vocabulary and their raw neural prediction is correct. Seven UNKNOWN cases reject before neural evaluation due to OOV words. Of 44 encodable cases, raw neural accuracy is 41/44 (93.18%), macro F1 0.94359; three UNKNOWN raw errors are corrected by the generic rejection guard. For the primary 49-case nonoverlap subset, raw encodable accuracy is 39/42 (92.86%); seven UNKNOWN cases reject before scoring. Effective accuracy is 51/51. This distinction prevents attributing guard behavior to learned semantic understanding.

| Final V3 evaluation scope | Cases | Effective accuracy | Known-command correctness | Dangerous false actions |
|---|---:|---:|---:|---:|
| Fresh V3, exclude TRAIN matches | 49 | 100% | 34/34, 100% | 0 |
| Fresh V3, all cases | 51 | 100% | 36/36, 100% | 0 |
| V1 post-audit regression, exclude TRAIN matches | 71 | 94.37% | 50/54, 92.59% | 0 |
| V1 post-audit regression, all cases | 110 | 96.36% | 86/90, 95.56% | 0 |
| V2 post-audit regression, exclude TRAIN matches | 73 | 97.26% | 51/53, 96.23% | 0 |
| V2 post-audit regression, all cases | 83 | 97.59% | 61/63, 96.83% | 0 |

V1 regressions have 39 TRAIN matches and four known-command OOV rejections (`into`, `accessories`, `output`). V2 regressions have 10 TRAIN matches and two known-command OOV rejections (`with`). Neither regression set overlaps VAL or TEST. Every final encodable known command in the three audits is correctly accepted; the six remaining positive failures are OOV rejection. Reported whole-set acceptance includes those failures. OOV safety rejection narrows supported English; it does not constitute successful command understanding.

Initial V1 failed: 60/90 known-command correctness, 28 OOV rejections, one confidence rejection and one unsafe FORWARD-to-WIPER_ON action; V1 also failed AVR compilation. Initial V2 failed the broad acceptance target: 50/63 known-command correctness, 12 OOV rejections and one confidence rejection, with no dangerous action. See `verification/report_v1.md` and `verification/report_v2.md` for unchanged first-evaluation evidence.

## Independent model and dataset checks

All 41,121 final corpus rows were independently normalized and audited: TRAIN 31,202, VAL 4,251, TEST 5,668. Normalized cross-split duplicates: zero. Whole template-family assignments are disjoint by split. Base action phrases and vocabulary are intentionally shared, so synthetic test measures recombination of authored phrases, not free-form English generalization. The vocabulary is independently confirmed to use TRAIN only. Dataset, vocabulary and model hashes match metadata. Minimum logged validation-loss checkpoint matches selected step 1,600; code uses validation only for checkpoint and confidence/margin calibration, constructs TEST features after freeze, and asserts validation coverage/safety before TEST evaluation.

Independent vectorized int8 computation over every final corpus row gives raw accuracy 100% and final guarded TEST accuracy 99.8589%, macro F1 0.998662, positive coverage/correctness 99.8409%, dangerous false actions 0. All final corpus mistakes are confidence rejections, not accepted wrong actions. These synthetic metrics are supporting evidence, not a replacement for sealed evaluation.

Every full-model and shard embedding/hidden/output tensor and bias in the canonical C++ headers was independently parsed and compared with the NPZ payload. All match. Int8 tensors match quantization of the learned float tensors. Both shards preserve the original full tensor scales, split hidden units 0–31 and 32–63, and omit global output bias; main adds that bias once. No layer-local renormalization is introduced.

Independent float64 finite differences on 30 parameter entries cover embedding, hidden weights/bias and output weights/bias, at initial and final trained values. Maximum absolute derivative discrepancy is 4.18e-11. Training forward/backprop code is unchanged from V2; checks were nevertheless repeated against final weights. V1's overwritten gradient reporting variable was a reporting defect; later reports preserve the actual checked gradient.

## Actual C++ and Uno checks

Actual compiled C++ full kernel was compared with the Python portable reference on 471 cases: 210 evenly sampled contexts (seven per split and label), every case from all three audits, and contract boundaries. Effective-decision mismatches: zero; maximum score discrepancy 7.62e-6. Actual separately compiled shard kernels on 431 encodable cases plus one main output bias differ from Python full scores by at most 1.48e-5. This is sampled final parity; V2 had exhaustive 23,468-case C++ parity, and the kernel algorithm is unchanged.

Independent tests cover 96/97-byte and 16/17-token boundaries, ASCII rejection, embedded NUL, internal punctuation, newline rejection, generic guard categories, nonfinite scores, empty kernel input and out-of-range token IDs. Padded transport IDs at noncontiguous positions produce the same pooled logits as parsed text; empty/all-PAD and out-of-range blocks reject. STOP and UNKNOWN clear drive, lights, wiper and horn flags. Accessory-only OFF preserves other outputs, and HORN_ON/OFF changes the horn flag. These are software contracts, not measured piezo `tone()`/`noTone()` behavior.

Final MCU inference code in `uno_probe/src/intent_model.h` contains no dynamic heap allocation; model/vocabulary arrays use AVR program memory. The host test uses standard strings for test I/O only. Static SRAM figures do not measure maximum stack depth. Independent actual Arduino CLI builds pass for full/I2C/SPI variants: 14,340/413, 12,620/425, 12,620/425 Flash/static-SRAM bytes respectively. All fit Uno R3 limits of 32,256/2,048. Tools: Python 3.14.0, NumPy 2.3.4, Apple clang via g++, Arduino CLI 1.5.1, AVR core 1.8.6.

The bounded acceptance target is met on the fresh final holdout, with residual finite-vocabulary limits disclosed. No final weights were tuned after V3 results. Physical four-board integration remains outside these host/compile checks.

## Evidence and repeatable checks

- `verification/final_v3_summary.json`: primary/nonoverlap metrics, overlaps, failures and split counts.
- `verification/v3_results_audit3.jsonl`: untouched final cases, raw scores/predictions and effective decisions.
- `verification/v3_results_audit1.jsonl`, `v3_results_audit2.jsonl`: final regression details.
- `verification/final_corpus_recomputation.json`: all-corpus independent metrics, hashes and complete tensor/scale comparisons.
- `verification/cpp_audit_v3.json`, `gradient_audit_v3.json`, `build_v3.log`: numerical/gradient/Uno evidence.
- Run `python3 intent/verification/evaluate_final_holdouts.py` and `python3 intent/verification/recompute_final.py`, compile the three host binaries from `intent/tools/host.cpp`, then run `python3 intent/verification/check_cpp.py` and `python3 intent/verification/check_gradients.py`; compile/run `verification/transport_contract.cpp` for independent policy/token checks. `intent/tools/build.sh` builds all three Uno variants.
