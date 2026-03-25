FROM ubuntu:20.04
ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y \
    build-essential cmake libncurses5-dev libncursesw5-dev libsdl2-dev git \
    && rm -rf /var/lib/apt/lists/*

WORKDIR /workspace
COPY . /workspace

CMD ["bash", "/workspace/tests/scripts/run_tests.sh"]
