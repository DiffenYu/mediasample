FROM centos:7.6.1810

ARG http_proxy=""
ARG https_proxy=""

ENV http_proxy=$http_proxy
ENV https_proxy=$http_proxy

COPY yumrepos/* /etc/yum.repos.d/

RUN yum groupinstall -y "GNOME Desktop"
RUN yum groupinstall -y "Development Tools"

RUN yum install -y openssh-server openssh-clients
RUN sed -i 's/PermitRootLogin prohibit-password/PermitRootLogin yes/g' /etc/ssh/sshd_config && \
        ssh-keygen -q -t rsa -b 2048 -f /etc/ssh/ssh_host_rsa_key -N '' && \
        ssh-keygen -q -t ecdsa -f /etc/ssh/ssh_host_ecdsa_key -N '' && \
        ssh-keygen -t dsa -f /etc/ssh/ssh_host_ed25519_key  -N ''

