FROM python:3.6

RUN apt-get update
RUN apt-get install -y vim

WORKDIR /root/files

# Project Structure setup

# Python Requirements
# https://spinnakermanchester.github.io/development/devenv.html#PythonRequirements

RUN pip install --upgrade pip setuptools wheel
RUN pip install "appdirs>=1.4.2,<2.0.0" future "numpy>=1.12,<1.9999"  "scipy>=0.16.0" "six>=1.8.0" "pylru>=1" enum34 future lxml jsonschema sortedcollections
RUN pip install  "rig>=2.0.0,<3.0.0" futures enum-compat pytz tzlocal "requests>=2.4.1" matplotlib
RUN pip install  csa "quantities>=0.12.1" "pynn>=0.9.2,<0.10" "lazyarray>=0.2.9,<=0.4.0" "neo>=0.5.2,< 0.7.0"

# C Requirements
# https://spinnakermanchester.github.io/common_pages/5.0.0/Compiler.html

# RUN apt-get install -y gcc-arm-none-eabi libnewlib-arm-none-eabi

RUN apt-get install -y perl perl-tk libterm-readline-gnu-perl

# Git Cloning

RUN for PROJ in spinnaker_tools spinn_common SpiNNUtils SpiNNMachine spalloc  spalloc_server PACMAN SpiNNMan DataSpecification  SpiNNFrontEndCommon  SpiNNakerGraphFrontEnd sPyNNaker  sPyNNaker8 sPyNNaker8NewModelTemplate PyNN8Examples  sPyNNakerVisualisers IntroLab JavaSpiNNaker  SupportScripts; do git clone https://github.com/SpiNNakerManchester/$PROJ --depth 1; done

RUN ./SupportScripts/setup.sh
ENV SPINN_DIRS /root/files/spinnaker_tools
ENV PATH $PATH:/root/files/spinnaker_tools/tools
ENV PERL5LIB /root/files/spinnaker_tools/tools
ENV NEURAL_MODELLING_DIRS /root/files/sPyNNaker/neural_modelling

RUN curl -sSL "https://developer.arm.com/-/media/Files/downloads/gnu-rm/9-2020q2/gcc-arm-none-eabi-9-2020-q2-update-x86_64-linux.tar.bz2?revision=05382cca-1721-44e1-ae19-1e7c3dc96118&la=en&hash=D7C9D18FCA2DD9F894FD9F3C3DC9228498FA281A" -o gcc-eabi.tar.bz2
RUN mkdir -p gcc-eabi && tar -xjf gcc-eabi.tar.bz2 -C gcc-eabi --strip-components 1
ENV PATH $PATH:/root/files/gcc-eabi/bin

RUN bash SupportScripts/automatic_make.sh
