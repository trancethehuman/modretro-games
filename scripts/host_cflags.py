"""Warning flags the host compiler needs for the engine's intentional idioms.

The radio text tables are exactly sized char arrays without a terminating
NUL. Newer GCC and Clang warn about that (-Wunterminated-string-initialization),
which -Werror turns into a failure; older compilers do not know the option.
Only flags the compiler accepts are returned.
"""
import functools
import subprocess
import tempfile
from pathlib import Path

CANDIDATES = ("-Wno-unterminated-string-initialization",)


@functools.lru_cache(maxsize=None)
def extra_flags(compiler):
    flags = []
    with tempfile.TemporaryDirectory() as tmp:
        source = Path(tmp) / "probe.c"
        source.write_text("int main(void){return 0;}\n")
        for flag in CANDIDATES:
            result = subprocess.run([compiler, "-Werror", flag, "-c", str(source), "-o", str(Path(tmp) / "probe.o")],
                                    capture_output=True)
            if result.returncode == 0:
                flags.append(flag)
    return tuple(flags)
