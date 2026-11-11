#!/usr/bin/env bash
# Start the shadow watcher in the background.
#
# A sandbox restart stops this process; the run does not lose its place. The
# checkpoint and the journal live in agent/paper/, so restarting this script
# resumes exactly where it stopped, with no duplicate trades.
#
#   ./agent/run_watch.sh            # 12 hours, every 5 minutes
#   ./agent/run_watch.sh 24 300     # 24 hours, every 5 minutes
set -u
cd "$(dirname "$0")/.." || exit 1

HOURS="${1:-12}"
INTERVAL="${2:-300}"

mkdir -p agent/paper
if [ -f agent/paper/watch.pid ] && kill -0 "$(cat agent/paper/watch.pid)" 2>/dev/null; then
  echo "المراقب يعمل بالفعل (PID $(cat agent/paper/watch.pid))"
  exit 0
fi

nohup python3 agent/tools/paper_watch.py --loop --hours "$HOURS" --interval "$INTERVAL" \
  > agent/paper/loop.out 2>&1 &
echo $! > agent/paper/watch.pid
echo "بدأ المراقب: PID $(cat agent/paper/watch.pid) لمدة ${HOURS} ساعة"
