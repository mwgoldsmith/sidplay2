%define name    libsidplay
%define version 2.1.0
%define release 1
%define major   2
%define residversion 0.11

Summary:        A Commodore 64 music player and SID chip emulator library.
Name:           %{name}
Version:        %{version}
Release:        %{release}
Source:         %{name}-%{version}.tar.bz2
Source1:        resid-%{residversion}.tar.bz2
Patch:          resid-%{residversion}-mute1.patch.bz2
Copyright:      GPL
Group:          System/Libraries
URL:            http://sidplay2.sourceforge.net/
BuildRoot:      %{_tmppath}/%{name}%{major}-buildroot
Prefix:         %{_prefix}

%description
This is a cycle-based version of a C64 music playing library
developed by Simon White which requires the ReSID library as
the SID emulator portion.

A modified version of ReSID %{residversion} is included in this package.

%package devel
Summary:        Development headers and libraries for %{name}%{major}
Group:          Development/C++

%description devel
This package includes the header and library files necessary
for developing applications to use %{name}%{major}.


%prep
rm -rf $RPM_BUILD_ROOT 
%setup -q -a 1
%patch -p0

%build
cd resid-%{residversion}
CXXFLAGS="$RPM_OPT_FLAGS" ./configure --disable-shared
make
cd ..

CXXFLAGS="$RPM_OPT_FLAGS" ./configure --prefix=%{_prefix} --with-resid=$PWD/resid-%{residversion}
make

%install
make DESTDIR=$RPM_BUILD_ROOT install

%clean
rm -rf $RPM_BUILD_ROOT

%postun
/sbin/ldconfig

%post
/sbin/ldconfig

%files
%defattr(-,root,root)
%doc AUTHORS COPYING ChangeLog README TODO
%{_libdir}/*.so.*

%files devel
%defattr(-,root,root)
%{_includedir}/sidplay/*
%{_libdir}/*.la
%{_libdir}/*.a
%{_libdir}/*.so

%changelog
* Wed Nov 7 2001 Simon White <s_a_white@email.com> 2.0.7-5
- Performance fix.

* Mon May 7 2001 Simon White <s_a_white@email.com> 2.0.7-4
- Fix for endian functions under gcc 2.96.

* Wed Apr 10 2001 Simon White <s_a_white@email.com> 2.0.7-3
- Use non Mandrake specific release number.

* Wed Apr 4 2001 Simon White <s_a_white@email.com> 2.0.7-2mdk
- Updated --prefix and make install so la file does not end up with
  a bad install path.

* Sun Apr 1 2001 Simon White <s_a_white@email.com> 2.0.7-1mdk
- First spec file.

# end of file
