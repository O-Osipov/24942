#!/bin/bash

SOCKET="./socket"
SERVER="./server"
CLIENT="./client"

echo "=== Очистка старого сокета ==="
rm -f "$SOCKET"

echo "=== Запуск сервера ==="
$SERVER &
SERVER_PID=$!
sleep 1

echo "=== Запуск клиентов ==="

# Клиент 1 — поток X без перевода строки
( yes X | tr -d '\n' | head -c 500 | $CLIENT ) &

# Клиент 2 — поток Y без перевода строки
( yes Y | tr -d '\n' | head -c 500 | $CLIENT ) &

# Даём им поработать
sleep 2

echo
echo "=== Остановка ==="

kill $SERVER_PID 2>/dev/null
wait

echo "=== Готово ==="
