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

%description
This is twoftpd, a new FTP server that strives to be secure, simple, and
efficient.

%prep
%setup

%build
make CFLAGS="$RPM_OPT_FLAGS" all

%install
rm -fr $RPM_BUILD_ROOT
make install_prefix=$RPM_BUILD_ROOT bindir=%{_bindir} mandir=%{_mandir} install

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc COPYING NEWS README
%{_bindir}/*
