#!/bin/bash
set -e

# --- Configuration ---
CONFIG_FILE="../config/int_config"
PORT=80
# echo test
ECHO_EXPECT="GET /"
TMP_OUT="tmp_echo_body.txt"
# static test
MOUNT_PATH="/static/index.txt"
FILE_PATH="../tests/app/index.txt"
TMP_BODY="tmp_static_body.bin"
TMP_HDR="tmp_static_headers.txt"
# CRUD handler test
BASE_URL="http://localhost:$PORT/api/Products"

# Start server
bin/webserver "$CONFIG_FILE" &
SERVER_PID=$!

# wait up to 5 s
for i in {1..10}; do
  if curl -s "http://localhost:$PORT/echo" >/dev/null; then
    break
  fi
  sleep 0.5
  if [ $i -eq 10 ]; then
    echo "ERROR: Server did not start"
    kill $SERVER_PID 2>/dev/null || true
    exit 1
  fi
done

EXIT_CODE=0

# ——— echo test ———
curl -sS "http://localhost:$PORT/echo" -o "$TMP_OUT"
if grep -q "$ECHO_EXPECT" "$TMP_OUT"; then
  echo "ECHO: ✓ saw '$ECHO_EXPECT'"
else
  echo "ECHO: ✗ did not see '$ECHO_EXPECT'"
  EXIT_CODE=1
fi

# ——— static test ———
curl -sS -D "$TMP_HDR" "http://localhost:$PORT$MOUNT_PATH" -o "$TMP_BODY"

# status
HTTP_STATUS=$(head -n1 "$TMP_HDR" | awk '{print $2}')
if [ "$HTTP_STATUS" -ne 200 ]; then
  echo "STATIC: ✗ expected 200, got $HTTP_STATUS"
  EXIT_CODE=1
else
  echo "STATIC: ✓ 200 OK"
fi

# optional: check content-type
CT=$(grep -i '^Content-Type:' "$TMP_HDR" | awk '{print $2}')
echo "STATIC: Content-Type = $CT"

# md5
BODY_HASH=$(md5sum "$TMP_BODY" | cut -d' ' -f1)
FILE_HASH=$(md5sum "$FILE_PATH" | cut -d' ' -f1)
if [ "$BODY_HASH" = "$FILE_HASH" ]; then
  echo "STATIC: ✓ MD5($MOUNT_PATH) == $BODY_HASH"
else
  echo "STATIC: ✗ MD5 mismatch!"
  echo "  body: $BODY_HASH"
  echo "  file: $FILE_HASH"
  EXIT_CODE=1
fi

# --- POST ---
POST_PAYLOAD='{"name":"Mouse","price":25.99}'
POST_RES=$(curl -sS -X POST -H "Content-Type: application/json" -d "$POST_PAYLOAD" "$BASE_URL")
POST_STATUS=$?
ID=$(echo "$POST_RES" | grep -oP '(?<="id": ")[^"]+') # extract ID from response JSON

if [ $POST_STATUS -ne 0 ] || [ -z "$ID" ]; then # checks if curl command failed (by status code) or if the ID is empty
  echo "POST: failed to create entity"
  EXIT_CODE=1
else
  echo "POST: created entity with ID: $ID"
fi

# --- GET ---
GET_RES=$(curl -sS "$BASE_URL/$ID")
if echo "$GET_RES" | grep -q '"name":"Mouse"'; then
  echo "GET: retrieved correct entity"
else
  echo "GET: incorrect entity data: $GET_RES"
  EXIT_CODE=1
fi

# --- PUT ---
UPDATED_PAYLOAD='{"name":"Mouse","price":19.99,"updated":true}'
PUT_RES=$(curl -sS -X PUT -H "Content-Type: application/json" -d "$UPDATED_PAYLOAD" "$BASE_URL/$ID")
if echo "$PUT_RES" | grep -q "\"id\": \"$ID\""; then
  echo "PUT: entity updated"
else
  echo "PUT: entity update failed"
  EXIT_CODE=1
fi

# --- GET after PUT ---
GET_UPDATED=$(curl -sS "$BASE_URL/$ID")
if echo "$GET_UPDATED" | grep -q '"updated":true'; then
  echo "GET after PUT: updated data is correct"
else
  echo "GET after PUT: updated data missing: $GET_UPDATED"
  EXIT_CODE=1
fi

# --- DELETE ---
DELETE_RES=$(curl -sS -X DELETE "$BASE_URL/$ID")
if echo "$DELETE_RES" | grep -q "\"deleted\": true"; then
  echo "DELETE: entity deleted"
else
  echo "DELETE: entity delete failed"
  EXIT_CODE=1
fi

# --- GET after DELETE ---
GET_DELETED=$(curl -sS -w "%{http_code}" -o /dev/null "$BASE_URL/$ID")
if [ "$GET_DELETED" -eq 404 ]; then
  echo "GET after DELETE: ✓ entity is gone"
else
  echo "GET after DELETE: ✗ expected 404, got $GET_DELETED"
  EXIT_CODE=1
fi

# --- MALFORMED REQUEST TEST ---
MALFORMED_RES=$(printf "this is not http\r\n\r\n" | nc localhost $PORT)
if echo "$MALFORMED_RES" | grep -q "400 Bad Request"; then
  echo "MALFORMED: 400 Bad Request detected"
else
  echo "MALFORMED: Expected 400 Bad Request, got:"
  echo "$MALFORMED_RES"
  EXIT_CODE=1
fi


# cleanup
kill $SERVER_PID 2>/dev/null || true
wait $SERVER_PID 2>/dev/null || true
rm -f "$TMP_OUT" "$TMP_BODY" "$TMP_HDR"

exit $EXIT_CODE
