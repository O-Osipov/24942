#!/usr/bin/env bash
set -e

SOCK="./socket"

echo "=== Очистка старого сокета ==="
rm -f "$SOCK"

echo "=== Запуск сервера ==="
./server > server_out.txt 2>&1 &
SPID=$!

# дать серверу подняться и создать/привязать сокет
sleep 0.2

echo "=== Запуск 2 spam-клиентов (мелкие порции) ==="
# важное: маленькие порции + небольшие задержки -> больше шансов на перемешивание
./client_spam x 3000 500 &
C1=$!
./client_spam y 3000 500 &
C2=$!

wait $C1 $C2

echo "=== Остановка сервера ==="
kill $SPID 2>/dev/null || true
wait $SPID 2>/dev/null || true

echo "=== Фрагмент вывода сервера (первые 200 символов) ==="
# убираем переводы строк, чтобы увидеть "мешанину" в одной строке
tr -d '\n' < server_out.txt | head -c 200
echo
echo "=== Готово: полный вывод в server_out.txt ==="
