%define plugindir %(pkg-config audacious --variable=input_plugin_dir)

Summary: Future Composer input plugin for Audacious
Name: audacious-plugin-fc
Version: 0.4
Release: 1
URL: http://xmms-fc.sourceforge.net/
License: GPL
Source:	http://download.sourceforge.net/xmms-fc/audacious-plugin-fc-0.3.tar.bz2
Group: Applications/Multimedia
Buildroot: %{_tmppath}/%{name}-%{version}-%{release}-buildroot-%(%{__id_u} -n)
BuildRequires: audacious-devel >= 2.2
BuildRequires: pkgconfig
Requires: audacious >= 2.2

%description
This is an input plugin for Audacious which can play back Future Composer
music files from AMIGA. Song-length detection and seek are implemented, too.


%prep
%setup -q


%build
%configure
make %{?_smp_mflags}


%install
rm -rf $RPM_BUILD_ROOT
mkdir -p $RPM_BUILD_ROOT
make DESTDIR=$RPM_BUILD_ROOT install


%clean
rm -rf $RPM_BUILD_ROOT


%files
%defattr(-,root,root,-)
%doc COPYING README src/FC.README
%{plugindir}/libfc.so
%exclude %{plugindir}/libfc.la
