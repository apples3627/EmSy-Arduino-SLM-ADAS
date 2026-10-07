#!/usr/bin/env python3
"""Acceptance test: original UTF-8 bytes -> full Uno probe -> observed intent.

The CPU model supplies comparison expectations only. It never supplies a serial
response, sends token IDs, or substitutes an answer when the board fails.
"""
import argparse
from collections import Counter
from datetime import datetime, timezone
import hashlib
import json
import os
from pathlib import Path
import platform
import re
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT))
import reference

REASONS = {'accepted': 0, 'non_ascii': 1, 'too_long': 2,
           'empty_or_nul': 3, 'unsupported': 4, 'empty': 5,
           'too_many_tokens': 6, 'oov': 7, 'safety_guard': 8,
           'unknown': 9, 'low_confidence': 10, 'nonfinite': 11}
BANNER = 'INTENT PROBE ONLY: no motors connected; READY'


def utc_now():
    return datetime.now(timezone.utc).isoformat()


def build_cases():
    """Frozen human labels plus explicit transport/normalization contracts."""
    source = ROOT / 'verification/sealed_holdout_v3.jsonl'
    cases = []
    for line in source.read_text().splitlines():
        row = json.loads(line)
        cases.append(dict(id=row['id'], text=row['text'],
                          semantic_label=reference.LABELS[row['expected']],
                          category=row['category'], framing='LF',
                          source='frozen_v3_holdout_reused_regression'))
    roots = [('go forward', 'FORWARD'), ('go backward', 'REVERSE'),
             ('stop everything', 'STOP'), ('turn the lights on', 'LIGHT_ON'),
             ('turn the lights off', 'LIGHT_OFF'), ('start the wipers', 'WIPER_ON'),
             ('stop the wipers', 'WIPER_OFF'), ('hello', 'UNKNOWN'),
             ('turn the buzzer on', 'HORN_ON'), ('silence the buzzer', 'HORN_OFF')]
    extra = [(f'root_{i}', text, label, 'root') for i, (text, label) in enumerate(roots)]
    extra += [('uppercase', 'PLEASE DRIVE AHEAD', 'FORWARD', 'normalization'),
              ('spaces', '  please   drive  ahead  ', 'FORWARD', 'normalization'),
              ('punctuation', 'please drive ahead.!?', 'FORWARD', 'normalization'),
              ('crlf', 'go forward', 'FORWARD', 'normalization'),
              ('bytes_96', 'go forward' + ' ' * 86, 'FORWARD', 'boundary'),
              ('tokens_16', ' '.join(['stop'] * 16), 'STOP', 'boundary'),
              ('bytes_97', 'go forward' + ' ' * 87, 'UNKNOWN', 'invalid'),
              ('tokens_17', ' '.join(['stop'] * 17), 'UNKNOWN', 'invalid'),
              ('non_ascii', 'go forward é', 'UNKNOWN', 'invalid'),
              ('nul', 'go\x00forward', 'UNKNOWN', 'invalid'),
              ('blank', '', 'UNKNOWN', 'invalid'),
              ('only_spaces', '   ', 'UNKNOWN', 'invalid'),
              ('internal_punctuation', 'go, forward', 'UNKNOWN', 'invalid'),
              ('control', 'go\tforward', 'UNKNOWN', 'invalid'),
              ('oov', 'teleport the car', 'UNKNOWN', 'invalid')]
    for cid, text, label, category in extra:
        cases.append(dict(id=cid, text=text, semantic_label=label, category=category,
                          framing='CRLF' if cid == 'crlf' else 'LF', source='manual_contract'))
        if category == 'invalid':
            cases.append(dict(id=cid + '_recovery', text='go forward',
                              semantic_label='FORWARD', category='recovery',
                              framing='LF', source='manual_contract'))
    return cases


def expected_for(text, model, meta):
    ids, why, guard = reference.prepare(text, meta['vocabulary'])
    if ids is None:
        return dict(raw_label=None, effective_label='UNKNOWN', reason=REASONS[why],
                    reason_name=why, response_kind='error')
    scores = reference.quant_logits(ids, model)
    label, why, _, _ = reference.decision(scores, guard, meta['threshold'], meta['margin'])
    return dict(raw_label=reference.LABELS[int(scores.argmax())],
                effective_label=reference.LABELS[label], reason=REASONS[why],
                reason_name=why, response_kind='classification')


