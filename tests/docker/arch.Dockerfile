FROM archlinux:latest

RUN pacman -Syu --noconfirm \
    base-devel cmake ncurses sdl2 git jdk21-openjdk curl unzip which

WORKDIR /workspace
COPY . /workspace

CMD ["bash", "/workspace/tests/docker/run_in_container.sh"]
