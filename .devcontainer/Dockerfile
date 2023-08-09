ARG esphome_version
FROM esphome/esphome-lint:${esphome_version}

ARG esphome_version
ARG apt_deps
ARG pip_deps
ARG workspace_dir
ARG pio_dir=${workspace_dir}/.platformio

RUN export DEBIAN_FRONTEND=noninteractive \
 && apt-get update \
 && apt-get install -y --no-install-recommends sudo ${apt_deps} \
 && apt-get autoremove -y \
 && apt-get clean -y \
 && rm -rf /var/lib/apt/lists/*

# pyparsing==2.2.0 needed to correctly compile with esp-idf: https://github.com/esphome/issues/issues/2540
RUN pip3 --disable-pip-version-check --no-cache-dir install esphome==${esphome_version} ${pip_deps} \
 && pio settings set enable_telemetry No \
 && pio settings set check_platformio_interval 1000000 \
 && rm -rf /tmp/pip-tmp

RUN useradd -m -s /bin/bash -G sudo vscode \
 && echo "vscode ALL=(root) NOPASSWD:ALL" > /etc/sudoers.d/vscode \
 && chmod 0440 /etc/sudoers.d/vscode

ENV PLATFORMIO_CORE_DIR="${pio_dir}"
RUN mkdir -p "${pio_dir}"

RUN mv /root/.platformio /home/vscode \
 && chown -R vscode:vscode /home/vscode/.platformio /esphome /piolibs "${pio_dir}"

ENV WORKSPACE_DIR=${workspace_dir}

# Set up tmpfs volumes.
VOLUME ["/tmp", "/run"]

COPY .bash_aliases /home/vscode/.bash_aliases
RUN chown vscode:vscode /home/vscode/.bash_aliases

LABEL devcontainer.metadata='[{ \
  "remoteUser": "vscode", \
}]'