def parse_response(lines):
    """Require exactly one complete full-model response before READY."""
    if len(lines) != 1:
        raise ValueError(f'Expected one response line; received {lines!r}')
    line = lines[0]
    match = re.fullmatch(r'RAW ([A-Z_]+) EFFECTIVE ([A-Z_]+) REASON (\d+)', line)
    if match:
        raw, effective, reason = match.groups()
        if raw not in reference.LABELS or effective not in reference.LABELS or int(reason) > 11:
            raise ValueError(f'Invalid classification: {line!r}')
        return dict(raw_label=raw, effective_label=effective, reason=int(reason),
                    response_kind='classification')
    match = re.fullmatch(r'ERROR (\d+)', line)
    if match and int(match[1]) in range(1, 8):
        return dict(raw_label=None, effective_label='UNKNOWN', reason=int(match[1]),
                    response_kind='error')
    if line == 'ERROR too_long -> GLOBAL_STOP':
        return dict(raw_label=None, effective_label='UNKNOWN', reason=2, response_kind='error')
    raise ValueError(f'Malformed or partial-model reply: {line!r}')


class LineReader:
    """Retain partial serial lines across short read timeouts."""
    def __init__(self, board):
        self.board = board
        self.pending = bytearray()

    def next_line(self, deadline):
        while time.perf_counter() < deadline:
            byte = self.board.read(1)
            if not byte:
                continue
            self.pending.extend(byte)
            if len(self.pending) > 1024:
                raise ValueError('Board output line exceeds 1024 bytes')
            if byte == b'\n':
                raw = bytes(self.pending)
                self.pending.clear()
                return raw.rstrip(b'\r\n').decode('ascii', errors='strict')
        raise TimeoutError('Timed out waiting for a complete board line')


def wait_startup(reader, log, timeout=5):
    deadline = time.perf_counter() + timeout
    while True:
        line = reader.next_line(deadline)
        log.append(dict(timestamp=utc_now(), line=line))
        if line == BANNER:
            return


def exchange(board, reader, payload, timeout, log):
    if b'\n' in payload[:-1] or not payload.endswith(b'\n'):
        raise ValueError('Payload must contain exactly one LF request delimiter')
    started = time.perf_counter()
    deadline = started + timeout
    count = board.write(payload)
    if count != len(payload):
        raise IOError(f'Short serial write: {count}/{len(payload)}')
    lines = []
    while True:
        line = reader.next_line(deadline)
        log.append(dict(timestamp=utc_now(), line=line))
        if line == 'READY':
            return parse_response(lines), (time.perf_counter() - started) * 1000
        lines.append(line)


def atomic_report(path, report):
    path.parent.mkdir(parents=True, exist_ok=True)
    temporary = path.with_name(path.name + '.tmp')
    temporary.write_text(json.dumps(report, indent=2, ensure_ascii=True) + '\n')
    os.replace(temporary, path)


def summarize(records):
    times = sorted(r['elapsed_ms'] for r in records if r.get('transport_pass'))
    def percentile(p):
        if not times:
            return None
        position = (len(times) - 1) * p
        lower = int(position)
        upper = min(lower + 1, len(times) - 1)
        return times[lower] + (times[upper] - times[lower]) * (position - lower)
    confusion = Counter(f"{r['semantic_label']}->{r.get('observed', {}).get('effective_label', 'NO_RESPONSE')}" for r in records)
    return dict(completed=len(records), transport_pass=sum(r.get('transport_pass', False) for r in records),
                parity_pass=sum(r.get('parity_pass', False) for r in records),
                semantic_pass=sum(r.get('semantic_pass', False) for r in records),
                recovery_pass=sum(r.get('semantic_pass', False) and r.get('parity_pass', False)
                                  for r in records if r['category'] == 'recovery'),
                recovery_total=sum(r['category'] == 'recovery' for r in records),
                timeouts=sum(r.get('error_type') == 'TimeoutError' for r in records),
                confusion=dict(confusion), rtt_ms=dict(p50=percentile(.5), p95=percentile(.95),
                                                       minimum=times[0] if times else None,
                                                       maximum=times[-1] if times else None))


