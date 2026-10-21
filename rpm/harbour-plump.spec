Name:       harbour-plump

Summary:    Plump (Oh Hell) against the bots
Version:    0.1.0
Release:    1
License:    GPLv3
URL:        https://github.com/klahr/plump
Source0:    %{name}-%{version}.tar.bz2
Requires:   sailfishsilica-qt5 >= 0.10.9
BuildRequires:  pkgconfig(sailfishapp) >= 1.0.2
BuildRequires:  pkgconfig(Qt5Core)
BuildRequires:  pkgconfig(Qt5Qml)
BuildRequires:  pkgconfig(Qt5Quick)
BuildRequires:  desktop-file-utils

%description
Plump, the Swedish Oh Hell, for three to five players against the bots
in three levels or online: bid exactly the tricks you will take, over
nineteen deals from ten cards down to one and back up, with trumps
turned up.

%prep
%setup -q -n %{name}-%{version}

%build

%qmake5 

%make_build

%check
make check

%install
%qmake5_install


desktop-file-install --delete-original         --dir %{buildroot}%{_datadir}/applications                %{buildroot}%{_datadir}/applications/*.desktop

%files
%defattr(-,root,root,-)
%{_bindir}/%{name}
%{_datadir}/%{name}
%{_datadir}/applications/%{name}.desktop
%{_datadir}/icons/hicolor/*/apps/%{name}.png
