xw=$src/xfer-watch
uid=`id -u`
gid=`id -g`
home=$tmp/home
mkdir -p $home
cvm=$tmp/cvm
TCPLOCALIP=127.0.0.1
TCPREMOTEIP=127.0.0.1
LOGREQUESTS=1
SOFTCHROOT=1
export LOGREQUESTS TCPLOCALIP TCPREMOTEIP SOFTCHROOT

env="env UID=$uid GID=$gid USER=ftp GROUP=ftp HOME=$home TZ=Canada/Saskatchewan"

# Support functions

strippid() { sed -e 's/\[[0123456789]*\]/[PID]/'; }

echolines() {
  for line in "$@"
  do
    printf "$line\r\n"
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

anon() { echolines "$@" | $env $src/twoftpd-anon 2>/dev/null | tail +2; }

xfer() { echolines "$@" | $env $src/twoftpd-xfer 2>/dev/null | tail +2; }

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
  $env $src/xfer-test $src/twoftpd-$backend $mode $io $conv "$@" | tail +2
}