def main(argv=None):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', required=True)
    parser.add_argument('--output', type=Path, default=ROOT / 'reports/single_uno_hardware.json')
    parser.add_argument('--repeat', type=int, default=2)
    parser.add_argument('--timeout', type=float, default=15)
    parser.add_argument('--baud', type=int, default=9600)
    args = parser.parse_args(argv)
    if args.repeat < 1 or args.timeout <= 0 or args.baud <= 0:
        parser.error('repeat, timeout, and baud must be positive')
    model, meta = reference.load_model()
    cases = build_cases()
    for case in cases:
        case['expected'] = expected_for(case['text'], model, meta)
    # Include an accepted request immediately after every parser rejection,
    # including OOV examples in the reused frozen holdout.
    expanded = []
    for index, case in enumerate(cases):
        expanded.append(case)
        if case['expected']['response_kind'] == 'error' and (index + 1 == len(cases) or cases[index + 1]['category'] != 'recovery'):
            recovery = dict(id=case['id'] + '_recovery', text='go forward',
                            semantic_label='FORWARD', category='recovery', framing='LF',
                            source='manual_contract')
            recovery['expected'] = expected_for(recovery['text'], model, meta)
            expanded.append(recovery)
    cases = expanded
    report = dict(started_at=utc_now(), port=args.port, baud=args.baud, repeat=args.repeat,
                  timeout_seconds=args.timeout, platform=platform.platform(), python=sys.version,
                  model_id=meta['model_id'], model_sha256=meta['model_sha256'],
                  model_file_sha256=hashlib.sha256((ROOT / 'model_int8.npz').read_bytes()).hexdigest(),
                  vocabulary_sha256=meta['vocabulary_sha256'], case_count=len(cases),
                  planned_requests=len(cases)*args.repeat,
                  case_sha256=hashlib.sha256(json.dumps(cases, sort_keys=True).encode()).hexdigest(),
                  interpretation='Reused frozen human-labeled regression + manual contracts; not a fresh generalization estimate. Label/reason parity only; probe does not print AVR logits. RTT includes UART transmission, inference, and output.',
                  dataflow='PC writes original text UTF-8 bytes plus LF/CRLF; Uno computes the observed response. CPU inference is only an expectation comparator; no fallback.',
                  startup_log=[], records=[])
    try:
        report['source_commit'] = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    except (OSError, subprocess.CalledProcessError):
        report['source_commit'] = None
    mismatches = [c['id'] for c in cases if c['expected']['effective_label'] != c['semantic_label']]
    report['cpu_manual_expectation_mismatches'] = mismatches
    board = None
    try:
        if mismatches:
            raise ValueError(f'CPU expectation disagrees with human labels: {mismatches}')
        import serial
        # Opening triggers the normal Uno DTR reset. Read its startup immediately;
        # do not sleep then discard input, which could erase the only banner.
        board = serial.Serial(args.port, args.baud, timeout=.1, write_timeout=args.timeout)
        reader = LineReader(board)
        wait_startup(reader, report['startup_log'])
        for repetition in range(args.repeat):
            for case in cases:
                row = dict(case, repetition=repetition + 1, timestamp=utc_now(), raw_lines=[],
                           transport_pass=False, parity_pass=False, semantic_pass=False)
                payload = case['text'].encode('utf-8') + (b'\r\n' if case['framing'] == 'CRLF' else b'\n')
                row['transmitted_bytes_hex'] = payload.hex()
                report['records'].append(row)
                try:
                    observed, elapsed = exchange(board, reader, payload, args.timeout, row['raw_lines'])
                    row.update(observed=observed, elapsed_ms=elapsed, transport_pass=True,
                               parity_pass=all(observed[k] == case['expected'][k] for k in observed),
                               semantic_pass=observed['effective_label'] == case['semantic_label'])
                except Exception as exc:
                    row.update(error=str(exc), error_type=type(exc).__name__)
                    raise  # Never send another request after lost framing.
                print(f"[{len(report['records'])}/{report['planned_requests']}] {case['id']}: {observed['effective_label']} reason={observed['reason']} {elapsed:.1f} ms", flush=True)
    except Exception as exc:
        report['fatal_error'] = dict(type=type(exc).__name__, message=str(exc), port=args.port,
                                    model_id=meta['model_id'])
    finally:
        if board is not None:
            board.close()
    report['finished_at'] = utc_now()
    report['summary'] = summarize(report['records'])
    report['passed'] = (not report.get('fatal_error') and
                        len(report['records']) == report['planned_requests'] and
                        all(r['transport_pass'] and r['parity_pass'] and r['semantic_pass'] for r in report['records']))
    atomic_report(args.output, report)
    print(json.dumps(dict(passed=report['passed'], report=str(args.output), **report['summary']), indent=2))
    if report.get('fatal_error'):
        print(json.dumps(report['fatal_error']), file=sys.stderr)
    return 0 if report['passed'] else 1


if __name__ == '__main__':
    raise SystemExit(main())
