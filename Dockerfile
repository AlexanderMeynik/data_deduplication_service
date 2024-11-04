FROM ubuntu:latest
LABEL authors="Lenovo"

RUN apt-get -qq update; \
    apt-get install -qqy --no-install-recommends \
        gnupg2 wget ca-certificates apt-transport-https \
        autoconf automake cmake dpkg-dev file make patch libc6-dev


RUN echo "deb https://apt.llvm.org/noble llvm-toolchain-noble-19 main" \
        > /etc/apt/sources.list.d/llvm.list && \
    wget -qO /etc/apt/trusted.gpg.d/llvm.asc  \
        https://apt.llvm.org/llvm-snapshot.gpg.key && \
    apt-get -qq update && \
    apt-get install -qqy -t llvm-toolchain-noble-19 clang-19 \
    clang-tidy-19 clang-format-19 lld-19 libc++-19-dev libc++abi-19-dev &&  \
    for f in /usr/lib/llvm-*/bin/*; do ln -sf "$f" /usr/bin; done && \
       ln -sf clang /usr/bin/cc && \
       ln -sf clang /usr/bin/c89 && \
       ln -sf clang /usr/bin/c99 &&  \
       ln -sf clang++ /usr/bin/c++ && \
       ln -sf clang++ /usr/bin/g++ && \
    rm -rf /var/lib/apt/lists/* && \
    whereis clang


RUN apt-get update && \
    apt-get install -y ninja-build git \
    libssl-dev libpq-dev cmake doxygen \
    qt6-base-dev libqt6charts6-dev zip && \
    cd /home && \
    mkdir deps && \
    chmod -R 777 deps && \
    cd deps


RUN git clone https://github.com/google/glog.git&& \
    cd glog && \
    mkdir build && \
    cd build && \
    cmake -G Ninja  \
      -DCMAKE_C_COMPILER:PATH="/usr/bin/clang"  \
      -DCMAKE_CXX_COMPILER:PATH="/usr/bin/clang++" .. && \
    cmake --build . && \
    ninja install

RUN cd /home/deps && \
    git clone https://github.com/jtv/libpqxx.git && \
    cd libpqxx && \
    mkdir build&&cd build && \
    cmake -G Ninja  \
      -DCMAKE_C_COMPILER:PATH="/usr/bin/clang"  \
      -DCMAKE_CXX_COMPILER:PATH="/usr/bin/clang++" .. && \
    cmake --build . && \
    ninja install # buildkit



RUN cd /home/deps && \
    git clone https://github.com/google/googletest && \
    cd googletest && \
    mkdir build&&cd build && \
    cmake -G Ninja \
      -DCMAKE_C_COMPILER:PATH="/usr/bin/clang" \
      -DCMAKE_CXX_COMPILER:PATH="/usr/bin/clang++" .. && \
    cmake --build . && \
    ninja install


RUN cd /home/deps && \
    git clone https://github.com/hosseinmoein/Leopard && \
    cd Leopard && \
    mkdir build && cd build && \
    cmake -G Ninja \
      -DCMAKE_C_COMPILER:PATH="/usr/bin/clang" \
      -DCMAKE_CXX_COMPILER:PATH="/usr/bin/clang++" .. || true && \
    cmake --install .

RUN wget  --no-check-certificate \
     https://github.com/openssl/openssl/releases/download/openssl-3.4.0/openssl-3.4.0.tar.gz && \
    tar -xf openssl-3.4.0.tar.gz openssl-3.4.0/ && \
    cd openssl-3.4.0 && \
    ./Configure enable-md2 --prefix=/opt/openssl-md2 && \
    make -j$(nproc) && \
    make install

WORKDIR /usr/application/src
