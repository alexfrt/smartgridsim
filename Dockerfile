FROM ubuntu:18.04 as intermediate

# Download and install dependencies
RUN apt update && apt install -y \
        wget gcc g++ python python-pip \
        gir1.2-goocanvas-2.0 python-gi python-gi-cairo python-pygraphviz python3-gi \
        python3-gi-cairo python3-pygraphviz gir1.2-gtk-3.0 ipython ipython3 \
        qt5-default openmpi-bin openmpi-common openmpi-doc libopenmpi-dev autoconf \
        cvs bzr unrar gdb valgrind uncrustify gsl-bin libgsl-dev flex bison \
        libfl-dev tcpdump sqlite sqlite3 libsqlite3-dev libxml2 libxml2-dev cmake \
        libc6-dev libc6-dev-i386 libclang-dev llvm-dev automake libgtk2.0-0 \
        libgtk2.0-dev vtun lxc libboost-signals-dev libboost-filesystem-dev python-tk \
     && rm -rf /var/lib/apt/lists/*
RUN pip install cxxfilt scipy matplotlib xmltodict

# Create the base directory
RUN mkdir workspace
WORKDIR /workspace

# Download and build NS-3
RUN wget http://www.nsnam.org/release/ns-allinone-3.28.tar.bz2
RUN tar xjf ns-allinone-3.28.tar.bz2
WORKDIR /workspace/ns-allinone-3.28
RUN ./build.py --enable-examples --enable-tests

# Make the NS-3 directory the workdir
WORKDIR /workspace/ns-allinone-3.28/ns-3.28/

# Enable the optimized mode
RUN ./waf configure --build-profile=optimized
RUN ./waf build

# Copy the project files
RUN rm -rf scratch/*
ADD . scratch/smartgridsim/

# Build the final image
FROM intermediate
WORKDIR /workspace/ns-allinone-3.28/ns-3.28/scratch/smartgridsim/
ENTRYPOINT [ "./run.sh" ]
