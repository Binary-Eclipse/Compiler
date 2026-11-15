"""ShakilScript online playground - small Flask backend.

POST /api/run  {"code": "...", "stdin": "..."}  ->  {"output", "exit_code", "timed_out", "time_ms"}
GET  /api/examples                               ->  list of bundled example programs
"""
import json, os, resource, subprocess, tempfile, time
from flask import Flask, jsonify, request, send_from_directory

BASE = os.path.dirname(os.path.abspath(__file__))
INTERPRETER = os.environ.get("SS_BIN", os.path.join(BASE, "interpreter", "shakilscript"))
TIMEOUT_S = 5            # wall-clock limit per run
MAX_OUTPUT = 100_000     # bytes of output returned
MAX_CODE = 50_000        # bytes of source accepted
MEM_LIMIT = 256 * 1024 * 1024

app = Flask(__name__, static_folder="static", static_url_path="/static")


def _limits():
    resource.setrlimit(resource.RLIMIT_CPU, (TIMEOUT_S, TIMEOUT_S))
    resource.setrlimit(resource.RLIMIT_AS, (MEM_LIMIT, MEM_LIMIT))
    resource.setrlimit(resource.RLIMIT_FSIZE, (1_000_000, 1_000_000))
    resource.setrlimit(resource.RLIMIT_NPROC, (64, 64))
    os.setsid()


@app.get("/")
def index():
    return send_from_directory("static", "index.html")


@app.get("/guide")
def guide():
    return send_from_directory("static", "guidebook.html")


@app.get("/api/examples")
def examples():
    out = []
    d = os.path.join(BASE, "examples")
    for f in sorted(os.listdir(d)):
        if not f.endswith(".ss"):
            continue
        stem = f[:-3]
        stdin_path = os.path.join(d, stem + ".in")
        out.append({
            "name": stem[3:].replace("_", " ").capitalize() if stem[:2].isdigit() else stem,
            "code": open(os.path.join(d, f), encoding="utf-8").read(),
            "stdin": open(stdin_path, encoding="utf-8").read() if os.path.exists(stdin_path) else "",
        })
    return jsonify(out)


@app.post("/api/run")
def run():
    data = request.get_json(silent=True) or {}
    code, stdin = str(data.get("code", "")), str(data.get("stdin", ""))
    if len(code.encode()) > MAX_CODE:
        return jsonify(output="Program too large.", exit_code=1, timed_out=False, time_ms=0), 413
    with tempfile.TemporaryDirectory() as tmp:
        src = os.path.join(tmp, "main.ss")
        with open(src, "w", encoding="utf-8", newline="\n") as fh:
            fh.write(code if code.endswith("\n") else code + "\n")
        t0, timed_out = time.time(), False
        try:
            p = subprocess.run([INTERPRETER, src], input=stdin.encode(), capture_output=True,
                               timeout=TIMEOUT_S, cwd=tmp, preexec_fn=_limits, env={})
            out, err, rc = p.stdout, p.stderr, p.returncode
        except subprocess.TimeoutExpired as e:
            out, err, rc, timed_out = e.stdout or b"", e.stderr or b"", -1, True
        text = (out + err).decode("utf-8", "replace")
        if len(text) > MAX_OUTPUT:
            text = text[:MAX_OUTPUT] + "\n... output truncated ..."
        if timed_out:
            text += f"\n\n[Stopped: program ran longer than {TIMEOUT_S} seconds - infinite loop?]"
        elif rc < 0:
            text += f"\n\n[Program crashed (signal {-rc}) - possibly too much memory or CPU]"
        return jsonify(output=text, exit_code=rc, timed_out=timed_out,
                       time_ms=int((time.time() - t0) * 1000))


if __name__ == "__main__":
    app.run(host="0.0.0.0", port=int(os.environ.get("PORT", 5000)))
