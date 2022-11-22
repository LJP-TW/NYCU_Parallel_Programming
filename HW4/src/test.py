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

def test_pi_block_linear(np=8, pi_block_linear='pi_block_linear'):
    cmd = ['mpirun', '-np', str(np), '--hostfile', 'hosts_pi_block_linear.txt', pi_block_linear, '1000000000']

    with open('report.txt', "a") as report:
        report.write('mpirun -np {} --hostfile hosts_pi_block_linear.txt {} 1000000000\n'.format(np, pi_block_linear))
        report.flush()
        subprocess.run(cmd, stdout=report)
        report.write('\n')
        report.flush()

with open('report.txt', "w") as report:
    report.write('')

test_mpi_hello(8)
test_mpi_hello(10)

test_pi_block_linear(2, 'pi_block_linear')
test_pi_block_linear(2, '/HW4/ref/pi_block_linear')
test_pi_block_linear(4, 'pi_block_linear')
test_pi_block_linear(4, '/HW4/ref/pi_block_linear')
test_pi_block_linear(8, 'pi_block_linear')
test_pi_block_linear(8, '/HW4/ref/pi_block_linear')
test_pi_block_linear(12, 'pi_block_linear')
test_pi_block_linear(12, '/HW4/ref/pi_block_linear')
test_pi_block_linear(16, 'pi_block_linear')
test_pi_block_linear(16, '/HW4/ref/pi_block_linear')
