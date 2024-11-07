from dataclasses import dataclass
from pathlib import Path
import argparse
import subprocess
import sys


class ComparisonError(Exception):
    pass


@dataclass(frozen=True)
class Args:
    program_under_test: Path
    reference_program: Path
    inputs_file: Path


def require_that_file_exists(path: Path) -> None:
    if not path.exists():
        exit(f"Error: File does not exist: {path}")
    if not path.is_file():
        exit(f"Error: This is not a regular file: {path}")


def read_args() -> Args:
    parser = argparse.ArgumentParser("compare_outputs")
    parser.add_argument("--put", dest="program_under_test", type=Path)
    parser.add_argument("--ref", dest="reference_program", type=Path)
    parser.add_argument("--inputs", dest="inputs_file", type=Path)
    args = parser.parse_args(sys.argv[1:])
    args_obj = Args(
        program_under_test=args.program_under_test,
        reference_program=args.reference_program,
        inputs_file=args.inputs_file,
    )
    require_that_file_exists(args_obj.program_under_test)
    require_that_file_exists(args_obj.reference_program)
    require_that_file_exists(args_obj.inputs_file)
    return args_obj


def run_program(binary: Path, input_data: str) -> str:
    process = subprocess.run([binary], input=input_data, capture_output=True, text=True)
    if process.returncode != 0:
        raise ComparisonError(
            f"Program {binary.name} exited with error on input '{input_data}':\n"
            "---STDOUT---\n"
            f"{process.stdout}\n"
            "---STDERR---\n"
            f"{process.stderr}\n"
        )
    output_text: str = process.stdout
    last_line = output_text.splitlines()[-1].strip()
    return last_line


def check_outputs(input_file: Path, program: Path, reference: Path) -> None:
    with open(input_file, "r") as file:
        for line_num, input_data in enumerate(file, start=1):
            input_data = input_data.strip()
            if not input_data:
                continue
            program_output = run_program(program, input_data)
            reference_output = run_program(reference, input_data)
            if program_output != reference_output:
                raise ComparisonError(
                    f"Output mismatch on line {line_num} of file '{input_file.name}':\n"
                    f"\tProgram {program.name} returned: '{program_output}'\n"
                    f"\tReference {reference.name} returned: '{reference_output}'\n"
                )


def main() -> None:
    args = read_args()
    try:
        check_outputs(args.inputs_file, args.program_under_test, args.reference_program)
    except ComparisonError as e:
        exit("Error: " + str(e))


if __name__ == "__main__":
    main()
