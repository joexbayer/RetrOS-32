#!/usr/bin/env bash

# Configure rootfs/sysutil/startup.sh to run testing applications on boot
# Uncomment boot/bootloader.s:393 to skip resolution prompt

# How to test:
#   ./tools/run_qemu_stress.sh
# Read logs at /tmp/qemu-headless.log
# Change code
# sudo make clean if you changed a header file
# sudo make img
# Repeat test

set -euo pipefail

# Configurable knobs
PORT="${PORT:-8080}"
HOST="${HOST:-127.0.0.1}"
RUNS="${RUNS:-8}"
REQS="${REQS:-50}"
CONCURRENCY="${CONCURRENCY:-6}"
TIMEOUT="${TIMEOUT:-5}"
WAIT_ATTEMPTS="${WAIT_ATTEMPTS:-15}"
BOOT_DELAY="${BOOT_DELAY:-4}"
LOGFILE="${LOGFILE:-/tmp/qemu-headless.log}"
PIDFILE="${PIDFILE:-/tmp/qemu-headless.pid}"
STRESS_SCRIPT="./tools/stress_webserv.sh"

if [[ "${EUID}" -eq 0 ]]; then
	SUDO=""
else
	SUDO="sudo -n"
fi

run_env() {
	if [[ -z "${SUDO}" ]]; then
		env "$@"
	else
		${SUDO} env "$@"
	fi
}

make_pid=""
qemu_pid=""

cleanup() {
	set +e
	if [[ -n "${qemu_pid}" ]]; then
		if ${SUDO} kill -0 "${qemu_pid}" 2>/dev/null; then
			${SUDO} kill "${qemu_pid}" 2>/dev/null || true
		fi
	fi
	if [[ -n "${make_pid}" ]]; then
		if ${SUDO} kill -0 "${make_pid}" 2>/dev/null; then
			${SUDO} kill "${make_pid}" 2>/dev/null || true
		fi
	fi
	rm -f "${PIDFILE}"
}
trap cleanup EXIT


# Check if port is already in use and kill the process
existing_pid=$(lsof -ti tcp:${PORT} 2>/dev/null || true)
if [[ -n "${existing_pid}" ]]; then
	echo "Port ${PORT} is already in use by PID ${existing_pid}, killing it..."
	${SUDO} kill "${existing_pid}" 2>/dev/null || true
	sleep 1
	# Force kill if still alive
	if ${SUDO} kill -0 "${existing_pid}" 2>/dev/null; then
		${SUDO} kill -9 "${existing_pid}" 2>/dev/null || true
		sleep 1
	fi
fi
echo "Starting QEMU headless on host port ${PORT}..."
run_env HOSTFWD_PORT="${PORT}" nohup make qemu-headless >"${LOGFILE}" 2>&1 &
make_pid=$!

sleep "${BOOT_DELAY}"

# Capture the actual QEMU process ID
qemu_pid=$(pgrep -f qemu-system-i386 || true)
if [[ -z "${qemu_pid}" ]]; then
	echo "QEMU failed to start; see log: ${LOGFILE}"
	exit 1
fi
echo "${qemu_pid}" > "${PIDFILE}"

if grep -q "Could not set up host forwarding" "${LOGFILE}"; then
	echo "QEMU failed to bind hostfwd on port ${PORT}; see log: ${LOGFILE}"
	exit 1
fi

# Wait for the guest web server to become reachable
ready=0
for attempt in $(seq 1 "${WAIT_ATTEMPTS}"); do
	if run_env curl -s -o /dev/null --max-time 1 "http://${HOST}:${PORT}/" 2>/dev/null; then
		ready=1
		break
	fi
	sleep 1
done

if [[ "${ready}" -ne 1 ]]; then
	echo "Web server not reachable after ${WAIT_ATTEMPTS} attempts; see log: ${LOGFILE}"
	exit 1
fi

for i in $(seq 1 "${RUNS}"); do
	echo "Run ${i}/${RUNS}"
	run_env HOST="${HOST}" PORT="${PORT}" REQS="${REQS}" CONCURRENCY="${CONCURRENCY}" TIMEOUT="${TIMEOUT}" "${STRESS_SCRIPT}" "http://${HOST}:${PORT}/"
done

echo "All ${RUNS} runs completed successfully."
