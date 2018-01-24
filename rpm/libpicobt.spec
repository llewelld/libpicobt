Name:           libpicobt
Version:        0.0.1
Release:        1%{?dist}
Summary:        Platform-neutral Bluetooth API

License:        Pico Copyright 2017
URL:            https://mypico.org
Source0:        https://gitlab.dtg.cl.cam.ac.uk/pico/%{name}-%{version}-Source.tar.gz

BuildRequires:  gcc
BuildRequires:  cmake
BuildRequires:  bluez5-libs-devel
Requires:       bluez5-libs

%description
Platform-neutral Bluetooth API for use with Pico

%package        devel
Summary:        Development files for %{name}
Requires:       %{name}%{?_isa} = %{version}-%{release}

%description    devel
The %{name}-devel package contains libraries and header files for
developing applications that use %{name}.


%prep
%setup -q

%build
%cmake . 
make %{?_smp_mflags}


%install
make install DESTDIR=%{buildroot}
find $RPM_BUILD_ROOT -name '*.la' -exec rm -f {} ';'


%check
ctest -V %{?_smp_mflags}

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
#%license add-license-file-here
%{_libdir}/*.so.*
#/usr/lib/*.so.*
%{_libdir}/*.so

%files devel
#%doc add-devel-docs-here
%{_includedir}/*
%{_libdir}/*.so
%{_libdir}/*.so.*
%{_libdir}/**.so
%{_libdir}/**.a
%{_libdir}/pkgconfig/*.pc


%changelog
* Mon Jul 11 2017 David Llewellyn-Jones - 0.0.1-1
- Added spec file for building as an RPM
- Support for use on Mer and Fedora
- Support reverse port bind and service registration
- Support connecting to a port directly
- Move to Visual Studio 2015
- Improved documentation

* Mon Feb  6 2017 David Llewellyn-Jones - 0.0.0-1
- Initial working platform-independent version.
- Improved compile-time dependency checking
- Improved MinGW build script
- Greater unit test coverage and improved testing programs
- Logging now uses system logs
- Ability to send simultaneously to multiple devices

