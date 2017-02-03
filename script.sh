#!/bin/bash

./waf configure --enable-examples --enable-tests --disable-gtk -d debug
./waf

export LD_LIBRARY_PATH="$(pwd)/build"
program="./build/examples/tcp/ns3-dev-tcp-variants-comparison-debug"

args=""
for tcp in TcpHighSpeed TcpHybla TcpVegas; do
  for error in 0.01 0.001 0.0001 0.00001 0.000001 0.0000001; do
    for sack in true false; do
      args="${args} \"--bandwidth=10Mbps --delay=45ms --access_delay=5ms --access_bandwidth=100Mbps --mtu=1500 --tracing=true --prefix_name=${tcp}-${sack}-${error} --transport_prot=${tcp} --sack=${sack} --error_p=${error}\""
    done
  done
done

eval "SHELL=$(type -p bash) parallel -v --colsep ' ' --halt now,fail=1 ${program} ::: ${args}"
