FROM ubuntu:24.04

RUN apt update

# Install build requirements
RUN apt install gcc libpam0g-dev -y

# Install wordlist for Wordle
RUN apt install wamerican -y

# Add demo user
RUN useradd wordle

# Build PAM-Wordle
COPY wordle.c .
RUN gcc -fPIC -c wordle.c
RUN ld -x --shared -o pam_wordle.so wordle.o
# Copy module where PAM can locate it (using * as path depends on platform architecture)
RUN cp pam_wordle.so /lib/*-linux-gnu/security/

# Configure su to use PAM-Wordle
RUN echo "auth required pam_wordle.so" > /etc/pam.d/su
RUN echo "@include common-auth" >> /etc/pam.d/su
RUN echo "@include common-account" >> /etc/pam.d/su
RUN echo "@include common-session" >> /etc/pam.d/su

ENTRYPOINT [ "sh", "-c", "echo 'Welcome to the PAM-Wordle demo container! Run `su wordle` to begin.' && /bin/bash"]
