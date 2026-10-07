# SPDX-License-Identifier: MPL-2.0
#
# Harbour package. Every Requires must be in ci/harbour/allowed_requires.conf (or a library
# from allowed_libraries.conf); see docs/HARBOUR.md. OS floor (>= 5.2.0) can't be
# `Requires: sailfish-version` (Harbour rejects it); carried by SDK target
# (__libc_start_main check) and package versions below.
#
# Version = release version (docs/RELEASING.md); CI refuses mismatching tag. Release stays 1;
# CI stamps 1.<run number> on non-release builds.
Name:       harbour-salama
Summary:    Web browser
Version:    0.8.0
Release:    1
License:    MPL-2.0
URL:        https://github.com/muhnschein/salama
Source0:    %{name}-%{version}.tar.bz2

BuildRequires:  cmake
BuildRequires:  desktop-file-utils
BuildRequires:  pkgconfig(sailfishapp) >= 1.0.2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5DBus)
BuildRequires:  pkgconfig(Qt5Gui)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  pkgconfig(Qt5Sql)
BuildRequires:  qt5-qttools-linguist

Requires:   sailfishsilica-qt5 >= 1.1.123
Requires:   sailfish-components-webview-qt5 >= 1.7.0
Requires:   sailfish-components-webview-qt5-popups >= 1.7.0
Requires:   sailfish-components-webview-qt5-pickers >= 1.7.0
Requires:   qt5-plugin-imageformat-ico
# QtGraphicalEffects: rounded tab preview corners.
Requires:   qt5-qtgraphicaleffects
# Nemo.Notifications: page notifications.
Requires:   nemo-qml-plugin-notifications-qt5

%description
Web browser for Sailfish OS with a Silica interface over the platform web engine.

%prep
%setup -q -n %{name}-%{version}

%build
mkdir -p build
cd build
cmake \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=%{_prefix} \
    -DSALAMA_BUILD_TESTS=OFF \
    -DSALAMA_REQUIRE_SAILFISHAPP=ON \
    -DSALAMA_VERSION=%{version} \
    ..
make %{?_smp_mflags}

%install
cd build
make DESTDIR=%{buildroot} install/strip
desktop-file-install --delete-original \
    --dir %{buildroot}%{_datadir}/applications \
    %{buildroot}%{_datadir}/applications/%{name}.desktop

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
