FROM docker.io/library/debian:stable-slim

RUN dpkg --add-architecture i386 \
	&& apt-get update -y \
	&& apt-get install -y --no-install-recommends \
		wine wine32 wine64 git wget unzip ca-certificates python3 python3-pip \
	&& rm -rf /var/lib/apt/lists/*

ENV WINEPREFIX=/wineprefix
ENV WINEDEBUG=-all

COPY set-env.reg /tmp/set-env.reg
RUN mkdir -p $WINEPREFIX && wineboot --init && wineserver -w \
	&& wine regedit /S /tmp/set-env.reg && wineserver -w

RUN mkdir -p $WINEPREFIX/drive_c/msvc6 \
	&& wget -q https://github.com/OmniBlade/decomp.me/releases/download/msvcwin9x/msvc6.4.tar.gz -O /tmp/msvc6.4.tar.gz \
	&& tar xzf /tmp/msvc6.4.tar.gz -C $WINEPREFIX/drive_c/msvc6 Bin Include ATL MFC \
	&& rm /tmp/msvc6.4.tar.gz

RUN git clone --depth 1 https://github.com/archaic-msvc/msvc600_sp4 /tmp/msvc600_sp4 \
	&& cp -r /tmp/msvc600_sp4/VC98/lib $WINEPREFIX/drive_c/msvc6/Lib \
	&& cp -r /tmp/msvc600_sp4/VC98/crt $WINEPREFIX/drive_c/msvc6/crt \
	&& rm -rf /tmp/msvc600_sp4

RUN wget -q https://github.com/Kitware/CMake/releases/download/v3.26.6/cmake-3.26.6-windows-i386.zip -O /tmp/cmake.zip \
	&& unzip -q /tmp/cmake.zip -d $WINEPREFIX/drive_c \
	&& mv $WINEPREFIX/drive_c/cmake-3.26.6-windows-i386 $WINEPREFIX/drive_c/cmake \
	&& rm /tmp/cmake.zip

RUN pip install --break-system-packages reccmp

COPY entrypoint.sh /entrypoint.sh
RUN chmod +x /entrypoint.sh
ENTRYPOINT [ "/entrypoint.sh" ]
