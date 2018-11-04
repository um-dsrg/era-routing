#!/usr/bin/env python3
"""
Creates the necessary directory structure and files to run the testing of the
new objectives setup.
"""
import argparse
import os
import subprocess
from pathlib import Path

from modules.job_script import JobScript


def create_flow_set(out_lgf_path, num_flow, net_load):
    """Create a graph file with the specified details."""
    home_dir = str(Path.home())
    dev_dir = home_dir + '/Development/Development/SimulationSetup'
    command = ['python3',
               dev_dir + '/generate_random_flow_set.py',
               '--base_lgf_file', dev_dir + '/base_geant.lgf',
               '--save_lgf_file', out_lgf_path,
               '--num_flows', str(num_flow),
               '--scaling_factor', '30',
               '--flow_protocol', 'T',
               '--vid_res_prob']

    # Vid flow resolution order: SD, HD, UHD
    if net_load == 'high':
        command.extend(['0.0', '0.0', '1.0'])
    elif net_load == 'medium':
        command.extend(['0.0', '1.0', '0.0'])
    elif net_load == 'low':
        command.extend(['1.0', '0.0', '0.0'])
    else:
        raise RuntimeError('The following network load is unknown {}'.format(net_load))

    print(repr(command))

    process_status = subprocess.run(command)
    if process_status.returncode != 0:
        raise RuntimeError('The following command returned with errors\n{}'.format(command))


def create_ksp_jobscript(k_value, net_load, num_flows, base_path, shell_graph_path, shell_base_path):
    """Creates the KSP command and jobscript from the given parameters."""
    job_queue = 'short'
    job_name = 'ksp_{}l_f_{}_k_{}'.format(net_load[0], num_flows, k_value)
    job_logs_dir = '${HOME}/logs/'  # Nyquist home directory
    job_not_flags = 'a'
    job_email = 'noel.farrugia.09@um.edu.mt'
    ksp_job = JobScript(job_queue, job_name, job_logs_dir, job_not_flags, job_email)

    ksp_result_file = shell_base_path + '/result_ksp_' + k_value + '.xml'

    command = ('${{HOME}}/Development/KShortestPath/build/release/ksp \\\n'
               '    --k {} \\\n'
               '    --lgfPath {} \\\n'
               '    --kspXmlPath {}'.format(k_value, shell_graph_path, ksp_result_file))

    print(repr(command))
    ksp_job.insert_command(command)

    ksp_job_path = base_path + '/run_ksp.pbs'
    print('Saving Ksp job in {}'.format(ksp_job_path))
    ksp_job.save_job_script(ksp_job_path)


def main():
    # *Note* Shell_<name> variables represent directories that are meant to be used by
    # the shell. This are used when writing paths to the jobscript command where the
    # path will be interpreted by the shell not by the python interpreter.

    home_dir = str(Path.home())
    sp_objs_dir = 'GDrive/Scratchpad/genetic_algorithm/new_objectives'
    base_directory = str(Path.home()) + '/' + sp_objs_dir
    network_load = ['high', 'medium', 'low']
    k_values = ['5']

    # Create the directories
    for net_load in network_load:
        if net_load == 'high':  # The max num of flows under high load is 150
            num_flows = list(range(50, 151, 50))
        else:
            num_flows = list(range(50, 301, 50))

        for num_flow in num_flows:
            shell_base_dir = '${HOME}/' + sp_objs_dir  # Used when writing the path to be used by a shell
            curr_dir = base_directory + '/load_' + net_load + '/flows_' + str(num_flow)
            shell_curr_dir = shell_base_dir + '/load_' + net_load + '/flows_' + str(num_flow)

            print('Creating directory {}'.format(curr_dir))

            try:
                os.makedirs(curr_dir)
            except OSError:
                print('Error when creating the directory {}'.format(curr_dir))
                raise

            graph_path = curr_dir + '/graph.lgf'
            shell_graph_path = shell_curr_dir + '/graph.lgf'

            # Create the graphs with the desired flow distribution
            create_flow_set(graph_path, num_flow, net_load)

            # Create the KSP jobscripts
            for k_value in k_values:
                create_ksp_jobscript(k_value, net_load, num_flow, curr_dir, shell_graph_path, shell_curr_dir)

            # TODO Need to create the ga jobscripts


if __name__ == '__main__':
    main()
