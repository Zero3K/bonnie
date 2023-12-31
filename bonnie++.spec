Summary: A program for benchmarking hard drives and filesystems
Name: bonnie++
Version: 1.94
Release: 1
Copyright: GPL
Group: Utilities/Benchmarking
URL: http://www.coker.com.au/bonnie++
Source: http://www.coker.com.au/bonnie++/experimental/bonnie++-%{version}.tgz 
BuildRoot: /tmp/%{name}-buildroot
Prefixes: %{_prefix} %{_datadir}
Requires: glibc >= 2.1
Requires: perl >= 5.0
BuildRequires: glibc-devel >= 2.1

%description
Bonnie++ is a benchmark suite that is aimed at performing a number of simple
tests of hard drive and file system performance.

%prep
%setup -q

%build
./configure --prefix=$RPM_BUILD_ROOT%{_prefix} --mandir=3D$RPM_BUILD_ROOT%{_mandir}
make

%install
rm -rf $RPM_BUILD_ROOT
make install

%clean
rm -rf $RPM_BUILD_ROOT

%files
%defattr(-,root,root)
%doc changelog.txt readme.html

%{_prefix}/sbin/bonnie++
%{_prefix}/sbin/zcav
%{_prefix}/bin/bon_csv2html
%{_prefix}/bin/bon_csv2txt
%{_mandir}/man1/bon_csv2html.1
%{_mandir}/man1/bon_csv2txt.1
%{_mandir}/man8/bonnie++.8
%{_mandir}/man8/zcav.8

%changelog
* Wed Sep 06 2000 Rob Latham <rlatham@plogic.com>
- first packaging
