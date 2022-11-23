#!/usr/bin/env python3
import subprocess

def test(np=8, hostfile='hosts_part2_4slots.txt', infile='./testdata/data1_1', ansfile='./testdata/ans1_1'):
    cmd = ['mpirun', '-np', str(np), '--hostfile', hostfile, 'matmul']

    with open(infile, "r") as f:
        with subprocess.Popen(cmd, stdin=f, stdout=subprocess.PIPE) as proc:
            output = proc.stdout.read()

    with open(ansfile, "rb") as f:
        ans = f.read()

    output = output.split(b'MPI running time', 1)
    myans = output[0]
    runtime = b'MPI running time' + output[1]

    with open('report.txt', "a") as report:
        report.write('mpirun -np {} --hostfile {} matmul < {}\n'.format(np, hostfile, infile))

        if ans != myans:
            report.write('[WA]\n')
        else:
            report.write(runtime.decode())

        report.write('\n')

with open('report.txt', "w") as report:
    report.write('')

test(1, 'hosts_part2_4slots.txt', './testdata/data0_1', './testdata/ans0_1')
test(1, 'hosts_part2_4slots.txt', '/home/.grade/HW4/data-set/data1_1', '/home/.grade/HW4/data-set/ans1_1')
test(1, 'hosts_part2_4slots.txt', '/home/.grade/HW4/data-set/data2_1', '/home/.grade/HW4/data-set/ans2_1')
test(1, 'hosts_part2_4slots.txt', '/home/.grade/HW4/data-set/data2_8', '/home/.grade/HW4/data-set/ans2_8')
test(1, 'hosts_part2_4slots.txt', '/home/.grade/HW4/data-set/data2_10', '/home/.grade/HW4/data-set/ans2_10')

test(4, 'hosts_part2_4slots.txt', './testdata/data0_1', './testdata/ans0_1')
test(4, 'hosts_part2_4slots.txt', '/home/.grade/HW4/data-set/data1_1', '/home/.grade/HW4/data-set/ans1_1')
test(4, 'hosts_part2_4slots.txt', '/home/.grade/HW4/data-set/data2_1', '/home/.grade/HW4/data-set/ans2_1')
test(4, 'hosts_part2_4slots.txt', '/home/.grade/HW4/data-set/data2_8', '/home/.grade/HW4/data-set/ans2_8')
test(4, 'hosts_part2_4slots.txt', '/home/.grade/HW4/data-set/data2_10', '/home/.grade/HW4/data-set/ans2_10')

test(7, 'hosts_part2_7slots.txt', './testdata/data0_1', './testdata/ans0_1')
test(7, 'hosts_part2_7slots.txt', '/home/.grade/HW4/data-set/data1_1', '/home/.grade/HW4/data-set/ans1_1')
test(7, 'hosts_part2_7slots.txt', '/home/.grade/HW4/data-set/data2_1', '/home/.grade/HW4/data-set/ans2_1')
test(7, 'hosts_part2_7slots.txt', '/home/.grade/HW4/data-set/data2_8', '/home/.grade/HW4/data-set/ans2_8')
test(7, 'hosts_part2_7slots.txt', '/home/.grade/HW4/data-set/data2_10', '/home/.grade/HW4/data-set/ans2_10')
