FROM fedora:latest

RUN dnf update -y && dnf install -y \
    gcc gcc-c++ libstdc++-static cmake make ncurses-devel SDL2-devel git \
    && dnf clean all

WORKDIR /workspace
COPY . /workspace

CMD ["bash", "/workspace/vessel/tests/scripts/run_tests.sh"]
