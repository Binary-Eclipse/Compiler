# ShakilScript Online Playground

Programiz-style page: editor + Run button + Output + Input box, powered by your real C/Flex/Bison interpreter.

## Option A - Show faculty from your own laptop (2 minutes)
Needs gcc and Python 3 (Linux, macOS, WSL, or MSYS2 UCRT64 on Windows).
    sh run_local.sh
Open http://localhost:5000. Others on the same Wi-Fi can open http://YOUR-LAPTOP-IP:5000.

## Option B - Public link (free), via Render
1. Push this folder to a new GitHub repository.
2. On render.com: New > Web Service > connect the repo > Runtime "Docker" > Free plan > Deploy.
3. You get https://your-name.onrender.com to share with your faculty.
(The free plan sleeps after inactivity; the first load may take about 30 s. Open it once before your demo.)

## Safety limits (app.py)
5 s time limit, 256 MB memory, 100 KB output, runs as non-root user inside Docker.

## Language words
ShakilScript uses its own names (show, ask, when, each, task, give, push, size ...). See NAMES.md.
