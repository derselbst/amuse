FROM ghcr.io/derselbst/qt6.suse:master

RUN zypper addrepo https://download.opensuse.org/repositories/home:derselbst:anmp/15.6/home:derselbst:anmp.repo
RUN zypper refresh
RUN zypper install -y qt6-linguist-devel alsa-devel libpulse-devel lzo-devel systemd-devel qt6-qml-devel qt6-svg-devel qt6-network-devel qt6-xml-devel libfluidsynth-devel

