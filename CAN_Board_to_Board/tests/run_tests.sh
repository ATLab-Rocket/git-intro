#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
cc -std=c11 -Wall -Wextra -Werror -pedantic -Iprotocol \
  protocol/atlab_can_protocol.c tests/test_protocol.c -o /tmp/atlab_can_protocol_test
/tmp/atlab_can_protocol_test
