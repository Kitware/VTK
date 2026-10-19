#!/usr/bin/env python3
import json
import sys

# VTK's own public C API (PyVTKObject_New, PyVTKMethodDescriptor_New, ...) is
# exported by the wheel's shared objects but is not part of CPython's stable
# ABI symbol table, so abi3audit flags it as a non-abi3 symbol. These are
# false positives, not real abi3 violations.
IGNORED_SYMBOL_PREFIX = 'PyVTK'


def main(report_path):
    with open(report_path) as f:
        report = json.load(f)

    violations = []
    for spec in report['specs'].values():
        for entry in spec['wheel']:
            so_name = entry['name']
            result = entry['result']
            for symbol in result['non_abi3_symbols']:
                if symbol.startswith(IGNORED_SYMBOL_PREFIX):
                    continue
                violations.append(f'{so_name}: {symbol} is not in the stable ABI')
            for symbol, added in result['future_abi3_objects'].items():
                violations.append(
                    f'{so_name}: {symbol} was added in {added}, after the abi3 floor')

    if violations:
        print('abi3 violations found:', file=sys.stderr)
        for violation in violations:
            print(f'  {violation}', file=sys.stderr)
        return 1

    print('No abi3 violations found (ignoring VTK\'s own PyVTK* symbols).')
    return 0


if __name__ == '__main__':
    if len(sys.argv) != 2:
        print(f'usage: {sys.argv[0]} <abi3audit-report.json>', file=sys.stderr)
        sys.exit(2)
    sys.exit(main(sys.argv[1]))
