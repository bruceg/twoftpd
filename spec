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

%description
This is twoftpd, a new FTP server that strives to be secure, simple, and
efficient.

%prep
%setup

%build
echo %{_bindir} >conf-bin
echo %{_mandir} >conf-man
make programs

%install
rm -fr $RPM_BUILD_ROOT
echo $RPM_BUILD_ROOT%{_bindir} >conf-bin
echo $RPM_BUILD_ROOT%{_mandir} >conf-man
make installer
./installer

mkdir -p $RPM_BUILD_ROOT/etc/twoftpd
pushd $RPM_BUILD_ROOT/etc/twoftpd
  echo ftp >ANONYMOUS
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
