## Actual steps

* nix-shell
* ./subprojects/fetch-deps.py
* make mesonSetupDebug
* make compile
* steam-run make run
* make compile && LD_LIBRARY_PATH=/etc/lsb/lib/ make run
