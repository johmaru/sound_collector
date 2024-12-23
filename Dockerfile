# Use an official GCC image as the base image
FROM gcc:13.3.0

RUN apt-get update && \
    apt-get install -y \
    portaudio19-dev \
    libasound2-dev \
    && rm -rf /var/lib/apt/lists/*

# Set the working directory in the container
WORKDIR /app

# Copy the current directory contents into the container at /app
COPY . /app

# Compile the C program
RUN gcc -I. main.c commands/mp3_analyzer.c -o SoundCollector -lportaudio

ENTRYPOINT ["./SoundCollector"]
CMD ["-3", "test.mp3"]