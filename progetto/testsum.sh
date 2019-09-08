#!/bin/bash
filename='testout.log'
nt1=0
nt1I=1000
nt2=0
nt2I=30
nt3=0
nt3I=20
nclient=0
tipo=0
n=1
while read line; do
    if [ -n "$line" ];
    then
        if [ "$line" == "RESPONSE: OK" ]; then
                    nt1=$((nt1+1))
                fi
        if [ "$line" == "Test2 OK" ]; then
                    nt2=$((nt2+1))
                fi
        if [ "$line" == "Test3 OK" ]; then
                    nt3=$((nt3+1))
                fi
        #if [ "$line" == "-------Start Test 1-------" ] ;
        #then    tipo=1
        #fi
        #if [ "$line" == "-------Start Test 2-------" ] ;
        #then    tipo=2
        #fi
        #if [ "$line" == "-------Start Test 3-------" ] ;
        #then     tipo=3
        #fi
        #caso di test che sto calcolando
        #case "$tipo" in
        #    1)  if [ "$line" == "RESPONSE: OK" ]; then
        #            nt1=$((nt1+1))
        #        fi
        #    ;;
        #    2) if [ "$line" == "Test2 OK" ]; then
        #            nt2=$((nt2+1))
        #        fi
        #    ;;
        #    3) if [ "$line" == "Test3 OK" ]; then
        #            nt3=$((nt3+1))
        #        fi
        #   ;;
        #esac
        #conto il numero di client connessi
        if [ "$line" == "DISCONNECTED: OK" ] ;
        then nclient=$((nclient+1))
        fi
    fi
    n=$((n+1))
done < $filename
echo "Client connessi: $nclient"
echo " -------Esito test 1-------"
echo " Batterie superate: $nt1 batterie fallite: $((nt1I-nt1))"
echo " -------Esito test 2-------"
echo " Batterie test 2 superate: $nt2 batterie fallite: $((nt2I-nt2))"
echo " -------Esito test 3-------"
echo " Batterie test 3 superate: $nt3 batterie fallite: $((nt3I-nt3))"
if [ $nclient == 100 ] && [ $nt1 == 1000 ] && [ $nt2 == 30 ] && [ $nt3 == 20 ] ;
then
    echo "Test completato correttamente"
else
    echo "Test fallito"
fi
#eseguo la signal sul server
BPID="$(pidof osserver)"
kill -SIGUSR1 $BPID
#kill -SIGINT $BPID
