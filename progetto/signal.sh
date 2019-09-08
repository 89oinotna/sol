#!/bin/bash
#eseguo la signal sul server
BPID="$(pidof objstore_server)"
kill -SIGUSR1 $BPID 