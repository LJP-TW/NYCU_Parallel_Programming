#!/usr/bin/env python3
import subprocess

def test_mpi_hello(np=8):
    cmd = ['mpirun', '-np', str(np), '--hostfile', 'hosts_mpi.txt', 'mpi_hello']

    with open('report.txt', "a") as report:
        report.write('mpirun -np {} --hostfile hosts_mpi.txt mpi_hello\n'.format(np))
        report.flush()
        subprocess.run(cmd, stdout=report)
        report.write('\n')
        report.flush()

with open('report.txt', "w") as report:
    report.write('')

test_mpi_hello(8)
test_mpi_hello(10)
