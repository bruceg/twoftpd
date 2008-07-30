xw=$src/xfer-watch
uid=`id -u`
gid=`id -g`
home=$tmp/home
mkdir -p $home
cvm=$tmp/cvm
TCPLOCALIP=127.0.0.1
TCPLOCALHOST=localhost
TCPREMOTEIP=127.0.0.1
LOGREQUESTS=1
CVM_RANDOM_BYTES=0
TIMEOUT=5
export CVM_RANDOM_BYTES LOGREQUESTS TCPLOCALIP TCPLOCALHOST TCPREMOTEIP TIMEOUT

env="env UID=$uid GID=$gid USER=ftp GROUP=ftp HOME=$home TZ=Canada/Saskatchewan"

# Support functions

strippid() { sed -e 's/\[[0123456789]*\]/[PID]/'; }

echolines() {
  for line in "$@"
  do
    printf "$line\r\n" 2>/dev/null
  done
}

authcmd() { $src/twoftpd-auth "$@" 2>&1 </dev/null | strippid; }

auth() {
  local cvm=$1
  local backend=$2
  shift 2
  echolines "$@" | $src/twoftpd-auth $cvm $backend 2>&1 | strippid
}

makecvm() {
  (
    echo '#!/bin/sh'
    for line in "$@"; do echo "$line"; done
  ) >$cvm
  chmod 755 $cvm
}

fixdir() { sed -e 's/^\(d.*\)    [123456789][0123456789] /\1  4096 /'; }
fixbytes() { sed -e 's/[0123456789]* bytes /### bytes /'; }

anon() { echolines "$@" | $env $src/twoftpd-anon 2>/dev/null | fixdir | tail -n +2; }
drop() { echolines "$@" | $env $src/twoftpd-drop 2>/dev/null | fixdir | tail -n +2; }
xfer() { echolines "$@" | $env $src/twoftpd-xfer 2>/dev/null | fixdir | tail -n +2; }

testparams() {
  local backend=$1
  local command=$2
  shift 2
  local param
  for param in "$@"; do
    $backend "$command $param"
  done
}

xfertest() {
  local backend=$1
  local mode=$2
  local io=$3
  local conv=$4
  shift 4
  $env $src/xfer-test $src/twoftpd-$backend $mode $io $conv "$@" | fixdir | tail -n +2
}

