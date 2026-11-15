FROM python:3.12-slim
RUN apt-get update && apt-get install -y --no-install-recommends gcc libc6-dev make && rm -rf /var/lib/apt/lists/*
WORKDIR /app
COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt
COPY . .
# Build the interpreter from the bundled (pre-generated) parser/lexer C files
RUN gcc -std=c11 -D_POSIX_C_SOURCE=200809L -O2 -Iinterpreter/src -o interpreter/shakilscript \
    interpreter/main.c interpreter/parser.tab.c interpreter/lex.yy.c interpreter/src/runtime.c interpreter/src/names.c -lm \
 && apt-get purge -y gcc make && apt-get autoremove -y
RUN useradd -m runner && chown -R runner /app
USER runner
ENV PORT=8080
CMD gunicorn -b 0.0.0.0:$PORT -w 2 --timeout 30 app:app
