Name:           liblouis
Version:        1.6.2
Release:        1%{?dist}
Summary:        A Braille translator and back-translator.

Group:          System Environment/Libraries
License:        LGPL3+
URL:            http://code.google.com/p/liblouis/
Source0:        %{name}-%{version}.tar.gz
BuildRoot:      %{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)

# BuildRequires:  
Requires:       libxml2

%description
Liblouis is an open-source braille translator and back-translator
based on the translation routines in the BRLTTY screenreader for
Linux. It has, however, gonefar beyond these routines. It is named in
honor of Louis Braille. In Linux and Mac OSX it is a shared library,
and in Windows it is a DLL.

%package        devel
Summary:        Development files for %{name}
Group:          Development/Libraries
Requires:       %{name} = %{version}-%{release}

%description    devel
The %{name}-devel package contains libraries and header files for
developing applications that use %{name}.


%prep
%setup -q


%build
#./autogen.sh
%configure --disable-static
#make %{?_smp_mflags}
make


%install
rm -rf $RPM_BUILD_ROOT
make install DESTDIR=$RPM_BUILD_ROOT
#find $RPM_BUILD_ROOT -name '*.la' -exec rm -f {} ';'


%clean
rm -rf $RPM_BUILD_ROOT


%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig


%files
%defattr(-,root,root,-)
%doc README
/usr/share/doc/liblouis/*
%{_infodir}/liblouis.info.gz
%{_libdir}/*
%{_bindir}/*
%dir /usr/share/liblouis
/usr/share/liblouis/*
#%dir %{_sysconfdir}/liblouis
#%dir %{_sysconfdir}/liblouis/tables
#%{_sysconfdir}/liblouis/tables/*
%files devel
%defattr(-,root,root,-)
%{_includedir}/*

%changelog
* Sat Sep 27 2008 - lars.bjorndal@broadpark.no
- Adjusted according to new package layout.
* Thu Feb 28 2008 - lars.bjorndal@broadpark.no
- Created this spec file.
