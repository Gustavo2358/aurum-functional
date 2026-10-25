#!/usr/bin/env python3
"""Public CLI boundary checks; expected values are fixed independently of Aurum."""
import json
import os
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
BINARY = Path(os.environ.get("AURUM_BINARY", ROOT / "build" / "aurum"))
BASE = b"CLOCK now=144600\nACCOUNT id=A1\nCARD id=C1 account_id=A1\nMERCHANT id=M1\n"
QUOTE = b'{"op":"QUOTE","decision":"OK","reason":"NONE","principal":100,"fee":0}\n'
AUTHORIZE = b"AUTHORIZE request_id=R1 account_id=A1 card_id=C1 merchant_id=M1 amount=100"
checks = 0


def run(commands, seed=BASE, env=None):
    with tempfile.TemporaryDirectory(prefix="aurum-protocol-") as d:
        path = Path(d) / "seed.txt"
        path.write_bytes(seed)
        return subprocess.run([str(BINARY), "--seed", str(path), "--commands", "-"],
                              input=commands, capture_output=True, env=env, check=False)


def check(condition, label):
    global checks
    assert condition, label
    checks += 1


def rows(result):
    return [json.loads(line) for line in result.stdout.splitlines()]


def main():
    result = run(b"QUOTE amount=000100\r\n")
    check(result.returncode == 0 and result.stdout == QUOTE and not result.stderr,
          "CRLF, normalized integer and exact canonical JSON")
    for length in (8192, 8193, 65536):
        command = b"QUOTE amount=100"
        command += b" " * (length - len(command) - 1) + b"\nQUOTE amount=100\n"
        result = run(command)
        output = rows(result)
        check(result.returncode == 0 and len(output) == 2 and output[-1]["principal"] == 100,
              f"line {length} drains and preserves following command")
        check(output[0]["decision"] == ("OK" if length == 8192 else "ERROR"),
              f"line {length} exact byte limit")
    for ending in (b"\n", b"\r\n", b""):
        command = b"QUOTE amount=100"
        command += b" " * (8192 - len(command) - len(ending)) + ending
        result = run(command)
        check(result.stdout == QUOTE and result.returncode == 0, "boundary includes terminator")
    for byte in (b"\x00", b"\x01", b"\x0b", b"\x7f", b"\x80", b"\xff", b"\r"):
        result = run(b"QUOTE amount=100" + byte + b"\nQUOTE amount=100\n")
        # A trailing CR followed by LF is the one permitted CR position.
        first = "OK" if byte == b"\r" else "ERROR"
        check([r["decision"] for r in rows(result)] == [first, "OK"], "ASCII grammar and complete drain")
    result = run(b"\n\t \n# comment\n \t# comment\r\nQUOTE amount=100 # inline\n")
    check(result.returncode == 0 and len(rows(result)) == 1 and rows(result)[0]["reason"] == "INVALID_INPUT",
          "ignored lines and forbidden inline comment")
    invalid = [
        b"QUOTE amount=0", b"QUOTE amount=+1", b"QUOTE amount=-1", b"QUOTE amount=1.0",
        b"QUOTE amount=1e3", b"QUOTE amount=1000000000001", b"QUOTE amount=9999999999999999999999999999999999",
        b"QUOTE amount=1 amount=1", b"QUOTE amount=1 amount=2", b"QUOTE amunt=1",
        b"QUOTE amount=1 magic=true", b"QUOTE amount=1=2", b"QUOTE amount=",
        b"QUOTE amount=1 currency=brl", b"QUOTE amount=1 country=USA", b"QUOTE amount=1 installments=-1",
        b"AUTHORIZE request_id=R1 account_id=A1 card_id=C1 merchant_id=M1",
        AUTHORIZE + b" channel=DRONE", AUTHORIZE + b" pin_ok=1", AUTHORIZE + b" card_present=True",
        b"GET_ACCOUNT account_id=A/1", b"GET_ACCOUNT account_id=" + b"A" * 49,
        b"TICK now=525600001", b"TICK now=-1", b"RECONCILE request_id=R1", b"CONFIG daily_count_limit=100",
    ]
    for command in invalid:
        result = run(command + b"\nQUOTE amount=100\n")
        output = rows(result)
        check(result.returncode == 0 and len(output) == 2 and output[0]["decision"] == "ERROR"
              and output[0]["reason"] == "INVALID_INPUT" and output[1]["principal"] == 100,
              "invalid envelope: " + repr(command))
    for command, reason in ((b"QUOTE amount=100 currency=ZZZ", "UNSUPPORTED_CURRENCY"),
                            (b"QUOTE amount=100 installments=0", "INSTALLMENTS"),
                            (b"QUOTE amount=100 installments=13", "INSTALLMENTS")):
        result = run(command + b"\n")
        check(rows(result)[0]["decision"] == "DECLINED" and rows(result)[0]["reason"] == reason,
              "business rejection remains outside lexical envelope")
    invalid_seeds = [
        b"", b"ACCOUNT id=A1\n", BASE + b"CLOCK now=1\n", BASE + b"ACCOUNT id=A2\n",
        BASE + b"CONFIG fx_usd=50000\n", b"CLOCK now=0\nCARD id=C1 account_id=A1\n",
        b"CLOCK now=0\nACCOUNT id=A1\nACCOUNT id=A1\n",
        b"CLOCK now=0\nCONFIG fx_usd=0\n", b"CLOCK now=0\nCONFIG fx_brl=10001\n",
        b"CLOCK now=0\nCONFIG fx_usd=50000\nCONFIG fx_usd=50000\n",
        b"CLOCK now=0\nCONFIG keys_capacity=0\n", b"CLOCK now=0\nCONFIG magic=1\n",
        b"CLOCK now=0\nMERCHANT id=M1\nCARD id=C1 account_id=A1\n",
        b"CLOCK now=0\nACCOUNT id=A1 mystery=1\n", b"CLOCK now=0 now=0\n",
        b"CLOCK now=525600001\n", b"CLOCK now=0\nCONFIG\n",
    ]
    for seed in invalid_seeds:
        result = run(b"QUOTE amount=100\n", seed)
        check(result.returncode == 2 and result.stdout == b"" and bool(result.stderr),
              "invalid seed never starts stream: " + repr(seed))
    result = run(AUTHORIZE + b"\n", b"CLOCK now=144600\n")
    check(rows(result)[0]["reason"] == "NOT_FOUND", "seed does not create implicit entities")
    result = run(b"QUOTE amount=100\n", b"CLOCK now=0\nCONFIG review_score=90\nCONFIG decline_score=95\n")
    check(result.stdout == QUOTE, "configuration validated after whole configuration section")
    result = run(AUTHORIZE + b"\nCAPTURE request_id=K1 auth_id=R1 principal=100\n" +
                 b"AUTHORIZE amount=000100 merchant_id=M1 card_id=C1 account_id=A1 request_id=R1 "
                 b"currency=BRL installments=1 country=BR channel=POS card_present=true pin_ok=true\n" +
                 b"GET_ACCOUNT account_id=A1\nGET_AUTH auth_id=R1\nRECONCILE\n")
    output = rows(result)
    check(result.returncode == 0 and result.stdout.splitlines()[0] == result.stdout.splitlines()[2],
          "canonical defaults and lexical order give original bytes after state changes")
    check(output[3]["debt"] == 100 and output[3]["held"] == 0 and output[3]["auth_ids"] == ["R1"]
          and output[4]["state"] == "CAPTURED" and output[5]["status"] == "OK"
          and output[5]["debt_difference"] == 0 and output[5]["held_difference"] == 0,
          "real state projections after capture and cached replay")
    commands = b"\n".join(AUTHORIZE.replace(b"request_id=R1", b"request_id=" + rid)
                            for rid in (b"Z9", b"a1", b"A1")) + b"\nGET_ACCOUNT account_id=A1\n"
    check(rows(run(commands))[-1]["auth_ids"] == ["A1", "Z9", "a1"], "ASCII report ordering")
    baseline = run(AUTHORIZE + b"\nTICK now=146040\nGET_ACCOUNT account_id=A1\n").stdout
    for changes in ({"LC_ALL": "C", "TZ": "UTC"}, {"LC_ALL": "C.UTF-8", "TZ": "Pacific/Honolulu"}):
        env = dict(os.environ, **changes)
        check(run(AUTHORIZE + b"\nTICK now=146040\nGET_ACCOUNT account_id=A1\n", env=env).stdout == baseline,
              "environment-independent bytes")
    with tempfile.TemporaryDirectory(prefix="aurum-protocol-") as d:
        seed = Path(d) / "seed.txt"
        seed.write_bytes(BASE)
        with open("/dev/full", "wb") as full:
            result = subprocess.run([str(BINARY), "--seed", str(seed), "--commands", "-"],
                                    input=b"QUOTE amount=100\n", stdout=full, stderr=subprocess.PIPE, check=False)
        check(result.returncode == 2 and bool(result.stderr), "blocking output failure exits 2")
    with tempfile.TemporaryDirectory(prefix="aurum-protocol-") as d:
        seed = Path(d) / "seed.txt"
        seed.write_bytes(BASE)
        child = subprocess.Popen([str(BINARY), "--seed", str(seed), "--commands", "-"],
                                 stdin=subprocess.PIPE, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
        child.stdout.close()
        child.stdin.write(b"QUOTE amount=100\n")
        child.stdin.close()
        diagnostic = child.stderr.read()
        child.stderr.close()
        check(child.wait() == 2 and bool(diagnostic), "closed output pipe exits 2")
    print(json.dumps({"protocol_adversarial": "PASS", "checks": checks}, sort_keys=True))


if __name__ == "__main__":
    main()
