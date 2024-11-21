RED = "\033[31m"
YELLOW = "\033[33m"
BLUE = "\033[34m"
CYAN = "\033[36m"
GREEN = "\033[32m"
CLEAR = "\033[0m"
import os
from typing import Union


def check_result(result: int) -> None:
    if result != 0:
        raise ValueError(f"{RED}{__file__}: Command failed{CLEAR}")
    print(f"{GREEN}{__file__}: Command success{CLEAR}")


def run_cmd(cmd: str) -> None:
    print(f"{YELLOW}{__file__}: Running {CLEAR}{cmd}")
    check_result(os.system(cmd))


def initialize(index: Union[int, str]) -> None:
    cmd = f"git fetch --all &&git merge origin/check{index}-startercode"
    run_cmd(cmd)
    cmd = "cmake -S . -B build"
    run_cmd(cmd)
    cmd = "cmake --build build"


def test(index: Union[int, str]) -> None:
    if not isinstance(index, int):
        try:
            index = int(index)
        except:
            raise TypeError(f"index must be an integer, but got {index}")
    cmd = f"cmake --build build --target check{index}"
    run_cmd(cmd)


def make_zip(index: Union[int, str]) -> None:
    if not isinstance(index, int):
        try:
            index = int(index)
        except:
            raise TypeError(f"index must be an integer, but got {index}")
    cmd = f"cd .. && zip -r 221240060-熊浚丞-check{index}.zip ./221240060-熊浚丞-check{index}.pdf ./minnow > /dev/null"
    run_cmd(cmd)


def main(*args) -> None:
    args = list(args)
    if not args:
        raise ValueError("No arguments provided")
    function = args.pop(0)
    # 将字符串转换为函数
    function = globals().get(function)
    if not function:
        raise ValueError(f"Unknown function: {function}")
    function(*args)


if __name__ == "__main__":
    import sys

    main(*sys.argv[1:])
