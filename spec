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
make CFLAGS="$RPM_OPT_FLAGS" bindir=%{_bindir} all

%install
rm -fr $RPM_BUILD_ROOT
make install_prefix=$RPM_BUILD_ROOT bindir=%{_bindir} mandir=%{_mandir} install

mkdir -p $RPM_BUILD_ROOT/etc/twoftpd
echo 900 >$RPM_BUILD_ROOT/etc/twoftpd/TIMEOUT

mkdir -p $RPM_BUILD_ROOT/var/log/twoftpd
mkdir -p $RPM_BUILD_ROOT/var/service/twoftpd/log
install -m 755 twoftpd.run $RPM_BUILD_ROOT/var/service/twoftpd/run
install -m 755 twoftpd-log.run $RPM_BUILD_ROOT/var/service/twoftpd/log/run

%clean
rm -rf $RPM_BUILD_ROOT

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
