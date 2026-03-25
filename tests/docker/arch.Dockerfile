FROM archlinux:latest

RUN pacman -Syu --noconfirm base-devel cmake ncurses sdl2 git

WORKDIR /workspace
COPY . /workspace

CMD ["bash", "/workspace/vessel/tests/scripts/run_tests.sh"]
