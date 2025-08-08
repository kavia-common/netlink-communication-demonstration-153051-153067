#!/bin/bash
cd /home/kavia/workspace/code-generation/netlink-communication-demonstration-153051-153067/netlink_client_server_frontend
npm run build
EXIT_CODE=$?
if [ $EXIT_CODE -ne 0 ]; then
   exit 1
fi

