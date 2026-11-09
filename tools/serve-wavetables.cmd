@echo off
rem Serves the wavetable browser at http://127.0.0.1:8797/wavetable-browser.html (Chrome/Edge).
rem Opening it over http lets Chrome run the audio engine on its own audio thread.
cd /d "%~dp0"
echo Open http://127.0.0.1:8797/wavetable-browser.html  (Ctrl+C to stop)
start "" http://127.0.0.1:8797/wavetable-browser.html
python -m http.server 8797 --bind 127.0.0.1
