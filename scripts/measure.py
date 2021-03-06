import argparse
import concurrent.futures
import os
import pathlib
import subprocess
import sys
from logging import DEBUG, basicConfig, getLogger
from typing import *

logger = getLogger(__name__)


def gen(*, seeds: List[int]) -> None:
    logger.info('running the generator...')
    with open('seeds.txt', 'w') as fh:
        for seed in seeds:
            print(seed, file=fh)
    with open('seeds.txt') as fh:
        subprocess.check_call(['cargo', 'run', '--manifest-path', str(pathlib.Path('tools', 'Cargo.toml')), '--bin', 'gen'], stdin=fh)


def run(*, command: str, input_path: pathlib.Path, output_path: pathlib.Path, seed: int) -> None:
    logger.info('running the command for seed %d...', seed)
    try:
        with open(input_path) as fh1:
            with open(output_path, 'w') as fh2:
                subprocess.check_call(command, stdin=fh1, stdout=fh2)
    except subprocess.SubprocessError:
        logger.exception('failed for seed = %d', seed)


def vis(*, input_path: pathlib.Path, output_path: pathlib.Path, vis_path: pathlib.Path, seed: int) -> int:
    logger.info('running the visualizer for seed %d...', seed)
    try:
        score_str = subprocess.check_output(['cargo', 'run', '--manifest-path', str(pathlib.Path('tools', 'Cargo.toml')), '--bin', 'vis', '--', str(input_path), str(output_path)])
    except subprocess.SubprocessError:
        logger.exception('failed for seed = %d', seed)
        return 0
    os.rename('out.svg', vis_path)
    return int(score_str)


def main() -> 'NoReturn':
    parser = argparse.ArgumentParser()
    parser.add_argument('-c', '--command', default='./a.out')
    parser.add_argument('-n', '--count', type=int, default=50)
    parser.add_argument('-j', '--jobs', type=int, default=2)
    parser.add_argument('--seed', type=int, default=0)
    args = parser.parse_args()

    basicConfig(level=DEBUG)

    # gen
    seeds = [args.seed + i for i in range(args.count)]
    gen(seeds=seeds)

    # run
    pathlib.Path('out').mkdir(exist_ok=True)
    with concurrent.futures.ThreadPoolExecutor(max_workers=args.jobs) as executor:
        for i, seed in enumerate(seeds):
            input_path = pathlib.Path('in', '%04d.txt' % i)
            output_path = pathlib.Path('out', '%04d.txt' % i)
            executor.submit(run, command=args.command, input_path=input_path, output_path=output_path, seed=seed)

    # vis
    pathlib.Path('vis').mkdir(exist_ok=True)
    scores: List[int] = []
    for i, seed in enumerate(seeds):
        input_path = pathlib.Path('in', '%04d.txt' % i)
        output_path = pathlib.Path('out', '%04d.txt' % i)
        vis_path = pathlib.Path('vis', '%04d.svg' % i)
        score = vis(input_path=input_path, output_path=output_path, vis_path=vis_path, seed=seed)
        scores.append(score)
        logger.info('seed = {}: score = {}'.format(seed, score))
    logger.info('50 * average = %d', int(50 * sum(scores) / len(scores)))

    if min(scores) <= 0:
        sys.exit(1)
    sys.exit(0)


if __name__ == '__main__':
    main()
