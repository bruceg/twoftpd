Name: @PACKAGE@
Summary: Secure, simple, and efficient FTP server
Version: @VERSION@
Release: 1
Copyright: GPL
Group: Utilities/System
Source: http://em.ca/~bruceg/@PACKAGE@/@PACKAGE@-@VERSION@.tar.gz
BuildRoot: %{_tmppath}/@PACKAGE@-buildroot
URL: http://em.ca/~bruceg/@PACKAGE@/
Packager: Bruce Guenter <bruceg@em.ca>
BuildRequires: bglibs
Requires: supervise-scripts >= 3.2
Requires: cvm >= 0.10

%description
This is twoftpd, a new FTP server that strives to be secure, simple, and
efficient.

%prep
%setup

%build
echo gcc "${optflags} -I/usr/local/bglibs/include" >conf-cc
echo gcc "-s -L/usr/local/bglibs/lib" >conf-ld
echo %{_bindir} >conf-bin
echo %{_mandir} >conf-man
make programs

%install
echo %{buildroot}%{_bindir} >conf-bin
echo %{buildroot}%{_mandir} >conf-man
make installer

rm -fr %{buildroot}
mkdir -p %{buildroot}%{_bindir}
mkdir -p %{buildroot}%{_mandir}
./installer

mkdir -p %{buildroot}/etc/twoftpd
pushd %{buildroot}/etc/twoftpd
  echo 1 >CHROOT
  echo 900 >TIMEOUT
  echo 1 >LOGREQUESTS
  echo 1 >LOGRESPONSES
popd

mkdir -p %{buildroot}/var/log/twoftpd
mkdir -p %{buildroot}/var/service/twoftpd/log
install -m 755 twoftpd.run %{buildroot}/var/service/twoftpd/run
install -m 755 twoftpd-log.run %{buildroot}/var/service/twoftpd/log/run

%clean
rm -rf %{buildroot}

%post
if ! [ -e /service/twoftpd ]; then
  svc-add -d /var/service/twoftpd
fi

%files
%defattr(-,root,root)
%doc COPYING NEWS README
%dir /etc/twoftpd
%config(noreplace) /etc/twoftpd/*
%{_bindir}/*
%{_mandir}/*
%attr(0700,root,root) %dir /var/log/twoftpd
%attr(1755,root,root) %dir /var/service/twoftpd
%dir /var/service/twoftpd/log
%config(noreplace) /var/service/twoftpd/log/run
%config(noreplace) /var/service/twoftpd/run
