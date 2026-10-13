#!/usr/bin/env python3
"""PC UART terminal for SLM_UART; transport only, no model or inference."""
import argparse
import sys
import threading


def valid_line(text):
    return bool(text) and len(text) <= 192 and all(32 <= ord(c) <= 126 for c in text)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('port', nargs='?', help='SLM_UART USB port, e.g. /dev/cu.usbmodem11301 or COM3')
    parser.add_argument('--baud', type=int, default=9600)
    parser.add_argument('--list', action='store_true', help='List available serial ports without opening them')
    args = parser.parse_args()
    try:
        import serial
        from serial.tools import list_ports
    except ImportError:
        parser.exit(1, 'pyserial이 필요합니다: python3 -m pip install pyserial\n')
    if args.list:
        for port in list_ports.comports():
            print(f'{port.device}\t{port.description}')
        return 0
    if not args.port:
        parser.error('SLM_UART의 USB 포트를 지정하세요. --list로 확인할 수 있습니다.')
    stop = threading.Event()
    try:
        board = serial.Serial(args.port, args.baud, timeout=0.2, write_timeout=2)
    except (serial.SerialException, OSError) as exc:
        parser.exit(1, f'포트를 열 수 없습니다: {exc}\nArduino IDE 시리얼 모니터를 닫고 포트를 확인하세요.\n')

    def receive():
        try:
            while not stop.is_set():
                data = board.readline()
                if data:
                    print('\r' + data.decode('ascii', errors='replace').rstrip('\r\n'), flush=True)
        except (serial.SerialException, OSError) as exc:
            if not stop.is_set():
                print(f'\nUART 연결 오류: {exc}', file=sys.stderr, flush=True)
                stop.set()

    print(f'UART 연결: {args.port}, {args.baud} baud / Newline', flush=True)
    print('READY 후 일반 명령을 보내세요. WAIT 중에도 !status / stop 사용 가능.', flush=True)
    print('예: go forward / go backward / lights on / horn on / stop', flush=True)
    print('전진/후진은 계속 유지됩니다. stop으로 전체 정지합니다.', flush=True)
    print('다중 명령: go forward and lights on and horn on', flush=True)
    print('APPLIED/READY를 확인한 뒤 다음 명령. 종료: exit 또는 Ctrl+C', flush=True)
    reader = threading.Thread(target=receive, daemon=True)
    reader.start()
    try:
        while not stop.is_set():
            try:
                text = input('UART> ')
            except (EOFError, KeyboardInterrupt):
                print()
                break
            if text.strip().lower() in ('exit', 'quit'):
                break
            if not text:
                continue
            if not valid_line(text):
                print('영어 ASCII 한 줄, 최대192바이트를 입력하세요.')
                continue
            if stop.is_set():
                break
            board.write((text + '\n').encode('ascii'))
            board.flush()
    except (serial.SerialException, OSError) as exc:
        print(f'UART 송신 오류: {exc}', file=sys.stderr)
        return 1
    finally:
        stop.set()
        reader.join(timeout=0.5)
        board.close()
    return 0


if __name__ == '__main__':
    raise SystemExit(main())
