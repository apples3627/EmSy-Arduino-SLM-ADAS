"""Pure fixtures: does not import pyserial or access a device."""
import unittest
import test_hardware as hw


class FakeSerial:
    def __init__(self, reply):
        self.reply = bytearray(reply)
        self.writes = []

    def read(self, count):
        if self.reply:
            byte = bytes(self.reply[:count])
            del self.reply[:count]
            return byte
        return b''

    def write(self, payload):
        self.writes.append(payload)
        return len(payload)


class HardwareFixtures(unittest.TestCase):
    def test_full_reply(self):
        self.assertEqual(hw.parse_response(['RAW FORWARD EFFECTIVE UNKNOWN REASON 8']),
                         dict(raw_label='FORWARD', effective_label='UNKNOWN', reason=8,
                              response_kind='classification'))

    def test_errors(self):
        for line, reason in [('ERROR 1', 1), ('ERROR 7', 7),
                             ('ERROR too_long -> GLOBAL_STOP', 2)]:
            observed = hw.parse_response([line])
            self.assertEqual(observed['reason'], reason)
            self.assertEqual(observed['effective_label'], 'UNKNOWN')
            self.assertIsNone(observed['raw_label'])

    def test_malformed_partial_duplicate_and_empty(self):
        for lines in [[], ['PARTIAL 0 1 2'], ['RAW BAD EFFECTIVE STOP REASON 0'],
                      ['ERROR 99'], ['ERROR 1', 'ERROR 1'], ['']]:
            with self.assertRaises(ValueError):
                hw.parse_response(lines)

    def test_startup_and_original_bytes_then_recovery(self):
        board = FakeSerial((hw.BANNER + '\r\nERROR 4\r\nREADY\r\n'
                            'RAW FORWARD EFFECTIVE FORWARD REASON 0\r\nREADY\r\n').encode())
        reader = hw.LineReader(board)
        startup = []
        hw.wait_startup(reader, startup, .02)
        log = []
        first, _ = hw.exchange(board, reader, b'go, forward\n', .02, log)
        second, _ = hw.exchange(board, reader, b'go forward\r\n', .02, [])
        self.assertEqual(first['reason'], 4)
        self.assertEqual(second['effective_label'], 'FORWARD')
        self.assertEqual(board.writes, [b'go, forward\n', b'go forward\r\n'])
        self.assertEqual([entry['line'] for entry in log], ['ERROR 4', 'READY'])

    def test_missing_ready_times_out_without_fallback(self):
        board = FakeSerial(b'RAW FORWARD EFFECTIVE FORWARD REASON 0\r\n')
        log = []
        with self.assertRaises(TimeoutError):
            hw.exchange(board, hw.LineReader(board), b'go forward\n', .002, log)
        self.assertEqual(len(board.writes), 1)
        self.assertEqual(len(log), 1)

    def test_invalid_ascii_response(self):
        board = FakeSerial(b'\xff\nREADY\n')
        with self.assertRaises(UnicodeDecodeError):
            hw.exchange(board, hw.LineReader(board), b'go forward\n', .02, [])

    def test_embedded_lf_cannot_be_hidden_in_a_request(self):
        board = FakeSerial(b'')
        with self.assertRaises(ValueError):
            hw.exchange(board, hw.LineReader(board), b'go\nforward\n', .02, [])
        self.assertEqual(board.writes, [])

    def test_manual_case_expectations(self):
        model, meta = hw.reference.load_model()
        cases = hw.build_cases()
        self.assertGreaterEqual(len(cases), 60)
        self.assertEqual(set(c['semantic_label'] for c in cases), set(hw.reference.LABELS))
        for case in cases:
            with self.subTest(case=case['id']):
                self.assertEqual(hw.expected_for(case['text'], model, meta)['effective_label'],
                                 case['semantic_label'])


if __name__ == '__main__':
    unittest.main()
