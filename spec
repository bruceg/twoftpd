Name: @PACKAGE@
Summary: Secure, simple, and efficient FTP server
Version: @VERSION@
Release: 1
Copyright: GPL
Group: Utilities/System
Source: http://em.ca/~bruceg/@PACKAGE@/@PACKAGE@-@VERSION@.tar.gz
BuildRoot: /tmp/@PACKAGE@-buildroot
URL: http://em.ca/~bruceg/@PACKAGE@/
Packager: Bruce Guenter <bruceg@em.ca>
Requires: supervise-scripts >= 3.2
Requires: cvm >= 0.3

%description
This is twoftpd, a new FTP server that strives to be secure, simple, and
efficient.

%prep
%setup

%build
echo gcc "$RPM_OPT_FLAGS" >conf-cc
echo gcc -s >conf-ld
echo %{_bindir} >conf-bin
echo %{_mandir} >conf-man
make programs

%install
echo $RPM_BUILD_ROOT%{_bindir} >conf-bin
echo $RPM_BUILD_ROOT%{_mandir} >conf-man
make installer

rm -fr $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT%{_bindir}
mkdir -p $RPM_BUILD_ROOT%{_mandir}
./installer

mkdir -p $RPM_BUILD_ROOT/etc/twoftpd
pushd $RPM_BUILD_ROOT/etc/twoftpd
  echo 1 >CHROOT
  echo 900 >TIMEOUT
popd

mkdir -p $RPM_BUILD_ROOT/var/log/twoftpd
mkdir -p $RPM_BUILD_ROOT/var/service/twoftpd/log
install -m 755 twoftpd.run $RPM_BUILD_ROOT/var/service/twoftpd/run
install -m 755 twoftpd-log.run $RPM_BUILD_ROOT/var/service/twoftpd/log/run

%clean
rm -rf $RPM_BUILD_ROOT

%post
if ! [ -e /service/twoftpd ]; then
  svc-add -d /var/service/twoftpd
fi

cd /etc/twoftpd
test -s ANON_UID || id -u ftp >ANON_UID
test -s ANON_GID || id -g ftp >ANON_GID
test -s ANON_HOME || echo /home/ftp >ANON_HOME

%files
%defattr(-,root,root)
%doc COPYING NEWS README
%dir /etc/twoftpd
%config(noreplace) /etc/twoftpd/*
%{_bindir}/*
%attr(0700,root,root) %dir /var/log/twoftpd
%attr(1755,root,root) %dir /var/service/twoftpd
%dir /var/service/twoftpd/log
%config(noreplace) /var/service/twoftpd/log/run
%config(noreplace) /var/service/twoftpd/run
