"""Native C software tests; no firmware execution and no physical peripherals."""
import argparse
import os
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--cc', default=os.environ.get('CC', 'cc'))
parser.add_argument('--zig', action='store_true', help='use zig cc')
parser.add_argument('--wasm', action='store_true', help='compile wasm32-wasi and execute with Node.js')
parser.add_argument('--node', default='node')
parser.add_argument('--wasi-sysroot', type=Path, help='optional official prebuilt WASI sysroot to avoid first libc build')
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
out = root / 'build' / ('host-wasi' if args.wasm else 'host')
out.mkdir(parents=True, exist_ok=True)
env = dict(os.environ) if args.wasm else dict(os.environ, ZIG_GLOBAL_CACHE_DIR=str(out / 'zig-global'), ZIG_LOCAL_CACHE_DIR=str(out / 'zig-local'))
compiler = [args.cc] + (['cc'] if args.zig else [])
tests = [
    ('protocol', ['tests/test_protocol.c', 'protocol/atlab_can_protocol.c'], []),
    ('session', ['tests/test_session.c', 'protocol/atlab_session.c', 'protocol/atlab_can_protocol.c'], []),
    ('logging', ['tests/test_logging.c', 'firmware/atlab_debug_log.c'], []),
    ('usb_transport', ['tests/test_transport.c', 'firmware/atlab_log_transport.c'], ['-DSTM32F407xx']),
    ('uart_transport', ['tests/test_transport.c', 'firmware/atlab_log_transport.c'], ['-DSTM32L476xx']),
]
for name, sources, flags in tests:
    binary = out / (name + ('.wasm' if args.wasm else ('.exe' if os.name == 'nt' else '')))
    command = compiler + ['-std=c11', '-Wall', '-Wextra', '-Werror', '-pedantic', '-Iprotocol', '-Ifirmware', '-Itests/mocks']
    if args.wasm:
        command += ['-target', 'wasm32-wasi']
    link_inputs = []
    if args.wasi_sysroot:
        if not args.wasm:
            parser.error('--wasi-sysroot requires --wasm')
        sysroot = args.wasi_sysroot.resolve()
        # Explicit archive avoids Zig automatically rebuilding libc for -lc.
        # Keep UBSan checks as traps, without depending on a sanitizer runtime.
        command += ['-nostdlib', '-fsanitize-trap=all', '-isystem', str(sysroot / 'include/wasm32-wasip1')]
        link_inputs = [str(sysroot / 'lib/wasm32-wasip1/crt1-command.o'), str(sysroot / 'lib/wasm32-wasip1/libc.a')]
    print('Compiling ' + name + (' (WASI)' if args.wasm else ' (native)'), flush=True)
    subprocess.run(command + flags + sources + link_inputs + ['-o', str(binary)], cwd=root, env=env, check=True)
    launch = [args.node, str(root / 'tests/run_wasi.cjs'), str(binary)] if args.wasm else [str(binary)]
    subprocess.run(launch, cwd=root, env=env, check=True)
print(f'PASS: {len(tests)} C test executables; {"WASI" if args.wasm else "native"}; software-only')
