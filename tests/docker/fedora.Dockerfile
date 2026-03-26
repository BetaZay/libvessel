FROM fedora:latest

RUN dnf update -y && dnf install -y \
    gcc gcc-c++ libstdc++-static cmake make ncurses-devel SDL2-devel git \
    java-21-openjdk-devel curl unzip which \
    && dnf clean all

WORKDIR /workspace
COPY . /workspace

CMD ["bash", "/workspace/tests/docker/run_in_container.sh"]
